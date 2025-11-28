/* swap.c */
#include "swap.h"
#include "memory.h"
#include "tlb.h"
#include "page_table.h" // invalidate_pte_mapping 함수 필요
#include "../common.h"
#include <stdio.h>

// Round-Robin Victim Pointer
static int swap_rr_idx = 0;

// [Internal] 비트마스크 읽기 헬퍼
static bool check_swappable(int pfn) {
    int byte_idx = pfn / 8;
    int bit_idx = pfn % 8;
    // Frame 0, 1 영역 읽기
    return (physical_memory[byte_idx] >> bit_idx) & 1;
}

int swap_out() {
    int victim_pfn = -1;
    int checked_count = 0;

    // 1. Round-Robin 방식으로 "스왑 가능한" 프레임 탐색
    while (checked_count < NUM_FRAMES) {
        int curr = swap_rr_idx;

        // 다음 victim 후보로 인덱스 이동
        swap_rr_idx = (swap_rr_idx + 1) % NUM_FRAMES;
        checked_count++;

        // bitmask 상에서 swappable=1 인 프레임만 후보
        if (check_swappable(curr)) {
            victim_pfn = curr;
            break;
        }
    }

    // 스왑 가능한 프레임을 못 찾은 경우 (이론상 거의 안 나와야 함)
    if (victim_pfn == -1) {
        fprintf(stderr, "Error: No swappable frames found! Memory deadlock.\n");
        return -1;
    }

    // 2. Victim 프레임에 매핑된 VPN 조회
    uint16_t victim_vpn = get_frame_owner(victim_pfn);

    // 3. TLB / Page Table에서 해당 VPN 무효화
    invalidate_tlb_by_vpn(victim_vpn);   // TLB 엔트리 제거
    invalidate_pt_mapping(victim_vpn);   // Page Table present 비트 0

    // 4. 프레임을 free 상태로 변경 (다음 allocate_free_frame에서 재사용)
    free_frame(victim_pfn);

    // 5. 확보된 빈 프레임 번호 반환
    return victim_pfn;
}
