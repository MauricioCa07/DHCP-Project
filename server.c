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
#define BUFFER_LEN 1024      // Define buffer length for incoming messages
#define IP_AVAILABLE 2


// Function prototypes
int creatingSocket();
void setupServer(int sockfd , struct sockaddr_in *server);
void setupBroadcast(int sockfd ,struct sockaddr_in *broadcast);
void setupClient(char ipClient[16], struct sockaddr_in *clientAddr);
void validateMac();
void sendDHCPOffer(int sockfd, struct sockaddr_in *client, int addr_len);
void sendAcknowledge(int sockfd, struct sockaddr_in *client, int addr_len);
int receiveDHCPDiscover(int sockfd, struct sockaddr_in *client, int *addr_len, char *buf);
int receiveDHCPRequest(int sockfd, struct sockaddr_in *client, int *addr_len, char *buf);



struct dhcp_packet {
    uint8_t op;                // OpCode: 1 para BOOTREQUEST, 2 para BOOTREPLY
    uint8_t htype;             // Hardware Type: 1 para Ethernet
    uint8_t hlen;              // Hardware Address Length: usualmente 6 para Ethernet
    uint8_t hops;              // Hops: número de saltos
    uint32_t xid;              // Transaction ID: identificador de la transacción
    uint16_t secs;             // Seconds elapsed: segundos transcurridos desde el inicio
    uint16_t flags;            // Flags: opciones de control
    struct in_addr ciaddr;     // Client IP address: IP actual del cliente (0.0.0.0 si no tiene)
    struct in_addr yiaddr;     // Your IP address: la IP asignada al cliente por el servidor
    struct in_addr siaddr;     // Server IP address: dirección IP del servidor DHCP
    struct in_addr giaddr;     // Gateway IP address: dirección IP del relay (opcional)
    uint8_t chaddr[16];        // Client hardware address: dirección MAC del cliente
    char sname[64];            // Server name: nombre del servidor (opcional)
    char file[128];            // Boot file name: nombre del archivo de arranque (opcional)
    uint8_t options[312];      // Options: opciones DHCP (variable en longitud)
};

int main() {
    // Create the UDP socket
    int sockfd = creatingSocket();

    // Setup server and client addresses
    struct sockaddr_in server, client;
    int addr_len = sizeof(struct sockaddr_in); 
    char buf[BUFFER_LEN];

    // Setup server configurations
    setupServer(sockfd,&server);
    setupBroadcast(sockfd, &client);



    // Main loop to handle DHCP Discover and Request
    while (1) {
        // Receive DHCP Discover messages
        receiveDHCPDiscover(sockfd, &server, &addr_len, buf);
        // Send DHCP Offer to the client
        sendDHCPOffer(sockfd, &client, addr_len);

        /* aqui debo hacer el setup clietn para ponerle la ip del cliente que recibi*/


        // Receive DHCP Request messages
        if (receiveDHCPRequest(sockfd, &client, &addr_len, buf)) {
            // Send DHCP Acknowledgment to the client
            sendAcknowledge(sockfd, &client, addr_len);
        }
    }

    close(sockfd);
    return 0;
}

// Function to create a UDP socket
int creatingSocket() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    return sockfd;
}

// Function to setup server address structure
void setupServer(int sockfd , struct sockaddr_in *server) {
    server->sin_family = AF_INET;          // Set address family to IPv4
    server->sin_port = htons(LISTEN_PORT); // Set port number (network byte order)
    server->sin_addr.s_addr = INADDR_ANY; // Allow connections from any IP address
    memset(&(server->sin_zero), 0, 8);     // Zero out the remaining structure

    if (bind(sockfd, (struct sockaddr *)server, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
}

void setupBroadcast(int sockfd ,struct sockaddr_in *broadcast){
    broadcast->sin_family = AF_INET;               // Familia de direcciones IPv4
    broadcast->sin_port = htons(ANSWER_PORT);       // Puerto al que envías el OFFER
    broadcast->sin_addr.s_addr = inet_addr("255.255.255.255"); // Dirección de broadcast
    memset(&(broadcast->sin_zero), 0, 8); 

    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt");
    }

}

void setupClient( char ipClient[16], struct sockaddr_in *clientAddr) {
    clientAddr->sin_family = AF_INET;              
    clientAddr->sin_port = htons(ANSWER_PORT);     
    clientAddr->sin_addr.s_addr = inet_addr(ipClient); 
    memset(&(clientAddr->sin_zero), 0, 8); 
}


char* giveIP(){
    static char ip[16];
    if (IP_AVAILABLE < 253){
        sprintf(ip, "192.168.200.%d", IP_AVAILABLE);
        return ip;
    }

    return NULL;
}

int receiveDHCPDiscover(int sockfd, struct sockaddr_in *server, int *addr_len, char *buf) {
    if (recvfrom(sockfd, buf, BUFFER_LEN, 0, (struct sockaddr *)server, (socklen_t *)addr_len) >= 0) {
        printf("Received DHCP DISCOVER from: %s\n", inet_ntoa(server->sin_addr));
        struct dhcp_packet *dhcp_msg = (struct dhcp_packet *)buf;
        if(dhcp_msg->htype == 1){
            printf("Holiii");
        }
        return 1;
    }
    perror("recvfrom");
    return 0;
}


// Function to receive DHCP Request messages
int receiveDHCPRequest(int sockfd, struct sockaddr_in *client, int *addr_len, char *buf) {
    struct dhcp_packet offer;
    offer.op = 2;
    offer.htype = 1;
    offer.hlen = 6;
    offer.hops = 0;
    offer.xid = htonl(rand());
    offer.secs = 0;
    offer.flags = htons(0x8000);
    offer.ciaddr = 

    if (recvfrom(sockfd, buf, BUFFER_LEN, 0, (struct sockaddr *)client, (socklen_t *)addr_len) >= 0) {
        printf("Received DHCP REQUEST from: %s\n", inet_ntoa(client->sin_addr));
        return 1;
    }
    perror("recvfrom");
    return 0;
}

// Function to send a DHCP Offer message to the client
void sendDHCPOffer(int sockfd, struct sockaddr_in *client, int addr_len) {
    char offer_message[] = "DHCP Offer: Here is your offer"; 
    if (sendto(sockfd, offer_message, strlen(offer_message), 0, (struct sockaddr *)client, addr_len) == -1) {
        perror("sendto");
        close(sockfd);
        exit(1);
    }
    printf("Sent DHCP Offer to: %s:%d\n", inet_ntoa(client->sin_addr), ntohs(client->sin_port));
}

// Function to send a DHCP Acknowledgment to the client
void sendAcknowledge(int sockfd, struct sockaddr_in *client, int addr_len) {
    char ack_message[] = "IP allocated"; 
    if (sendto(sockfd, ack_message, strlen(ack_message), 0, (struct sockaddr *)client, addr_len) == -1) {
        perror("sendto");
        close(sockfd);
        exit(1);
    }
    printf("Sent DHCP Acknowledgment to: %s:%d\n\n", inet_ntoa(client->sin_addr), ntohs(client->sin_port));
}


