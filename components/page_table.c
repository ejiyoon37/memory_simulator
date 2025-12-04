#include "page_table.h"
#include "memory.h"
#include "log.h"
#include "swap.h" 

// 내부 헬퍼: 특정 테이블 프레임의 PTE 주소 반환
uint8_t* get_pte_ptr(int table_pfn, int index) {
    return &physical_memory[(table_pfn * FRAME_SIZE) + index];
}

// 내부 헬퍼: 새 테이블(PD or PT)을 위한 프레임 할당
// [Spec] Page table entries are never evicted. (Swappable = false)
int alloc_table_frame() {
    // 인자: vpn=0 (Dummy), is_swappable=false
    int pfn = allocate_free_frame(0, false); 
    
    if (pfn == -1) {
        // 메모리가 꽉 찼다면 스왑 수행 (Victim 선정 및 해제)
        // [Spec] If main memory is full, page swap should be occurred.
        pfn = swap_out(); 
        
        // 스왑으로 빈 공간이 생겼으므로 다시 할당 시도
        pfn = allocate_free_frame(0, false);
    }
    return pfn;
}

PT_Result walk_page_table(uint16_t va) {
    int vpn1 = GET_VPN1(va);
    int vpn2 = GET_VPN2(va);
    int vpn3 = GET_VPN3(va);

    PT_Result result;
    result.pfn = -1;
    result.hit = false;

    // 1. Root Page Table (PD1) - 항상 PFN 2
    int pd1_pfn = 2; 

    uint8_t* pte1 = get_pte_ptr(pd1_pfn, vpn1);

    // [수정] 탐색 중에는 할당하지 않음. 없으면 Miss.
    if (!IS_PTE_PRESENT(*pte1)) {
        log_pt_miss(GET_FULL_VPN(va));
        return result;
    }
    int pd2_pfn = GET_PTE_PFN(*pte1);

    // 2. Page Directory 2 (PD2)
    uint8_t* pte2 = get_pte_ptr(pd2_pfn, vpn2);

    // [수정] 탐색 중에는 할당하지 않음. 없으면 Miss.
    if (!IS_PTE_PRESENT(*pte2)) {
        log_pt_miss(GET_FULL_VPN(va));
        return result;
    }
    int pt_pfn = GET_PTE_PFN(*pte2);

    // 3. Page Table (Leaf)
    uint8_t* pte3 = get_pte_ptr(pt_pfn, vpn3);

    if (IS_PTE_PRESENT(*pte3)) {
        // [Log] Page Table Hit
        log_pt_hit(GET_FULL_VPN(va), GET_PTE_PFN(*pte3));
        result.pfn = GET_PTE_PFN(*pte3);
        result.hit = true;
    } else {
        // [Log] Page Table Miss
        log_pt_miss(GET_FULL_VPN(va));
        result.pfn = -1; 
        result.hit = false;
    }

    return result;
}

// Page Table에 최종 매핑 업데이트 (Swap In 후 호출)
void update_page_table(uint16_t va, int new_pfn) {
    int vpn1 = GET_VPN1(va);
    int vpn2 = GET_VPN2(va);
    int vpn3 = GET_VPN3(va);
    
    int pd1_pfn = 2;
    
    // 1. PD1 -> PD2 탐색 및 할당
    uint8_t* pte1 = get_pte_ptr(pd1_pfn, vpn1);
    if (!IS_PTE_PRESENT(*pte1)) {
        // [수정] 업데이트 시점에 테이블이 없으면 생성 (Lazy Allocation)
        int new_table_pfn = alloc_table_frame();
        *pte1 = CREATE_PTE(new_table_pfn);
    }
    int pd2_pfn = GET_PTE_PFN(*pte1);

    // 2. PD2 -> PT 탐색 및 할당
    uint8_t* pte2 = get_pte_ptr(pd2_pfn, vpn2);
    if (!IS_PTE_PRESENT(*pte2)) {
        // [수정] 업데이트 시점에 테이블이 없으면 생성
        int new_table_pfn = alloc_table_frame();
        *pte2 = CREATE_PTE(new_table_pfn);
    }
    int pt_pfn  = GET_PTE_PFN(*pte2);

    // 3. PT -> Data PFN 업데이트
    uint8_t* pte3 = get_pte_ptr(pt_pfn, vpn3);

    // Update
    *pte3 = CREATE_PTE(new_pfn);
    
    // [Log] Page Table Update
    log_pt_update(GET_FULL_VPN(va), new_pfn);
}

void invalidate_pt_mapping(uint16_t vpn) {
    // 주소 쪼개기 (매크로 사용을 위해 가상 주소 포맷으로 복원)
    uint16_t va_dummy = vpn << 3; 
    
    int vpn1 = GET_VPN1(va_dummy);
    int vpn2 = GET_VPN2(va_dummy);
    int vpn3 = GET_VPN3(va_dummy);

    // 1. Root Page Table (PD1) 접근 - PFN 2 고정
    int pd1_pfn = 2;
    uint8_t* pte1 = get_pte_ptr(pd1_pfn, vpn1);

    // PD1 엔트리 자체가 없으면 하위도 없으므로 종료
    if (!IS_PTE_PRESENT(*pte1)) return;
    
    int pd2_pfn = GET_PTE_PFN(*pte1);

    // 2. Page Directory 2 (PD2) 접근
    uint8_t* pte2 = get_pte_ptr(pd2_pfn, vpn2);

    if (!IS_PTE_PRESENT(*pte2)) return;

    int pt_pfn = GET_PTE_PFN(*pte2);

    // 3. Leaf Page Table (PT) 접근
    uint8_t* pte3 = get_pte_ptr(pt_pfn, vpn3);

    // [핵심] 최종 PTE가 존재한다면 Present 비트만 끄기
    if (IS_PTE_PRESENT(*pte3)) {
        *pte3 &= ~PTE_PRESENT_MASK; 
    }
}