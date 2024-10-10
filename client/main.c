#include "dhcp_client.h"

int main() {
    int socketFd = createSocket();
    struct sockaddr_in serverAddress, clientAddress;
    int addressLength = sizeof(struct sockaddr);
    char buffer[BUFFER_LEN];

    configureServerAddress(&serverAddress, socketFd);
    configureClientAddress(&clientAddress, socketFd);

    int attempts = 0;
    int receivedOffer = 0;

    while (attempts < MAX_ATTEMPTS && !receivedOffer) {
        printf("Sending DHCP Discover (attempt %d of %d)...\n", attempts + 1, MAX_ATTEMPTS);
        sendDhcpDiscover(socketFd, &serverAddress, addressLength);

        if (receiveDhcpOffer(socketFd, &clientAddress, &addressLength, buffer)) {
            receivedOffer = 1;
        }

        if (!receivedOffer) {
            printf("No offer received, waiting %d seconds...\n", TIMEOUT);
            sleep(TIMEOUT);
        }

        attempts++;
    }

    if (!receivedOffer) {
        printf("No DHCP Offer received after %d attempts.\n", MAX_ATTEMPTS);
        close(socketFd);
        return 1;
    }

    sendDhcpRequest(socketFd, &serverAddress);

    if (receiveDhcpAck(socketFd, &clientAddress, &addressLength, buffer)) {
        struct dhcpMessage *ackMessage = (struct dhcpMessage *)buffer;
        printDhcpDetails(ackMessage);
    } else {
        printf("Failed to receive DHCP Acknowledge.\n");
    }

    close(socketFd);
    return 0;
}