#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>

#define ANSWER_PORT 1067   
#define LISTEN_PORT  1068
#define BUFFER_LEN 2048
#define MAX_ATTEMPTS 5
#define TIMEOUT 2

struct dhcpMessage {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t clientIpAddr;
    uint32_t yourIpAddr;
    uint32_t serverIpAddr;
    uint32_t gatewayIpAddr;
    uint8_t clientHardwareAddr[16];
    char serverName[64];
    char bootFileName[128];
    uint8_t options[312];
};

int createSocket();
void configureServerAddress(struct sockaddr_in *serverAddress, int socketFd);
void configureClientAddress(struct sockaddr_in *clientAddress, int socketFd);
void sendDhcpDiscover(int socketFd, struct sockaddr_in *serverAddress, int addressLength);
int receiveDhcpOffer(int socketFd, struct sockaddr_in *clientAddress, int *addressLength, char *buffer);
void sendDhcpRequest(int socketFd, struct sockaddr_in *serverAddress);
int receiveDhcpAck(int socketFd, struct sockaddr_in *clientAddress, int *addressLength, char *buffer);
void getClientMacAddress(uint8_t *macAddress);
void printDhcpDetails(struct dhcpMessage *message);

extern unsigned char clientMac[16];  
extern uint32_t transactionIdClient;  
extern uint32_t requestedIp;

#endif
