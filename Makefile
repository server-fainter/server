all : server.c
	gcc -o server server.c -lwebsockets -lpthread

clean :
	rm server