#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define ANSWER_PORT 1067   
#define LISTEN_PORT  1068
#define BUFFER_LEN 2048      /* Maximum size for the message buffer */
#define MAX_ATTEMPTS 5       /* Maximum number of DHCP discovery attempts */
#define TIMEOUT 2            /* Timeout duration (in seconds) between attempts */


int createSocket();
void configureServerAddress(struct sockaddr_in *server, int sockfd);
void configureClientAddress(struct sockaddr_in *client, int sockfd);
void sendDHCPDiscover(int sockfd, struct sockaddr_in *server, int addrLen);
int receiveDHCPOffer(int sockfd, struct sockaddr_in *client, int *addr_len, char *buf);
void sendDHCPRequest(int sockfd, struct sockaddr_in *server, int addrLen);
void receiveDHCPAck(int sockfd, struct sockaddr_in *client, int *addr_len, char *buf);
void getClientMACAddress(uint8_t *macAddr);

struct dhcp_message {
    uint8_t op;              // Operation code
    uint8_t htype;           // Hardware type
    uint8_t hlen;            // Hardware address length
    uint8_t hops;            // Hops (generalmente 0)
    uint32_t xid;            // Transaction ID
    uint16_t secs;           // Seconds elapsed
    uint16_t flags;          // Flags
    uint32_t ciaddr;         // Client IP address
    uint32_t yiaddr;         // 'Your' IP address
    uint32_t siaddr;         // Server IP address
    uint32_t giaddr;         // Gateway IP address
    uint8_t chaddr[16];      // Client hardware address
    char sname[64];          // Server name (optional)
    char file[128];          // Boot filename (optional)
    uint8_t options[312];    // DHCP options
};

int main() {
    int sockfd = createSocket();             /* Socket descriptor */
    struct sockaddr_in server,client;               /* Server address structure */
    int addr_len = sizeof(struct sockaddr);  /* Length of the server address */
    char buf[BUFFER_LEN];                    /* Buffer for incoming data */

    configureServerAddress(&server,sockfd);
    configureClientAddress(&client,sockfd);


    int attempts = 0;
    int received_offer = 0;

    while (attempts < MAX_ATTEMPTS && !received_offer) {
        printf("Sending DHCP Discover (attempt %d of %d)...\n", attempts + 1, MAX_ATTEMPTS);
        sendDHCPDiscover(sockfd, &server, addr_len);

        if (receiveDHCPOffer(sockfd, &client, &addr_len, buf)) {
            received_offer = 1;
        }


        if (!received_offer) {
            printf("No offer received, waiting %d seconds...\n", TIMEOUT);
            sleep(TIMEOUT);
        }

        attempts++;
    }

    if (!received_offer) {
        printf("No DHCP Offer received after %d attempts.\n", MAX_ATTEMPTS);
    }

    if (received_offer) {
        sendDHCPRequest(sockfd, &server, addr_len);
        receiveDHCPAck(sockfd, &client, &addr_len, buf);
    }

    close(sockfd);
    return 0;
}


int createSocket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(1);
    }
    return sockfd;
}

void configureServerAddress(struct sockaddr_in *server, int sockfd) {
    server->sin_family = AF_INET;                  /* Set address family to IPv4 */
    server->sin_port = htons(ANSWER_PORT);                  /* Server listens on port 67 */
    server->sin_addr.s_addr = inet_addr("255.255.255.255"); /* Broadcast address */
    memset(&(server->sin_zero), 0, 8);             /* Clear remaining structure fields */

    int broadcast_permission = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0) {
        perror("Error enabling broadcast option");
        exit(1);
    }
}

void configureClientAddress(struct sockaddr_in *client, int sockfd) {
    client->sin_family = AF_INET;                  /* Set address family to IPv4 */
    client->sin_port = htons(LISTEN_PORT);                  /* Client listens on port 68 */
    client->sin_addr.s_addr = INADDR_ANY;          /* Listen on any available interface */
    memset(&(client->sin_zero), 0, 8);             /* Clear remaining structure fields */

    /* Bind the socket to the client's address */
    if (bind(sockfd, (struct sockaddr *)client, sizeof(struct sockaddr_in)) == -1) {
        perror("bind failed");
        close(sockfd);
        exit(1);
    }
}


