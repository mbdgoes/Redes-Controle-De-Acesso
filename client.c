#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#define BUFSIZE 500
static const int MAXPENDING = 5;

int main(int argc, char *argv[])
{
  if (argc < 3 || argc > 4) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)", "<Server Address> [<Server Port>]");
  struct sockaddr_storage storage;
  if (0 != addrparse(argv[1], argv[2], &storage))
  {
    DieWithUserMessage("Parameter(s)", "<Server Address> [<Server Port>]");
  }

  int sock = socket(storage.ss_family, SOCK_STREAM, 0);
  if (sock < 0)
    DieWithUserMessage("socket() failed", "unable to create socket");

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != connect(sock, addr, sizeof(storage)))
  {
    if (sock < 0)
      DieWithUserMessage("connect() failed", "unable to connect");
  }

  char addrstr[BUFSIZE];
  addrtostr(addr, addrstr, BUFSIZE);
  printf("connected to %s\n", addrstr);

  while (1)
  {
    char sentMessage[BUFSIZE];
    memset(sentMessage, 0, BUFSIZE);
    fgets(sentMessage, BUFSIZE - 1, stdin);
    size_t numBytes = send(sock, sentMessage, strlen(sentMessage), 0);

    if (numBytes < 0)
      DieWithSystemMessage("send() failed");
    else if (numBytes != strlen(sentMessage))
      DieWithUserMessage("send()", "sent unexpected number of bytes");

    char recvMessage[BUFSIZE];
    memset(recvMessage, 0, BUFSIZE);
    numBytes = recv(sock, recvMessage, BUFSIZE - 1, 0);
    if (numBytes < 0)
      DieWithSystemMessage("recv() failed");
    else if (numBytes == 0)
      DieWithUserMessage("recv()", "connection closed prematurely");

    // recvMessage[numBytes] = '\0';
    fputs(recvMessage, stdout);
    fputc('\n', stdout);
  }
  close(sock);
  exit(0);
}