import argparse
import random
import numpy as np

# 시스템 상수 정의
ADDRESS_BITS = 12
PAGE_SIZE = 8
OFFSET_BITS = 3  # log2(8) = 3
NUM_PAGES = 2 ** (ADDRESS_BITS - OFFSET_BITS) # 512 pages
TOTAL_ACCESSES = 10000 # 지시서 Slide 24: 10,000 memory accesses

def get_virtual_address(vpn):
    """VPN을 기반으로 랜덤한 Offset을 결합하여 가상 주소 생성"""
    offset = random.randint(0, PAGE_SIZE - 1)
    return (vpn << OFFSET_BITS) | offset

def generate_uniform():
    """Uniform 분포: 모든 페이지가 동일한 확률로 접근됨"""
    accesses = []
    for _ in range(TOTAL_ACCESSES):
        vpn = random.randint(0, NUM_PAGES - 1)
        accesses.append(get_virtual_address(vpn))
    return accesses

def generate_zipfian(s):
    """Zipfian 분포: 일부 페이지가 매우 빈번하게 접근됨"""
    # 1. 제타 분포(Zipf) 확률 계산
    # P(k) = (1/k^s) / H, where H = sum(1/n^s)
    ranks = np.arange(1, NUM_PAGES + 1)
    weights = 1 / (ranks ** s)
    weights /= weights.sum()  # 확률 정규화

    # 2. 페이지(VPN) 선택 (numpy.random.choice 사용)
    # VPN 0 ~ 511을 랜덤하게 섞어서 Rank를 부여 (특정 번호 편향 방지)
    vpns = np.arange(NUM_PAGES)
    np.random.shuffle(vpns)
    
    # 3. 확률 분포에 따라 VPN 선택
    chosen_vpns = np.random.choice(vpns, size=TOTAL_ACCESSES, p=weights)
    
    accesses = [get_virtual_address(vpn) for vpn in chosen_vpns]
    return accesses

def main():
    parser = argparse.ArgumentParser(description="Memory Access Trace Generator")
    parser.add_argument("-d", "--dist", choices=["uniform", "zipfian"], required=True, help="Distribution type")
    parser.add_argument("-s", "--s", type=float, default=0.0, help="Skew parameter for Zipfian")
    parser.add_argument("-o", "--output", default="test_input", help="Output file name")
    
    args = parser.parse_args()

    if args.dist == "uniform":
        traces = generate_uniform()
    elif args.dist == "zipfian":
        if args.s <= 0:
            print("Error: Zipfian distribution requires s > 0")
            return
        traces = generate_zipfian(args.s)

    # 파일 저장
    with open(args.output, "w") as f:
        f.write(f"{len(traces)}\n")
        for addr in traces:
            f.write(f"0x{addr:03x}\n")

    print(f"Generated {len(traces)} accesses in '{args.output}' ({args.dist}, s={args.s})")

if __name__ == "__main__":
    main()