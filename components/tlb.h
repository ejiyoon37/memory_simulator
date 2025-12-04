#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <stdbool.h>

// TLB 엔트리 구조체
typedef struct {
    uint16_t vpn;           // VPN (상위 9비트)
    uint16_t pfn;           // PFN (하위 7비트)
    bool valid;             // 유효 비트
    uint64_t last_access_time; // [LRU] 마지막 접근 시간
} TLB_Entry;

// TLB 관련 상수
#define TLB_SIZE 16

// 함수 프로토타입
void init_tlb();
int search_tlb(uint16_t vpn);
void update_tlb(uint16_t vpn, uint16_t pfn);
void invalidate_tlb_by_vpn(uint16_t vpn);

#endif