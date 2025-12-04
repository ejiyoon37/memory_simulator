#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // getopt

#include "common.h"
#include "log.h"
#include "memory.h"
#include "tlb.h"
#include "page_table.h"
#include "swap.h"

// 전역 변수 정의
Policy g_policy = POLICY_RR; // 기본값 RR
uint64_t g_time = 0;         // 시뮬레이션 시간

// 명령줄 인자 저장용 변수
char *policy_str = NULL;
char *input_file = NULL;
char *output_file = NULL;

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -p <policy> -f <input_file> -l <output_file>\n", prog_name);
    fprintf(stderr, "  -p: replacement policy (RR or LRU)\n");
    fprintf(stderr, "  -f: input test case file\n");
    fprintf(stderr, "  -l: output log file\n");
}

int main(int argc, char *argv[]) {
    int opt;

    // 1. 명령줄 인자 파싱 (getopt 사용)
    while ((opt = getopt(argc, argv, "p:f:l:")) != -1) {
        switch (opt) {
            case 'p':
                policy_str = optarg;
                break;
            case 'f':
                input_file = optarg;
                break;
            case 'l':
                output_file = optarg;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // 필수 인자 확인
    if (!policy_str || !input_file || !output_file) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // 정책 설정
    if (strcmp(policy_str, "RR") == 0) {
        g_policy = POLICY_RR;
    } else if (strcmp(policy_str, "LRU") == 0) {
        g_policy = POLICY_LRU;
    } else {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // 2. 초기화 (로그, 메모리, TLB)
    open_log_file(output_file);
    init_memory();
    init_tlb();
    
    // 3. 입력 파일 열기
    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        perror("Failed to open input file");
        close_log_file();
        exit(EXIT_FAILURE);
    }

    int total_accesses = 0;
    // 첫 줄: 총 접근 횟수 읽기
    if (fscanf(fp, "%d", &total_accesses) != 1) {
        fprintf(stderr, "Invalid input file format.\n");
        fclose(fp);
        close_log_file();
        exit(EXIT_FAILURE);
    }

    // 4. Main Simulation Loop
    uint32_t va_temp; // fscanf는 32bit로 읽음
    
    // 파일에서 주소를 하나씩 읽음
    while (fscanf(fp, "%x", &va_temp) == 1) {
        uint16_t va = (uint16_t)va_temp;
        uint16_t vpn = GET_FULL_VPN(va);
        uint16_t offset = GET_OFFSET(va);

        // [LRU] 시간 증가 (메모리 접근 1회 = 시간 1 흐름)
        g_time++;

        // State Machine Loop
        while (1) {
            // (1) Access VA 로그 출력
            log_va_access(va);

            // (2) TLB Lookup
            int pfn = search_tlb(vpn); 

            if (pfn != -1) {
                // --- Case A: TLB Hit ---
                // [LRU] 데이터 페이지 접근 시간 갱신
                if (g_policy == POLICY_LRU) {
                    acknowledge_frame_access(pfn);
                }

                // (4) PA 계산 및 출력
                uint16_t pa = (pfn << 3) | offset;
                log_pa_result(pa);
                
                break; // 처리 완료
            } 
            else {
                // --- Case B: TLB Miss ---
                
                // (5) Page Table Lookup
                PT_Result pt_res = walk_page_table(va); 

                if (pt_res.hit) {
                    // --- Case B-1: Page Table Hit ---
                    update_tlb(vpn, pt_res.pfn);
                    
                    // [LRU] 여기서도 페이지 접근으로 칠 수 있지만, 
                    // 루프를 돌아 TLB Hit가 될 때 acknowledge_frame_access가 호출되므로 
                    // 여기서는 생략해도 무방하거나, 혹은 명시적으로 호출해도 됩니다.
                    // 구조상 continue 후 TLB Hit에서 처리됩니다.
                    
                    continue; // Retry
                } 
                else {
                    // --- Case B-2: Page Table Miss (Page Fault) ---
                    
                    // (6) Allocate Free Frame (or Swap)
                    int new_pfn = allocate_free_frame(vpn, true); 
                    
                    if (new_pfn == -1) {
                        // Memory Full -> Swap Out 발생
                        new_pfn = swap_out(); 
                        
                        // 다시 할당 시도
                        int retry_pfn = allocate_free_frame(vpn, true);
                        if (retry_pfn == -1) {
                            fprintf(stderr, "Critical Error: Memory allocation failed even after swap.\n");
                            exit(1);
                        }
                        new_pfn = retry_pfn;
                    }

                    // (7) Update Page Table & TLB
                    update_page_table(va, new_pfn);
                    update_tlb(vpn, new_pfn);
                    
                    continue; // Retry
                }
            }
        }
    }

    // 5. 종료 처리
    fclose(fp);
    close_log_file();
    
    return 0;
}