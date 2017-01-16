CC = g++
CFLAGS = -std=c++11 -Wall -Wextra -Wshadow -Wconversion -O2

all : server

server : main.o http.o
	$(CC) main.o http.o -lboost_system -lboost_thread -o server

main.o : main.cpp http.h
	$(CC) $(CFLAGS) -c main.cpp

http.o : http.cpp http.h
	$(CC) $(CFLAGS) -c http.cpp

clean :
	rm -f server *.o
