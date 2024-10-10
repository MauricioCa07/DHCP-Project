#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define LISTEN_PORT 1067   
#define ANSWER_PORT 1068
#define BUFFER_LEN 1024

struct dhcpMessage {
    uint8_t op;
    uint8_t hardwareType;
    uint8_t hardwareLen;
    uint8_t hops;
    uint32_t transactionId;
    uint16_t seconds;
    uint16_t flags;
    uint32_t clientIpAddress;
    uint32_t yourIpAddress;
    uint32_t serverIpAddress;
    uint32_t gatewayIpAddress;
    uint8_t clientHardwareAddress[16];
    char serverName[64];
    char bootFileName[128];
    uint8_t options[312];
};

extern uint8_t clientMacAddress[16];
extern uint32_t clientTransactionId;
extern char* assignedIpAddress;
extern int ipAvailable;

int createSocket();
void setupServer(int socketFd, struct sockaddr_in *server);
void setupBroadcast(int socketFd, struct sockaddr_in *broadcast);
int receiveDhcpDiscover(int socketFd, struct sockaddr_in *client, int *addrLen, char *buffer);
char* assignIpAddress();
char* getServerIpAddress();
void sendDhcpOffer(int socketFd, struct sockaddr_in *client, int addrLen);
int receiveDhcpRequest(int socketFd, struct sockaddr_in *client, int *addrLen, char *buffer);
void sendDhcpAcknowledge(int socketFd, struct sockaddr_in *client, int addrLen);

#endif