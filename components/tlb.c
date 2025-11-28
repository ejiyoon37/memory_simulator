#include "tlb.h"
#include "log.h" // 제공된 로그 함수 사용
#include <stdio.h>
#include "../common.h"

// 전역 변수 선언
TLB_Entry tlb[TLB_SIZE];
int tlb_rr_idx = 0; // Round-Robin 교체 포인터 (0 ~ 15)

// 1. TLB 초기화
void init_tlb() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = false;
        tlb[i].vpn = 0;
        tlb[i].pfn = 0;
    }
    tlb_rr_idx = 0; // 포인터 초기화
}

// 2. TLB 검색 (Lookup)
// 반환값: Hit이면 PFN, Miss이면 -1
int search_tlb(uint16_t vpn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        // Valid하고 VPN이 일치하면 Hit!
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            log_tlb_hit(vpn, tlb[i].pfn); // [Log] TLB Hit
            
            // RR 정책에서는 Hit 되어도 포인터나 순서가 변하지 않음 (LRU와 차이점)
            return tlb[i].pfn;
        }
    }

    // 못 찾았으면 Miss
    log_tlb_miss(vpn); // [Log] TLB Miss
    return -1;
}

// 3. TLB 업데이트 (Replacement)
// RR 정책: 현재 포인터 위치의 엔트리를 덮어쓰고 포인터 증가
void update_tlb(uint16_t vpn, uint16_t pfn) {
    // 1. 현재 RR 포인터가 가리키는 위치에 덮어쓰기
    tlb[tlb_rr_idx].vpn = vpn;
    tlb[tlb_rr_idx].pfn = pfn;
    tlb[tlb_rr_idx].valid = true;

    // [Log] TLB Update
    log_tlb_update(vpn, pfn);

    // 2. RR 포인터 이동 (Circular Buffer)
    tlb_rr_idx = (tlb_rr_idx + 1) % TLB_SIZE;
}

void invalidate_tlb_by_vpn(uint16_t vpn) {
    // 16개 엔트리를 모두 검사
    for (int i = 0; i < TLB_SIZE; i++) {
        // 유효하고(valid), VPN이 일치한다면
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            tlb[i].valid = false;
            // 주의: 여기서 break를 하면 안 됩니다. 
            // 이론상 TLB에 중복 항목이 없어야 하지만, 안전하게 다 검사하거나
            // 구현에 따라 중복이 없음을 보장한다면 break 가능.
            // 여기서는 안전하게 break 없이 진행합니다.
        }
    }
    // 별도의 로그 출력은 지시서에 명시되지 않았으므로 하지 않음.
}