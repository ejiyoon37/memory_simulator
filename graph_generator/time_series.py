import matplotlib.pyplot as plt
import os
import argparse

# 실험 시나리오
SCENARIOS = ["uniform", "zipf_0.5", "zipf_1.0", "zipf_1.5"]
LABELS = ["Uniform", "Zipf (s=0.5)", "Zipf (s=1.0)", "Zipf (s=1.5)"]
COLORS = ['gray', 'orange', 'green', 'blue']

def get_cumulative_rates(log_file):
    """
    로그 파일을 읽어 시간(접근 횟수)에 따른 누적 TLB Miss Rate를 계산
    """
    total_access = 0
    tlb_miss_count = 0
    
    # 데이터 포인트 저장용 (너무 많으면 그래프가 느려지므로 100회 단위로 저장)
    x_data = []
    y_data = []
    
    try:
        with open(log_file, 'r') as f:
            for line in f:
                if "Access VA:" in line:
                    total_access += 1
                elif "TLB Miss:" in line:
                    tlb_miss_count += 1
                
                # 100회 접근마다 기록 (또는 매번 기록)
                if total_access > 0 and total_access % 100 == 0:
                    miss_rate = (tlb_miss_count / total_access) * 100
                    x_data.append(total_access)
                    y_data.append(miss_rate)
                    
    except FileNotFoundError:
        print(f"[Warning] Log file '{log_file}' not found.")
        return [], []
        
    return x_data, y_data

def plot_time_series(log_dir):
    plt.figure(figsize=(12, 8))
    
    for i, scenario in enumerate(SCENARIOS):
        log_path = os.path.join(log_dir, f"output_{scenario}")
        x, y = get_cumulative_rates(log_path)
        
        if x and y:
            plt.plot(x, y, label=LABELS[i], color=COLORS[i], linewidth=2)

    plt.title('Cumulative TLB Miss Rate over Time', fontsize=16)
    plt.xlabel('Number of Memory Accesses', fontsize=14)
    plt.ylabel('Cumulative TLB Miss Rate (%)', fontsize=14)
    plt.ylim(0, 100) # 0% ~ 100% 범위 고정
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend(fontsize=12)
    
    output_file = 'time_series_analysis.png'
    plt.savefig(output_file)
    print(f"Graph Saved: {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--log_dir", default=".", help="Directory containing simulator output logs")
    args = parser.parse_args()
    
    plot_time_series(args.log_dir)