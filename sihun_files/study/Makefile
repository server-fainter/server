# 컴파일러
CC = gcc

# 컴파일 플래그
CFLAGS = -Wall -O2 -I/usr/include/libwebsockets
LDFLAGS = -L/usr/lib -lwebsockets

# 실행 파일 이름
TARGET = server

# 소스 파일
SRC = server.c

# 빌드 대상
all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 정리
clean:
	rm -f $(TARGET)

# 실행
run: $(TARGET)
	./$(TARGET)
