#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <arpa/inet.h>

struct action{
    int type;
    int coordinates[2];
    int board[4][4];
};

void DieWithUserMessage(const char *msg, const char *detail);
void DieWithSystemMessage(const char *msg);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
char *computeCommand(char *command, struct action *action);








#endif