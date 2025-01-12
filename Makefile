CC = gcc
CFLAGS  =

all: client server

client: common.o client.c
	$(CC) $(CFLAGS) common.o client.c -o ./client

server: common.o server.c
	$(CC) $(CFLAGS) common.o server.c -o ./server

common.o: common.h common.c
	$(CC) $(CFLAGS) -c common.c

clean:
	$(RM) ./client ./server *.o *~