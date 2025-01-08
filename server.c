#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "common.h"

void *handle_peer_connection(void *arg) {
    PeerConnection *peerConn = (PeerConnection *)arg;
    Message receivedMsg, sendMsg;
    fd_set current_sockets, ready_sockets;
    
    FD_ZERO(&current_sockets);
    FD_SET(peerConn->socket, &current_sockets);

    // If this is the connecting server (active side), send initial REQ_CONNPEER
    if (peerConn->isInitiator) {
        setMessage(&sendMsg, REQ_CONNPEER, "");
        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
    }

    while (1) {
        ready_sockets = current_sockets;
        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(peerConn->socket, &ready_sockets)) {
            int bytesReceived = recv(peerConn->socket, &receivedMsg, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                peerConn->isConnected = 0;
                close(peerConn->socket);
                printf("Peer disconnected\n");
                break;
            }

            switch(receivedMsg.type) {
                case REQ_CONNPEER: {
                    // Generate a random PID for ourselves
                    peerConn->myId = rand() % 1000;
                    char pidStr[10];
                    snprintf(pidStr, sizeof(pidStr), "%d", peerConn->myId);
                    
                    setMessage(&sendMsg, RES_CONNPEER, pidStr);
                    send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                    
                    printf("Peer %d connected\n", peerConn->myId);
                    break;
                }
                
                case RES_CONNPEER: {
                    // Store their ID and print it
                    peerConn->theirId = atoi(receivedMsg.payload);
                    printf("New Peer ID: %d\n", peerConn->theirId);
                    
                    // If we haven't sent our ID yet and we're the initiator
                    if (peerConn->isInitiator && !peerConn->hasExchangedIds) {
                        peerConn->myId = rand() % 1000;
                        char myPidStr[10];
                        snprintf(myPidStr, sizeof(myPidStr), "%d", peerConn->myId);
                        
                        setMessage(&sendMsg, RES_CONNPEER, myPidStr);
                        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                        
                        printf("Peer %d connected\n", peerConn->myId);
                        peerConn->hasExchangedIds = 1;
                    }
                    break;
                }

                 case REQ_DISCPEER: {
                    int requestingPeerId = atoi(receivedMsg.payload);
                    
                    if (requestingPeerId != peerConn->theirId) {
                        setMessage(&sendMsg, ERROR, "02");
                        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                    } else {
                        // Valid peer ID
                        setMessage(&sendMsg, OK, "01");
                        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                        printf("Peer %d disconnected\n", requestingPeerId);
                        
                        // Close connection and start listening
                        peerConn->isConnected = 0;
                        close(peerConn->socket);
                        printf("No peer found, starting to listen...\n");
                        return NULL;
                    }
                    break;
                }
                
                case ERROR: {
                    if (strcmp(receivedMsg.payload, "01") == 0) {
                        printf("Peer limit exceeded\n");
                        close(peerConn->socket);
                        return NULL;
                    }
                    if (strcmp(receivedMsg.payload, "02") == 0) {
                        printf("%s\n", returnErrorMessage(&receivedMsg));
                    }
                    break;
                    break;
                }
                
                case REQ_LOCREG:
                    // Print debug message as required in spec
                    char uid[11], locId[10];
                    sscanf(receivedMsg.payload, "%s %s", uid, locId);
                    printf("REQ_LOCREG %s %s\n", uid, locId);

                    // Handle location registration
                    LocationServer *locServer = &peerConn->locationServer;
                    int loc = atoi(locId);
                    int oldLoc = -1;
                    int userFound = 0;

                    // Search for existing user location
                    for(int i = 0; i < locServer->userCount; i++) {
                        if(strcmp(locServer->locationUserDatabase[i], uid) == 0) {
                            oldLoc = locServer->lastLocationSeen[i];
                            locServer->lastLocationSeen[i] = loc;
                            userFound = 1;
                            break;
                        }
                    }

                    // If user not found, add them
                    if(!userFound && locServer->userCount < MAX_USERS) {
                        strncpy(locServer->locationUserDatabase[locServer->userCount], uid, 10);
                        locServer->lastLocationSeen[locServer->userCount] = loc;
                        locServer->userCount++;
                    }

                    // Send response with old location
                    char response[BUFSIZE];
                    Message responseMsg;
                    snprintf(response, BUFSIZE, "%d", oldLoc);
                    setMessage(&responseMsg, RES_LOCREG, response);
                    send(peerConn->socket, &responseMsg, sizeof(Message), 0);
                    break;

                 case REQ_USRAUTH:
                    // Print debug message as required in spec
                    printf("REQ_USRAUTH %s\n", receivedMsg.payload);

                    // Check if user has special permissions
                    UserServer *usrServer = &peerConn->userServer;
                    int isSpecial = 0;
                    char *uuid = receivedMsg.payload;

                    for(int i = 0; i < usrServer->userCount; i++) {
                        if(strcmp(usrServer->userDatabase[i], uuid) == 0) {
                            isSpecial = usrServer->specialPermissions[i];
                            break;
                        }
                    }

                    // Send response with permission status
                    response[BUFSIZE];
                    snprintf(response, BUFSIZE, "%d", isSpecial);
                    setMessage(&responseMsg, RES_USRAUTH, response);
                    send(peerConn->socket, &responseMsg, sizeof(Message), 0);
                    break;
                
                case OK: {
                    if (strcmp(receivedMsg.payload, "01") == 0) {
                        printf("%s\n", returnOkMessage(&receivedMsg));
                        printf("Peer %d disconnected\n", peerConn->theirId);
                        close(peerConn->socket);
                        exit(1);
                    }
                    break;
                }

                default:
                    // Handle other message types if needed
                    break;
            }
        }
    }
}

