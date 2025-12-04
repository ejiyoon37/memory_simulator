/* swap.c */
#include "swap.h"
#include "memory.h"
#include "tlb.h"
#include "page_table.h"
#include "../common.h"
#include <stdio.h>

static int swap_rr_idx = 0; // RR Victim Pointer

// [LRU] 각 프레임의 마지막 접근 시간 기록용 배열
static uint64_t frame_last_access[NUM_FRAMES];

// [Internal] 비트마스크 읽기 헬퍼 (memory.c의 Helper와 유사 역할)
// 실제 구현은 memory.c의 physical_memory에 접근해야 하므로, 
// memory.h에 선언된 check_swappable 함수나 매커니즘을 사용하는 것이 좋으나,
// 여기서는 내부 static 함수로 구현합니다.
static bool check_swappable(int pfn) {
    int byte_idx = pfn / 8;
    int bit_idx = pfn % 8;
    return (physical_memory[byte_idx] >> bit_idx) & 1;
}

// [LRU] 프레임 접근 시간 갱신
void acknowledge_frame_access(int pfn) {
    if (pfn >= 0 && pfn < NUM_FRAMES) {
        frame_last_access[pfn] = g_time;
    }
}

int swap_out() {
    int victim_pfn = -1;

    if (g_policy == POLICY_RR) {
        // --- Round Robin Policy ---
        int checked_count = 0;
        while (checked_count < NUM_FRAMES) {
            int curr = swap_rr_idx;
            swap_rr_idx = (swap_rr_idx + 1) % NUM_FRAMES;
            checked_count++;

            if (check_swappable(curr)) {
                victim_pfn = curr;
                break;
            }
        }
    } 
    else if (g_policy == POLICY_LRU) {
        // --- LRU Policy ---
        // Swappable한 프레임 중 last_access_time이 가장 작은 프레임 탐색
        uint64_t min_time = UINT64_MAX;
        
        for (int i = 0; i < NUM_FRAMES; i++) {
            if (check_swappable(i)) {
                if (frame_last_access[i] < min_time) {
                    min_time = frame_last_access[i];
                    victim_pfn = i;
                }
            }
        }
    }

    if (victim_pfn == -1) {
        fprintf(stderr, "Error: No swappable frames found! Memory deadlock.\n");
        return -1;
    }

    // Victim 처리
    uint16_t victim_vpn = get_frame_owner(victim_pfn);
    invalidate_tlb_by_vpn(victim_vpn);
    invalidate_pt_mapping(victim_vpn);
    free_frame(victim_pfn);

    return victim_pfn;
}