/* memory.c */
#include "memory.h"
#include <string.h>
#include <stdio.h>

uint8_t physical_memory[MEM_SIZE];

// [Reverse Mapping] PFN -> VPN 매핑 정보 저장
// Swap Out 시 해당 VPN의 PTE/TLB를 무효화하기 위함
static uint16_t frame_owner_vpn[NUM_FRAMES];

// [Internal] 비트마스크 조작 (물리 메모리 0~15번지 직접 액세스)
// 0: Non-swappable, 1: Swappable
static void set_swappable_bit(int pfn, bool swappable) {
    int byte_idx = pfn / 8; // 0 or 1 (since pfn 0~127 -> 16 bytes)
    int bit_idx = pfn % 8;
    
    // 비트마스크는 Frame 0, 1에 위치함
    uint8_t *mask_byte = &physical_memory[byte_idx];
    
    if (swappable) {
        *mask_byte |= (1 << bit_idx);
    } else {
        *mask_byte &= ~(1 << bit_idx);
    }
}


static bool frame_allocated[NUM_FRAMES];

void init_memory() {
    memset(physical_memory, 0, MEM_SIZE);
    memset(frame_allocated, 0, sizeof(frame_allocated));
    memset(frame_owner_vpn, 0, sizeof(frame_owner_vpn));

    // [Spec] Frame 0, 1: Bitmask 저장용 (Allocated, Non-swappable)
    frame_allocated[0] = true; set_swappable_bit(0, false);
    frame_allocated[1] = true; set_swappable_bit(1, false);

    // [Spec] Frame 2: Root Page Directory (Allocated, Non-swappable)
    frame_allocated[2] = true; set_swappable_bit(2, false);
}

int allocate_free_frame(uint16_t vpn, bool is_swappable) {
    // 1. 빈 프레임 탐색 (Frame 3부터 시작)
    for (int i = 3; i < NUM_FRAMES; i++) {
        if (!frame_allocated[i]) {
            frame_allocated[i] = true;
            set_swappable_bit(i, is_swappable);
            frame_owner_vpn[i] = vpn; // 소유주 등록
            
            // 메모리 0으로 초기화
            memset(&physical_memory[i * FRAME_SIZE], 0, FRAME_SIZE);
            return i;
        }
    }
    return -1; // Memory Full
}

uint8_t* get_frame_ptr(int pfn) {
    return &physical_memory[pfn * FRAME_SIZE];
}

uint16_t get_frame_owner(int pfn) {
    return frame_owner_vpn[pfn];
}

void free_frame(int pfn) {
    if (pfn >= 0 && pfn < NUM_FRAMES) {
        frame_allocated[pfn] = false;
    }
}