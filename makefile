CFLAGS = -fpic -coverage -lm -std=c99
CP=g++
CC=gcc

all: chatserve chatclient

chatserve: chatserve.o
	$(CP) $(CFLAGS) -o chatserve chatserve.o
chatserve.o: chatserve.cpp
	$(CP) $(CFLAGS) -c chatserve.cpp

chatclient: chatclient.o
	$(CC) $(CFLAGS) -o chatclient chatclient.o
chatclient.o: chatclient.c
	$(CC) $(CFLAGS) -c chatclient.c

clean:
	rm -f *.o chatserve.exe chatclient.exe chatserve chatclient *.gcov *.gcda *.gcno
