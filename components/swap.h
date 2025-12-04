/* swap.h */
#ifndef SWAP_H
#define SWAP_H

#include <stdint.h>

// 메모리가 부족할 때 Victim을 선정하고 스왑 아웃 수행
int swap_out();

// [LRU] 프레임 접근 시 시간 기록
void acknowledge_frame_access(int pfn);

#endif