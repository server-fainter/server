# 변수 설정
CC = gcc
CFLAGS = -Wall -g $(shell pkg-config --cflags libwebsockets)
LDFLAGS = $(shell pkg-config --libs libwebsockets)
TARGET = server
SRCS = server.c
OBJS = $(SRCS:.c=.o)

# 기본 타겟
all: $(TARGET)

# 실행 파일 생성 규칙
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# 오브젝트 파일 생성 규칙
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 청소 규칙
clean:
	rm -f $(OBJS) $(TARGET)

# PHONY 타겟 선언
.PHONY: all clean
