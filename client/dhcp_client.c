#include "dhcp_client.h"

unsigned char clientMac[16];  
uint32_t transactionIdClient;  
uint32_t requestedIp;

int createSocket() {                         
    int socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        perror("Socket creation failed");
        exit(1);
    }
    return socketFd;
}

void configureServerAddress(struct sockaddr_in *serverAddress, int socketFd) {
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(ANSWER_PORT);
    serverAddress->sin_addr.s_addr = inet_addr("255.255.255.255");
    memset(&(serverAddress->sin_zero), 0, 8);

    int broadcastPermission = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission)) < 0) {
        perror("Error enabling broadcast option");
        exit(1);
    }
}

void configureClientAddress(struct sockaddr_in *clientAddress, int socketFd) {
    clientAddress->sin_family = AF_INET;
    clientAddress->sin_port = htons(LISTEN_PORT);
    clientAddress->sin_addr.s_addr = INADDR_ANY;
    memset(&(clientAddress->sin_zero), 0, 8);

    if (bind(socketFd, (struct sockaddr *)clientAddress, sizeof(struct sockaddr_in)) == -1) {
        perror("bind failed");
        close(socketFd);
        exit(1);
    }     
}

void sendDhcpDiscover(int socketFd, struct sockaddr_in *serverAddress, int addressLength) {
    struct dhcpMessage message;
    memset(&message, 0, sizeof(message));
    message.op = 1;
    message.htype = 1;
    message.hlen = 6;
    message.hops = 0;
    message.xid = htonl(rand());
    message.flags = htons(0x8000);

    getClientMacAddress(message.clientHardwareAddr);
    transactionIdClient = message.xid;
    memcpy(clientMac, message.clientHardwareAddr, 6);

    unsigned char *options = message.options;
    options[0] = 53;
    options[1] = 1;
    options[2] = 1;
    options[3] = 255;

    if (sendto(socketFd, &message, sizeof(message), 0, (struct sockaddr *)serverAddress, addressLength) < 0) {
        perror("Failed to send DHCP Discover");
    } else {
        printf("DHCP Discover sent\n");
    }
}

int receiveDhcpOffer(int socketFd, struct sockaddr_in *serverAddress, int *addressLength, char *buffer) {
    if ((recvfrom(socketFd, buffer, BUFFER_LEN, 0, (struct sockaddr *)serverAddress, (socklen_t *)addressLength)) >= 0) {
        printf("Received DHCP Offer from: %s\n", inet_ntoa(serverAddress->sin_addr));
        struct dhcpMessage *message = (struct dhcpMessage *)buffer;
        if (message->op == 2 && message->xid == transactionIdClient && memcmp(message->clientHardwareAddr, clientMac, 6) == 0) {
            requestedIp = ntohl(message->yourIpAddr);
            return 1;
        }
    }
    perror("Failed to receive DHCP Offer");
    return 0;  
}

void sendDhcpRequest(int socketFd, struct sockaddr_in *serverAddress) {
    struct dhcpMessage requestMessage;
    memset(&requestMessage, 0, sizeof(requestMessage));
    requestMessage.op = 1;
    requestMessage.htype = 1;
    requestMessage.hlen = 6;
    requestMessage.xid = transactionIdClient;
    requestMessage.flags = htons(0x8000);
    requestMessage.yourIpAddr = htonl(requestedIp);
    memcpy(requestMessage.clientHardwareAddr, clientMac, 6);

    unsigned char *options = requestMessage.options;
    options[0] = 53;
    options[1] = 1;
    options[2] = 3;
    options[3] = 50;
    options[4] = 4;
    memcpy(&options[5], &requestMessage.yourIpAddr, 4);
    options[9] = 255;

    if (sendto(socketFd, &requestMessage, sizeof(requestMessage), 0, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) < 0) {
        perror("Failed to send DHCP Request");
    } else {
        printf("DHCP Request sent\n");
    }
}

int receiveDhcpAck(int socketFd, struct sockaddr_in *clientAddress, int *addressLength, char *buffer) {
    ssize_t bytesReceived = recvfrom(socketFd, buffer, BUFFER_LEN, 0, (struct sockaddr *)clientAddress, (socklen_t *)addressLength);
    if (bytesReceived >= 0) {
        printf("Received DHCP Acknowledge from: %s\n", inet_ntoa(clientAddress->sin_addr));
        return 1;
    } else {
        perror("Failed to receive DHCP Acknowledge");
        return 0;
    }
}

void getClientMacAddress(uint8_t *macAddress) {
    char buffer[256];
    FILE *fp = popen("ifconfig -a | grep -i ether", "r");
    if (fp == NULL) {
        perror("popen failed");
        return;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (sscanf(buffer, "%*s %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
                   &macAddress[0], &macAddress[1], &macAddress[2], 
                   &macAddress[3], &macAddress[4], &macAddress[5]) == 6) {
            break;
        }
    }
    fclose(fp);
}

void printDhcpDetails(struct dhcpMessage *message) {
    struct in_addr ipAddr;
    ipAddr.s_addr = ntohl(message->yourIpAddr);
    printf("Assigned IP Address: %s\n", inet_ntoa(ipAddr));

    unsigned char *options = message->options;
    int i = 0;

    while (i < 312) {
        switch (options[i]) {
            case 1:  // Subnet Mask
                if (options[i + 1] == 4) {
                    struct in_addr subnetMask;
                    memcpy(&subnetMask.s_addr, &options[i + 2], 4);
                    printf("Subnet Mask: %s\n", inet_ntoa(subnetMask));
                }
                i += 6; 
                break;
            case 6:  // DNS Server
                if (options[i + 1] == 4) {
                    struct in_addr dnsServer;
                    memcpy(&dnsServer.s_addr, &options[i + 2], 4);
                    printf("DNS Server: %s\n", inet_ntoa(dnsServer));
                }
                i += 6;
                break;
            case 255:  // End of options
                return;
            default:
                i += options[i + 1] + 2;
                break;
        }
    }
}