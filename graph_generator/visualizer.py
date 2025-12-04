import matplotlib.pyplot as plt
import os
import argparse
from collections import Counter

# 실험 시나리오 이름 (파일 이름과 매칭)
SCENARIOS = ["uniform", "zipf_0.5", "zipf_1.0", "zipf_1.5"]
LABELS = ["Uniform", "Zipf (s=0.5)", "Zipf (s=1.0)", "Zipf (s=1.5)"]

def parse_access_frequency(input_file):
    """
    입력 파일(테스트 케이스)을 읽어 각 페이지(VPN)의 접근 횟수를 카운트합니다.
    """
    vpns = []
    try:
        with open(input_file, 'r') as f:
            lines = f.readlines()
            # 첫 줄(총 접근 횟수)은 건너뜀
            for line in lines[1:]:
                if line.strip():
                    # 16진수 문자열 -> 정수 변환
                    addr = int(line.strip(), 16)
                    # 12-bit 주소에서 하위 3-bit(Offset)를 제외하면 VPN
                    vpn = addr >> 3 
                    vpns.append(vpn)
    except FileNotFoundError:
        print(f"[Warning] Input file '{input_file}' not found.")
        return {}
    
    return Counter(vpns)

def parse_metrics(log_file):
    """
    시뮬레이터 출력 로그 파일을 읽어 성능 지표(TLB Miss, Page Fault)를 계산합니다.
    """
    stats = {
        "total_access": 0,
        "tlb_miss": 0,
        "pt_miss": 0 # Page Fault (Page Table Miss)
    }
    
    try:
        with open(log_file, 'r') as f:
            for line in f:
                if "Access VA:" in line:
                    stats["total_access"] += 1
                elif "TLB Miss:" in line:
                    stats["tlb_miss"] += 1
                elif "Page Table Miss:" in line:
                    stats["pt_miss"] += 1
    except FileNotFoundError:
        print(f"[Warning] Log file '{log_file}' not found.")
        return None

    # 결과가 없으면 0 리턴
    if stats["total_access"] == 0:
        return 0.0, 0.0

    # 퍼센트(%) 비율 계산
    tlb_miss_rate = (stats["tlb_miss"] / stats["total_access"]) * 100
    page_fault_rate = (stats["pt_miss"] / stats["total_access"]) * 100
    
    return tlb_miss_rate, page_fault_rate

def plot_access_frequency(input_dir):
    """
    1. 페이지 접근 빈도 그래프 생성 (4개 서브플롯)
    - X축: Page Rank (빈도수 순위)
    - Y축: Access Count
    """
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    fig.suptitle('Page Access Frequency by Distribution', fontsize=16)
    axes = axes.flatten()
    
    for i, scenario in enumerate(SCENARIOS):
        input_path = os.path.join(input_dir, f"input_{scenario}")
        counts = parse_access_frequency(input_path)
        
        if not counts:
            continue
            
        # 빈도수 내림차순 정렬 (Zipf 특성을 잘 보여주기 위함)
        y_values = sorted(counts.values(), reverse=True)
        x_values = range(len(y_values))
        
        axes[i].bar(x_values, y_values, width=1.0, color='royalblue')
        axes[i].set_title(LABELS[i])
        axes[i].set_xlabel('Page Rank')
        axes[i].set_ylabel('Access Count')
        axes[i].grid(axis='y', linestyle='--', alpha=0.7)

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig('access_frequency.png')
    print("Graph Saved: access_frequency.png")

def plot_performance_metrics(log_dir):
    """
    2. 성능 지표 비교 그래프 생성 (Bar Chart)
    - TLB Miss Rate와 Page Fault Rate 비교
    """
    tlb_miss_rates = []
    page_fault_rates = []
    
    for scenario in SCENARIOS:
        log_path = os.path.join(log_dir, f"output_{scenario}")
        res = parse_metrics(log_path)
        if res:
            tlb_miss_rates.append(res[0])
            page_fault_rates.append(res[1])
        else:
            tlb_miss_rates.append(0)
            page_fault_rates.append(0)
            
    # 바 차트 그리기
    x = range(len(SCENARIOS))
    width = 0.35
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    rects1 = ax.bar([p - width/2 for p in x], tlb_miss_rates, width, label='TLB Miss Rate (%)', color='skyblue')
    rects2 = ax.bar([p + width/2 for p in x], page_fault_rates, width, label='Page Fault Rate (%)', color='salmon')
    
    ax.set_ylabel('Rate (%)')
    ax.set_title('LRU Simulator Performance Comparison')
    ax.set_xticks(x)
    ax.set_xticklabels(LABELS)
    ax.legend()
    ax.grid(axis='y', linestyle='--', alpha=0.5)
    
    # 막대 위에 값 표시
    ax.bar_label(rects1, padding=3, fmt='%.1f')
    ax.bar_label(rects2, padding=3, fmt='%.1f')
    
    plt.tight_layout()
    plt.savefig('performance_comparison.png')
    print("Graph Saved: performance_comparison.png")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    # 입력 파일과 로그 파일이 있는 디렉토리 지정 (기본값: 현재 폴더)
    parser.add_argument("--input_dir", default=".", help="Directory containing input testcases")
    parser.add_argument("--log_dir", default=".", help="Directory containing simulator output logs")
    args = parser.parse_args()
    
    print("Generating Graphs...")
    plot_access_frequency(args.input_dir)
    plot_performance_metrics(args.log_dir)
    print("Done.")₩