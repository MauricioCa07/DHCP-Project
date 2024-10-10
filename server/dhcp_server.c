#include "dhcp_server.h"

int createSocket() {
    int socketFd;
    if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    return socketFd;
}

void setupServer(int socketFd , struct sockaddr_in *server) {
    server->sin_family = AF_INET;
    server->sin_port = htons(LISTEN_PORT);
    server->sin_addr.s_addr = INADDR_ANY;
    memset(&(server->sin_zero), 0, 8);

    if (bind(socketFd, (struct sockaddr *)server, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
}

void setupBroadcast(int socketFd, struct sockaddr_in *broadcast){
    broadcast->sin_family = AF_INET;
    broadcast->sin_port = htons(ANSWER_PORT);
    broadcast->sin_addr.s_addr = inet_addr("255.255.255.255");
    memset(&(broadcast->sin_zero), 0, 8);

    int broadcastEnable = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("setsockopt");
    }
}

int receiveDhcpDiscover(int socketFd, struct sockaddr_in *client, int *addrLen, char *buffer) {
    if (recvfrom(socketFd, buffer, BUFFER_LEN, 0, (struct sockaddr *)client, (socklen_t *)addrLen) >= 0) {
        printf("Received DHCP DISCOVER from: %s\n", inet_ntoa(client->sin_addr));
        struct dhcpMessage *message = (struct dhcpMessage *)buffer;
        if (message->op == 1 && message->hardwareType == 1 && message->hardwareLen == 6 && message->clientIpAddress == 0){
            clientTransactionId = message->transactionId;
            memcpy(clientMacAddress, message->clientHardwareAddress, 16);
            return 1;
        }
        return 0;
    }
    perror("recvfrom");
    return 0;
}

char* assignIpAddress() {
    static char ip[16];
    if (ipAvailable < 253){
        sprintf(ip, "192.168.200.%d", ipAvailable);
        ipAvailable++;
        return ip;
    }
    return NULL;
}

char* getServerIpAddress() {
    FILE *fp;
    char buffer[16];
    char *ipAddress = malloc(16);

    fp = popen("ifconfig", "r");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, "inet ") != NULL) {
            sscanf(buffer, " inet %s", ipAddress);
            break;
        }
    }
    pclose(fp);

    return ipAddress;
}

void sendDhcpOffer(int socketFd, struct sockaddr_in *client, int addrLen) {
    struct dhcpMessage offerMessage;
    memset(&offerMessage, 0, sizeof(offerMessage)); 
    offerMessage.op = 2; 
    offerMessage.hardwareType = 1; 
    offerMessage.hardwareLen = 6; 
    offerMessage.transactionId = clientTransactionId; 
    offerMessage.flags = htons(0x8000);

    assignedIpAddress = assignIpAddress();
    offerMessage.yourIpAddress = htonl(inet_addr(assignedIpAddress));
    offerMessage.serverIpAddress = htonl(inet_addr(getServerIpAddress()));

    memcpy(offerMessage.clientHardwareAddress, clientMacAddress, 16);

    unsigned char *options = offerMessage.options;
    int index = 0;

    // DHCP Message Type - DHCPOFFER
    options[index++] = 53; // Option: DHCP Message Type
    options[index++] = 1;  // Length
    options[index++] = 2;  // Offer

    // Subnet Mask
    options[index++] = 1;  // Option: Subnet Mask
    options[index++] = 4;  // Length
    uint32_t subnetMask = htonl(0xFFFFFF00); // 255.255.255.0 in network format
    memcpy(&options[index], &subnetMask, 4);
    index += 4;

    // DNS Server
    options[index++] = 6;  // Option: DNS Server
    options[index++] = 4;  // Length
    uint32_t dns = htonl(134744072);  // DNS IP in network format
    memcpy(&options[index], &dns, 4);
    index += 4;

    // End option
    options[index++] = 255; // End of options

    if (sendto(socketFd, &offerMessage, sizeof(offerMessage), 0, (struct sockaddr *)client, addrLen) == -1) {
        perror("sendto");
        close(socketFd);
        exit(1);
    }
    printf("Sent DHCP Offer to: %s:%d\n", inet_ntoa(client->sin_addr), ntohs(client->sin_port));
}

int receiveDhcpRequest(int socketFd, struct sockaddr_in *client, int *addrLen, char *buffer) {
    if (recvfrom(socketFd, buffer, BUFFER_LEN, 0, (struct sockaddr *)client, (socklen_t *)addrLen) >= 0) {
        struct dhcpMessage *message = (struct dhcpMessage *)buffer;
        if (message->op == 1 && clientTransactionId == message->transactionId && message->hardwareType == 1 && message->hardwareLen == 6 ){
            printf("Received DHCP REQUEST from: %s\n", inet_ntoa(client->sin_addr));
            return 1;
        }
        return 0;
    }
    perror("recvfrom");
    return 0;
}

void sendDhcpAcknowledge(int socketFd, struct sockaddr_in *client, int addrLen) {
    struct dhcpMessage acknowledgeMessage;
    memset(&acknowledgeMessage, 0, sizeof(acknowledgeMessage)); 
    acknowledgeMessage.op = 2; 
    acknowledgeMessage.hardwareType = 1; 
    acknowledgeMessage.hardwareLen = 6; 
    acknowledgeMessage.transactionId = clientTransactionId; 
    acknowledgeMessage.flags = htons(0x8000);
    acknowledgeMessage.yourIpAddress = htonl(inet_addr(assignedIpAddress)); 
    acknowledgeMessage.serverIpAddress = htonl(inet_addr(getServerIpAddress()));

    memcpy(acknowledgeMessage.clientHardwareAddress, clientMacAddress, 16);

    unsigned char *options = acknowledgeMessage.options;
    int index = 0;

    // DHCP Message Type - ACK
    options[index++] = 53; // Option: DHCP Message Type
    options[index++] = 1;  // Length
    options[index++] = 5;  // Acknowledge

    // Subnet Mask
    options[index++] = 1;  // Option: Subnet Mask
    options[index++] = 4;  // Length
    uint32_t subnetMask = htonl(0xFFFFFF00); // 255.255.255.0 in network format
    memcpy(&options[index], &subnetMask, 4);
    index += 4;

    // DNS Server
    options[index++] = 6;  // Option: DNS Server
    options[index++] = 4;  // Length
    uint32_t dns = htonl(134744072);  // DNS IP in network format
    memcpy(&options[index], &dns, 4);
    index += 4;

    // End option
    options[index++] = 255; // End of options

    if (sendto(socketFd, &acknowledgeMessage, sizeof(acknowledgeMessage), 0, (struct sockaddr *)client, addrLen) == -1) {
        perror("sendto");
        close(socketFd);
        exit(1);
    }
    printf("Sent DHCP Acknowledgment to: %s:%d\n", inet_ntoa(client->sin_addr), ntohs(client->sin_port));
}