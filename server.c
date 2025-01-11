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
        .otherPeerConnected = 0,
        .userServer = NULL,
        .locationServer = NULL
    };
    if (client_port == 50000) {
        peerConn.userServer = &userServer;
    }
    else if (client_port == 60000) {
        peerConn.locationServer = &locationServer;
    }

    // Try to establish peer connection first
    int peer_sock = establishPeerConnection("::1", peer_port, &peerConn);
    
    // Create peer server socket
    int peer_server_sock = -1;
    
    if (peer_sock < 0) {
        // Set up listening socket for peer connections
        peer_server_sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (peer_server_sock < 0) perror("Peer socket creation failed");
        
        int enableDualStack = 0;
        int enableReuse = 1;
        setsockopt(peer_server_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(enableReuse));
        setsockopt(peer_server_sock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));

        struct sockaddr_in6 server_addr = {0};
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_port = htons(peer_port);
        server_addr.sin6_addr = in6addr_any;

        int bindServer = bind(peer_server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if(bindServer < 0) perror("Peer bind failed");
        
        printf("No peer found, starting to listen...\n");
        listen(peer_server_sock, 1);

        // Accept initial peer connection
        struct sockaddr_in6 peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        peerConn.socket = accept(peer_server_sock, (struct sockaddr*)&peer_addr, &addr_len);
        if (peerConn.socket >= 0) {
            peerConn.isConnected = 1;
            peerConn.isInitiator = 0;
            
            pthread_t peer_thread;
            pthread_create(&peer_thread, NULL, handlePeerConnection, &peerConn);
            pthread_detach(peer_thread);
        }
    } else {
            peerConn.isConnected = 1;
            peerConn.isInitiator = 1;
            
            pthread_t peer_thread;
            pthread_create(&peer_thread, NULL, handlePeerConnection, &peerConn);
            pthread_detach(peer_thread);
        }

    // Set up client server socket
    int client_server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    int enableDualStack = 0;
    int enableReuse;
    setsockopt(client_server_sock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));
    setsockopt(client_server_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(enableReuse));
    
    struct sockaddr_in6 server_addr = {0};
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(client_port);
    server_addr.sin6_addr = in6addr_any; 

    if (bind(client_server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(client_server_sock, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Create thread for stdin handling
    pthread_t stdin_thread;
    pthread_create(&stdin_thread, NULL, handleServerStdin, &peerConn);
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
            
            if (!peerConn.isConnected && !peerConn.otherPeerConnected) {
                peerConn.socket = accept(peer_server_sock, (struct sockaddr*)&peer_addr, &addr_len);
                if (peerConn.socket >= 0) {
                    peerConn.isConnected = 1;
                    peerConn.isInitiator = 0;
                    peerConn.hasExchangedIds = 0;
                    
                    pthread_t peer_thread;
                    pthread_create(&peer_thread, NULL, handlePeerConnection, &peerConn);
                    pthread_detach(peer_thread);
                }
            } else{
                int temp_sock = accept(peer_server_sock, (struct sockaddr*)&peer_addr, &addr_len);
                if (temp_sock >= 0) {
                    Message rejectMsg;
                    setMessage(&rejectMsg, ERROR, "01");
                    send(temp_sock, &rejectMsg, sizeof(Message), 0);
                    close(temp_sock);
                }
            }       
        }

        // Check for client connections
        if (FD_ISSET(client_server_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_sock = accept(client_server_sock, (struct sockaddr*)&client_addr, &addr_len);
            
            if (client_sock >= 0) {
                int currentClientCount = 0;
                if(client_port == 50000) currentClientCount = userServer.clientCount;
                else currentClientCount = locationServer.clientCount;

                if (currentClientCount >= 10) {
                    printf("Client limit exceeded\n");
                    close(client_sock);
                    continue;
                }
                // Receive initial connection message
                Message initMsg;
                if (recv(client_sock, &initMsg, sizeof(Message), 0) <= 0) {
                    printf("Error receiving initial message\n");
                    close(client_sock);
                    continue;
                }

                // Extract location ID from initial message
                int locationId = -1;
                if (initMsg.type == REQ_CONN) {
                    locationId = atoi(initMsg.payload);
                }
                
                ClientThreadParams *params = malloc(sizeof(ClientThreadParams));
                params->client_sock = client_sock;
                params->peerConn = &peerConn;
                params->locationId = locationId;
                if (client_port == 50000) {  // User Server
                    params->userServer = &userServer;
                    params->locationServer = NULL;
                } else {  // Location Server
                    params->userServer = NULL;
                    params->locationServer = &locationServer;
                }

                // Process the initial connection message
                Message responseMsg;
                computeCommand(params->userServer, params->locationServer, &responseMsg, &initMsg);
                send(client_sock, &responseMsg, sizeof(Message), 0);

                pthread_t client_thread;
                if (pthread_create(&client_thread, NULL, handleBothServersToClientMessages, params) != 0) {
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