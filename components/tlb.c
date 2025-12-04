/* components/tlb.c */
#include "tlb.h"
#include "log.h"
#include <stdio.h>
#include "../common.h" // g_policy, g_time 외부 변수 참조

// 전역 변수 선언
TLB_Entry tlb[TLB_SIZE];
int tlb_rr_idx = 0; // RR 교체 포인터

// 1. TLB 초기화
void init_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = false;
        tlb[i].vpn = 0;
        tlb[i].pfn = 0;
        tlb[i].last_access_time = 0; // [LRU] 초기화
    }
    tlb_rr_idx = 0;
}

// 2. TLB 검색 (Lookup)
int search_tlb(uint16_t vpn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        // Valid하고 VPN이 일치하면 Hit!
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            log_tlb_hit(vpn, tlb[i].pfn);
            
            // [LRU] Hit 발생 시 접근 시간 갱신 (RR일 땐 무시됨)
            if (g_policy == POLICY_LRU) {
                tlb[i].last_access_time = g_time;
            }
            return tlb[i].pfn;
        }
    }

    log_tlb_miss(vpn);
    return -1;
}

// 3. TLB 업데이트 (Replacement)
void update_tlb(uint16_t vpn, uint16_t pfn) {
    int target_idx = -1;

    // A. 빈 공간이 있는지 먼저 확인
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            target_idx = i;
            break;
        }
    }

    // B. 빈 공간이 없다면 교체 정책에 따라 Victim 선정
    if (target_idx == -1) {
        if (g_policy == POLICY_RR) {
            // Round-Robin 방식
            target_idx = tlb_rr_idx;
            tlb_rr_idx = (tlb_rr_idx + 1) % TLB_SIZE;
        } 
        else if (g_policy == POLICY_LRU) {
            // [LRU] last_access_time이 가장 작은 엔트리 찾기
            uint64_t min_time = UINT64_MAX;
            for (int i = 0; i < TLB_SIZE; i++) {
                if (tlb[i].last_access_time < min_time) {
                    min_time = tlb[i].last_access_time;
                    target_idx = i;
                }
            }
        }
    }

    // C. 엔트리 업데이트
    tlb[target_idx].vpn = vpn;
    tlb[target_idx].pfn = pfn;
    tlb[target_idx].valid = true;
    
    // [LRU] 새로운 엔트리가 들어왔으므로 현재 시간으로 갱신
    if (g_policy == POLICY_LRU) {
        tlb[target_idx].last_access_time = g_time;
    }

    log_tlb_update(vpn, pfn);
}

void invalidate_tlb_by_vpn(uint16_t vpn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            tlb[i].valid = false;
        }
    }
}