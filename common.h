#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct action {
	int type;
	int coordinates[2];
	int board[4][4];
};

struct gameSetup {
	int initialBoard[4][4];
	int currBoard[4][4];
};

void DieWithUserMessage(const char *msg, const char *detail);
void DieWithSystemMessage(const char *msg);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

void initializeBoard(struct gameSetup *gameSetup, const char *filename);
void computeCommand(struct action *action, struct action *receivedData, struct gameSetup *game);
void printBoard(int board[4][4]);
void fillBoard(int board[4][4], int num);

#endif