# 컴파일러 설정
CC = gcc
# 컴파일 옵션: 
# -Wall: 모든 경고 출력
# -g: 디버깅 정보 포함 (gdb 사용 가능)
# -I.: 현재 디렉토리(root)를 헤더 경로에 포함 (common.h 등)
# -I./components: components 폴더를 헤더 경로에 포함 (log.h, tlb.h 등)
CFLAGS = -Wall -g -I. -I./components

# 소스 파일 목록 자동 탐색
# 1. 메인 파일
MAIN_SRC = main.c
# 2. components 폴더 내의 모든 .c 파일 (log.c, tlb.c, memory.c, page_table.c, swap.c)
COMP_SRCS = $(wildcard components/*.c)

# 전체 소스 리스트 합치기
SRCS = $(MAIN_SRC) $(COMP_SRCS)

# 오브젝트 파일 목록 생성 (.c -> .o 변환)
OBJS = $(SRCS:.c=.o)

# 최종 실행 파일 이름
TARGET = simulator

# 기본 타겟 (make 입력 시 실행됨)
all: $(TARGET)

# 링크 단계: 모든 .o 파일들을 묶어서 실행 파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 컴파일 단계: 각 .c 파일을 .o 파일로 변환
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 정리 타겟 (make clean 입력 시 실행됨)
# 생성된 오브젝트 파일들과 실행 파일을 삭제
clean:
	rm -f $(OBJS) $(TARGET)

# 가짜 타겟 선언 (파일 이름과 겹치지 않게 함)
.PHONY: all clean