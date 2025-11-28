#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "common.h"

// 결과 반환용 구조체
typedef struct {
    int pfn;      // 찾은 물리 프레임 번호 (없으면 -1)
    bool hit;     // 최종 데이터 페이지가 메모리에 있었는지 여부
} PT_Result;

// Page Walk 수행 (va를 받아 VPN1,2,3 추출)
PT_Result walk_page_table(uint16_t va);

// [Error 수정] Page Table 업데이트 함수 선언 추가
void update_page_table(uint16_t va, int new_pfn);

// 스왑 아웃 시 매핑 끊기
void invalidate_pt_mapping(uint16_t vpn);

#endif