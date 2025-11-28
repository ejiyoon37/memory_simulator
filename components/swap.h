/* swap.h */
#ifndef SWAP_H
#define SWAP_H

#include <stdint.h>

// 메모리가 부족할 때 Victim을 선정하고 스왑 아웃 수행
// 반환값: 확보된 빈 프레임 번호 (PFN)
int swap_out();

#endif