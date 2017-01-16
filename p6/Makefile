CC = g++
CFLAGS = -std=c++11 -Wall -Wextra -Wshadow -Wconversion -O2

all : server

server : main.o
	$(CC) main.o -lboost_system -lboost_thread -o server

main.o : main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean :
	rm -f server *.o
