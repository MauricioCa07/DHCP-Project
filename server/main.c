#include "dhcp_server.h"

uint8_t clientMacAddress[16]; 
uint32_t clientTransactionId;
char* assignedIpAddress;
int ipAvailable = 2;

int main() {
    int socketFd = createSocket();
    struct sockaddr_in server, client;
    int addrLen = sizeof(struct sockaddr_in); 
    char buffer[BUFFER_LEN];

    setupServer(socketFd, &server);
    setupBroadcast(socketFd, &client);

    while (1) {
        if (!receiveDhcpDiscover(socketFd, &server, &addrLen, buffer)){
            continue;
        }
        sendDhcpOffer(socketFd, &client, addrLen);

        if (receiveDhcpRequest(socketFd, &client, &addrLen, buffer)) {
            sendDhcpAcknowledge(socketFd, &client, addrLen);
        }
    }

    close(socketFd);
    return 0;
}