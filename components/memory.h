/* memory.h */
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include "../common.h"

#define MEM_SIZE 1024
#define FRAME_SIZE 8

// 외부 참조 (물리 메모리 배열)
extern uint8_t physical_memory[MEM_SIZE];

// 초기화
void init_memory();

// 프레임 할당 요청
// vpn: 이 프레임을 사용할 가상 주소의 VPN (Page Table의 경우 무시 가능)
// is_swappable: 데이터 페이지면 true, 페이지 테이블이면 false
// 반환값: 성공 시 PFN, 실패(Full) 시 -1
int allocate_free_frame(uint16_t vpn, bool is_swappable);

// 특정 프레임의 데이터 접근 헬퍼
uint8_t* get_frame_ptr(int pfn);

// Reverse Mapping 확인 (Swap 모듈용)
uint16_t get_frame_owner(int pfn);

void free_frame(int pfn);

#endif