void *handle_stdin(void *arg) {
    PeerConnection *peerConn = (PeerConnection *)arg;
    char input[BUFSIZE];
    Message sendMsg;

    while (1) {
        if (fgets(input, BUFSIZE - 1, stdin) == NULL) {
            continue;
        }
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "kill") == 0) {
            if (peerConn->isConnected) {
                char pidStr[10];
                snprintf(pidStr, sizeof(pidStr), "%d", peerConn->myId);
                setMessage(&sendMsg, REQ_DISCPEER, pidStr);
                send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                
                // Wait for response
                Message recvMsg;
                recv(peerConn->socket, &recvMsg, sizeof(Message), 0);
                
                if (recvMsg.type == OK && strcmp(recvMsg.payload, "01") == 0) {
                    printf("%s\n", returnOkMessage(&recvMsg));
                    printf("Peer %d disconnected\n", peerConn->theirId);
                    close(peerConn->socket);
                    exit(0);  // Exit immediately after successful disconnection
                } else if (recvMsg.type == ERROR) {
                    printf("%s\n", returnErrorMessage(&recvMsg));
                }
            }
        }
    }
    return NULL;
}

void *handle_client(void *arg) {
    ClientThreadParams *params = (ClientThreadParams *)arg;
    Message receivedMsg, responseMsg;
    
    while (1) {
        int bytesReceived = recv(params->client_sock, &receivedMsg, sizeof(Message), 0);
        if (bytesReceived <= 0) {
            printf("Client disconnected\n");
            break;
        }

        // Process client command
        computeCommand(params->userServer, params->locationServer, &responseMsg, &receivedMsg);
        
        // Send response to client
        send(params->client_sock, &responseMsg, sizeof(Message), 0);

        // Forward to peer if connected
        if (params->peerConn->isConnected) {
            send(params->peerConn->socket, &receivedMsg, sizeof(Message), 0);
        }

        if (receivedMsg.type == EXIT || receivedMsg.type == REQ_DISC) {
            if (receivedMsg.type == REQ_DISC) {
                // Wait for response to be sent before closing
                usleep(100000);  // Small delay to ensure response is sent
            }
            break;
        }
    }

    close(params->client_sock);
    free(params);
    return NULL;
}

