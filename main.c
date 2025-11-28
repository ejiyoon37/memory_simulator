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

    // 정책 확인 (Part 1은 RR만 우선 대응, 나중에 LRU 추가)
    // if (strcmp(policy_str, "RR") != 0 && strcmp(policy_str, "LRU") != 0) { ... }
    // 여기서는 일단 넘어갑니다.

    // 2. 초기화 (로그, 메모리, TLB)
    open_log_file(output_file);
    init_memory();
    init_tlb();
    
    // LRU 모드일 경우 swap 모듈 등에 알리는 초기화가 필요할 수 있음
    // set_swap_policy(policy_str); // 추후 구현

    // 3. 입력 파일 열기
    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        perror("Failed to open input file");
        close_log_file();
        exit(EXIT_FAILURE);
    }

    int total_accesses = 0;
    // 첫 줄: 총 접근 횟수 읽기 [cite: 294]
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

        // [핵심 로직] State Machine Loop (Slide 13)
        // Hit가 될 때까지 반복하거나, Update 후 재시도 구조
        while (1) {
            // (1) Access VA 로그 출력 [cite: 168]
            log_va_access(va);

            // (2) TLB Lookup [cite: 48, 52]
            int pfn = search_tlb(vpn); // 내부에서 Hit/Miss 로그 출력됨

            if (pfn != -1) {
                // --- Case A: TLB Hit ---
                // [cite: 169] If TLB hit, nothing to do (just calculate PA).
                
                // (3) Hit 로그 출력 (search_tlb 내부에서 이미 수행됨)
                
                // (4) PA 계산 및 출력
                uint16_t pa = (pfn << 3) | offset;
                log_pa_result(pa);
                
                // 이 주소 처리는 끝났으므로 다음 주소로 넘어감
                break; 
            } 
            else {
                // --- Case B: TLB Miss ---
                // (search_tlb 내부에서 Miss 로그 이미 출력됨)

                // (5) Page Table Lookup [cite: 170]
                PT_Result pt_res = walk_page_table(va); // 내부에서 PT Hit/Miss 로그 출력됨

                if (pt_res.hit) {
                    // --- Case B-1: Page Table Hit (Memory Resident) ---
                    // [cite: 170] Update TLB and the address is accessed again.
                    
                    update_tlb(vpn, pt_res.pfn); // 로그 출력됨
                    
                    // Loop 다시 시작 (Access VA부터 다시)
                    continue; 
                } 
                else {
                    // --- Case B-2: Page Table Miss (Page Fault) ---
                    // [cite: 172] If both TLB and PT miss, swap the page...
                    
                    // (6) Allocate Free Frame (or Swap)
                    // 데이터 페이지이므로 swappable = true
                    int new_pfn = allocate_free_frame(vpn, true); 
                    
                    if (new_pfn == -1) {
                        // Memory Full -> Swap Out 발생 [cite: 148]
                        new_pfn = swap_out(); // Victim 선정, TLB/PT Invalidate 수행
                        
                        // 다시 할당 시도 (이제 빈 공간이 있음)
                        // 주의: swap_out은 빈 공간만 만듦. allocate_free_frame을 다시 호출하거나
                        // swap_out이 반환한 pfn을 그대로 씀. 
                        // 제 memory.c 구현상 allocate_free_frame을 다시 불러서
                        // owner 설정 및 상태 갱신을 해야 함.
                        int retry_pfn = allocate_free_frame(vpn, true);
                        if (retry_pfn == -1) {
                            // 여기로 오면 심각한 에러 (Bitmask=0인 페이지만 가득 찬 경우 등)
                            fprintf(stderr, "Critical Error: Memory allocation failed even after swap.\n");
                            exit(1);
                        }
                        new_pfn = retry_pfn;
                    }

                    // (7) Update Page Table & TLB [cite: 172]
                    update_page_table(va, new_pfn); // 로그 출력됨
                    update_tlb(vpn, new_pfn);       // 로그 출력됨
                    
                    // [cite: 173] Then the address is accessed again.
                    continue;
                }
            }
        }
    }

    // 5. 종료 처리
    fclose(fp);
    close_log_file();
    
    return 0;
}