#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

// --- 시스템 상수 ---
#define MEM_SIZE 1024
#define PAGE_SIZE 8
#define FRAME_SIZE 8
#define NUM_FRAMES (MEM_SIZE / FRAME_SIZE) // 128
#define TLB_SIZE 16

// --- 주소 비트 구조 (12-bit) ---
// | VPN1 (3) | VPN2 (3) | VPN3 (3) | Offset (3) |
#define MASK_3BIT 0x07

#define GET_VPN1(va)    (((va) >> 9) & MASK_3BIT)
#define GET_VPN2(va)    (((va) >> 6) & MASK_3BIT)
#define GET_VPN3(va)    (((va) >> 3) & MASK_3BIT)
#define GET_OFFSET(va)  ((va) & MASK_3BIT)
#define GET_FULL_VPN(va) ((va) >> 3)

// --- PTE 구조 (1 Byte) ---
// | Present (1) | PFN (7) |
#define PTE_PRESENT_MASK 0x80
#define PTE_PFN_MASK     0x7F

#define IS_PTE_PRESENT(pte) ((pte) & PTE_PRESENT_MASK)
#define GET_PTE_PFN(pte)    ((pte) & PTE_PFN_MASK)
#define CREATE_PTE(pfn)     ((uint8_t)(0x80 | ((pfn) & 0x7F))) // Present=1 설정

// --- 교체 정책 정의 ---
typedef enum {
    POLICY_RR,
    POLICY_LRU
} Policy;

// 전역 변수 선언 (main.c에 정의)
extern Policy g_policy;
extern uint64_t g_time; // LRU용 시뮬레이션 시간 (메모리 액세스 횟수)

#endif