int establish_peer_connection(const char* serverAddress, int port, PeerConnection *peerConn) {
    struct sockaddr_in server_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Try to connect
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
        peerConn->socket = sock;
        peerConn->isConnected = 1;
        peerConn->isInitiator = 1;  // Mark as the initiating peer
        peerConn->hasExchangedIds = 0;
        return sock;
    }

    close(sock);
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Parameters: <Server port> <Client port>\n");
        exit(EXIT_FAILURE);
    }

    int peer_port = atoi(argv[1]);  // 40000 for P2P
    int client_port = atoi(argv[2]); // 50000 or 60000 for clients
    
    // Initialize random seed
    srand(time(NULL));
    
    UserServer userServer = {.userCount = 0};
    LocationServer locationServer = {.userCount = 0};
    PeerConnection peerConn = {
        .peerId = -1,
        .socket = -1,
        .isConnected = 0,
        .port = peer_port,
        .isInitiator = 0,
        .hasExchangedIds = 0,
        .myId = -1,
        .theirId = -1,
        .userServer = {.userCount = 0},
        .locationServer = {.userCount = 0}
    };

    // Try to establish peer connection first
    int peer_sock = establish_peer_connection("127.0.0.1", peer_port, &peerConn);
    
    // Create peer server socket
    int peer_server_sock = -1;
    
    if (peer_sock < 0) {
        printf("No peer found, starting to listen...\n");
        
        // Set up listening socket for peer connections
        peer_server_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(peer_port),
            .sin_addr.s_addr = INADDR_ANY
        };

        bind(peer_server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        listen(peer_server_sock, 1);

        // Accept initial peer connection
        struct sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        peerConn.socket = accept(peer_server_sock, (struct sockaddr*)&peer_addr, &addr_len);
        if (peerConn.socket >= 0) {
            peerConn.isConnected = 1;
            peerConn.isInitiator = 0;
            
            pthread_t peer_thread;
            pthread_create(&peer_thread, NULL, handle_peer_connection, &peerConn);
            pthread_detach(peer_thread);
        }
    } else {
        peerConn.isConnected = 1;
        peerConn.isInitiator = 1;
        
        pthread_t peer_thread;
        pthread_create(&peer_thread, NULL, handle_peer_connection, &peerConn);
        pthread_detach(peer_thread);
    }

    // Set up client server socket
    int client_server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(client_port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(client_server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(client_server_sock, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening for clients on port %d...\n", client_port);

    // Create thread for stdin handling
    pthread_t stdin_thread;
    pthread_create(&stdin_thread, NULL, handle_stdin, &peerConn);
    pthread_detach(stdin_thread);

    // Set up fd_set for multiple socket handling
    fd_set master_fds;
    FD_ZERO(&master_fds);
    FD_SET(client_server_sock, &master_fds);
    int maxfd = client_server_sock;
    
    if (peer_server_sock != -1) {
        FD_SET(peer_server_sock, &master_fds);
        maxfd = (peer_server_sock > maxfd) ? peer_server_sock : maxfd;
    }

    // Handle connections in main thread
    while (1) {
        fd_set read_fds = master_fds;
        if (select(maxfd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }

        // Check for peer connections
        if (peer_server_sock != -1 && FD_ISSET(peer_server_sock, &read_fds)) {
            struct sockaddr_in peer_addr;
            socklen_t addr_len = sizeof(peer_addr);
            
            if (!peerConn.isConnected) {
                peerConn.socket = accept(peer_server_sock, (struct sockaddr*)&peer_addr, &addr_len);
                if (peerConn.socket >= 0) {
                    peerConn.isConnected = 1;
                    peerConn.isInitiator = 0;
                    peerConn.hasExchangedIds = 0;
                    
                    pthread_t peer_thread;
                    pthread_create(&peer_thread, NULL, handle_peer_connection, &peerConn);
                    pthread_detach(peer_thread);
                }
            }
        }

        // Check for client connections
        if (FD_ISSET(client_server_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_sock = accept(client_server_sock, (struct sockaddr*)&client_addr, &addr_len);
            
            if (client_sock >= 0) {
                printf("New client connected\n");
                
                ClientThreadParams *params = malloc(sizeof(ClientThreadParams));
                params->client_sock = client_sock;
                params->peerConn = &peerConn;
                params->userServer = &userServer;
                params->locationServer = &locationServer;

                pthread_t client_thread;
                if (pthread_create(&client_thread, NULL, handle_client, params) != 0) {
                    perror("Failed to create client thread");
                    free(params);
                    close(client_sock);
                } else {
                    pthread_detach(client_thread);
                }
            }
        }
    }

    // Cleanup
    if (peerConn.isConnected) {
        close(peerConn.socket);
    }
    if (peer_server_sock != -1) {
        close(peer_server_sock);
    }
    close(client_server_sock);

    return 0;
}