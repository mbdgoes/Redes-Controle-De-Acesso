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
#include <sys/select.h>

//================ CONSTANTES ====================
#define BUFSIZE       500
#define MAX_USERS     30

#define EXIT          -1
#define REQ_CONNPEER  17
#define RES_CONNPEER  18
#define REQ_DISCPEER  19
#define REQ_CONN      20
#define RES_CONN      21
#define REQ_DISC      22

#define REQ_USRADD    33
#define REQ_USRACCESS 34
#define RES_USRACCESS 35
#define REQ_LOCREG    36
#define RES_LOCREG    37
#define REQ_USRLOC    38
#define RES_USRLOC    39
#define REQ_LOCLIST   40
#define RES_LOCLIST   41
#define REQ_USRAUTH   42
#define RES_USRAUTH   43

#define ERROR         255
#define OK            0

//============ STRUCTS =============================
typedef struct Message {
    int type;          
    size_t size;
    char payload[BUFSIZE]; // Payload fixo (máximo de 500 bytes)
} Message;

typedef struct UserServer {
    char userDatabase[100][11];
    int specialPermissions[100];
    int userCount;
    int clientCount;
} UserServer;

typedef struct LocationServer {
    char locationUserDatabase[100][11];
    int lastLocationSeen[100];
    int userCount;
    int clientCount;
} LocationServer;

typedef struct {
    int peerId;
    int socket;
    int isConnected;
    int port;
    int isInitiator;       
    int hasExchangedIds;   
    int myId;              
    int theirId;           
    int otherPeerConnected;
    UserServer *userServer;
    LocationServer *locationServer;
} PeerConnection;

typedef struct {
    int peerCount;
    PeerConnection peer;
} ServerState;

typedef struct {
    int client_sock;
    PeerConnection *peerConn;
    UserServer *userServer;
    LocationServer *locationServer;
    int locationId;
} ClientThreadParams;

typedef struct {
    int clientId;
    int locationId;
    int isInitialized;     // Flag to track if client has received IDs from both servers
    int hasUserServerId;   // Flag to track if received ID from user server
    int hasLocServerId;    // Flag to track if received ID from location server
} ClientState;


//============ FUNCOES DE REDE =======================
int initServerSockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);
int addrParse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void setMessage(Message *message, int type, char* payload);
char* returnErrorMessage(Message *message);
char* returnOkMessage(Message *message);

//============= FUNCOES DE CLIENTE ====================
void initializeClient(ClientState *state, int locationId);
void computeInput(Message *sentMessage, char command[BUFSIZE], int* error, int clientId);
void handleConnectionResponse(Message *message, ClientState *state, int serverType);
void handleReceivedData(struct Message* receivedData, int sock, int serverType);
int validateLocationId(int locationId);

//============= FUNCOES DE SERVIDOR  ==================
void findUser(LocationServer *locationServer, Message *message, char* userId);
int establishPeerConnection(const char* serverAddress, int port, PeerConnection *peerConn);
void *handlePeerConnection(void *arg);
void *handleServerStdin(void *arg);
void *handleBothServersToClientMessages(void *arg);
void computeCommand(UserServer *userServer, LocationServer *LocationServer, Message *action, Message *receivedData);

#endif