/* Function to send a DHCP Discover message */
void sendDHCPDiscover(int sockfd, struct sockaddr_in *server, int addrLen) {
    struct dhcp_message discover;
    discover.op = 1;                          
    discover.htype = 1;                       
    discover.hlen = 6;                        
    discover.hops = 0;                        
    discover.xid = htonl(rand());             
    discover.secs = 0;                        
    discover.flags = htons(0x8000);           
    discover.ciaddr = 0;                     
    discover.yiaddr = 0;                      
    discover.siaddr = 0;                      
    discover.giaddr = 0;                      

    /* Nos aseguramos de que esten en 0*/
    memset(discover.chaddr, 0, 16);          
    getClientMACAddress(discover.chaddr);      
 
    /* Como no hay no ponemos */
    memset(discover.sname, 0, sizeof(discover.sname));
    memset(discover.file, 0, sizeof(discover.file));


    unsigned char *options = discover.options;
    options[0] = 53;  
    options[1] = 1;   
    options[2] = 1;   


    //Subnet Mask
    options[3] = 1;   
    options[4] = 4;   
    options[5] = 255;
    options[6] = 255;
    options[7] = 255;
    options[8] = 0;  

    //Router/Gateway
    options[9] = 3;   
    options[10] = 4;  
    options[11] = 192;
    options[12] = 168;
    options[13] = 200;
    options[14] = 4;  

    // DNS
    options[15] = 6;  
    options[16] = 4;  
    options[17] = 8;
    options[18] = 8;
    options[19] = 8;
    options[20] = 8;  

    options[21] = 255; 

    if (sendto(sockfd, &discover, sizeof(discover), 0, (struct sockaddr *)server, addrLen) == -1) {
        perror("Failed to send DHCP Discover message");
        close(sockfd);
        exit(1);
    }
    printf("DHCP Discover sent to: %s:%d\n", inet_ntoa(server->sin_addr), ntohs(server->sin_port));
}

/* Function to receive a DHCP Offer message */
int receiveDHCPOffer(int sockfd, struct sockaddr_in *server, int *addr_len, char *buf) {
    ssize_t bytes_received = recvfrom(sockfd, buf, BUFFER_LEN, 0, (struct sockaddr *)server, (socklen_t *)addr_len);
    if (bytes_received >= 0) {
        printf("Received DHCP Offer from: %s\n", inet_ntoa(server->sin_addr));
        return 1;  /* Successfully received an offer */
    }
    perror("Failed to receive DHCP Offer");
    return 0;  /* Failed to receive an offer */
}

/* Function to send a DHCP Request message */
void sendDHCPRequest(int sockfd, struct sockaddr_in *server, int addrLen) {
    const char *request_message = "DHCP Request";
    if (sendto(sockfd, request_message, strlen(request_message), 0, (struct sockaddr *)server, addrLen) == -1) {
        perror("Failed to send DHCP Request");
        close(sockfd);
        exit(1);
    }
    printf("DHCP Request sent to: %s:%d\n", inet_ntoa(server->sin_addr), ntohs(server->sin_port));
}

/* Function to receive a DHCP Acknowledge message */
void receiveDHCPAck(int sockfd, struct sockaddr_in *client, int *addr_len, char *buf) {
    ssize_t bytes_received = recvfrom(sockfd, buf, BUFFER_LEN, 0, (struct sockaddr *)client, (socklen_t *)addr_len);
    if (bytes_received >= 0) {
        printf("Received DHCP Acknowledge from: %s\n", inet_ntoa(client->sin_addr));
    } else {
        perror("Failed to receive DHCP Acknowledge");
    }
}


void getClientMACAddress(uint8_t *macAddr) {
    char buffer[256];
    FILE *fp = popen("ifconfig -a | grep -i ether", "r");

    if (fp == NULL) {
        perror("popen failed");
        return;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Extraer la dirección MAC (formato: ether 00:1A:2B:3C:4D:5E)
        if (sscanf(buffer, "%*s %02x:%02x:%02x:%02x:%02x:%02x", 
            &macAddr[0], &macAddr[1], &macAddr[2], 
            &macAddr[3], &macAddr[4], &macAddr[5]) == 6) {
            break;  // MAC address encontrada
        }
    }
    fclose(fp);
}
