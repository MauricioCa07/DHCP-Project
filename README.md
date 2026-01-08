# DHCP-Project

Welcome to the **DHCP-Project**! This repository contains the implementation of a **Dynamic Host Configuration Protocol (DHCP)** for a NAT network. The project includes both a **client** and **server** written in **C**, providing functionalities to assign IP addresses dynamically to clients through UDP sockets.

## Project Overview

The **DHCP-Project** aims to simulate the behavior of the DHCP protocol in a controlled NAT network environment. The primary goal of the project is to provide:
- **IP Address Assignment**: Allocate IP addresses dynamically to clients.
- **Subnet Mask Configuration**: Set up subnet masks for clients to correctly communicate within the network.
- **DNS Server Assignment**: Configure Domain Name System (DNS) information for client requests.

This implementation is designed to run within a NAT environment, typically using **two VirtualBox virtual machines**, to simulate both **client** and **server** interaction.

The project demonstrates the working of DHCP in distributing network configuration parameters using **UDP sockets** for communication. Each time a client joins the network, it requests network configuration, and the server dynamically allocates an IP address that has not been previously assigned.

## Installation

Follow the steps below to get started with the DHCP-Project:

### Step 1: Clone the Repository

```bash
# Clone the repository
git clone https://github.com/MauricioCa07/DHCP-Project.git
```

### Step 2: Setting Up the Client

```bash
# Navigate to the client directory
cd client

# Compile the client
gcc -o dhcp_client main.c dhcp_client.c

# Run the client (use sudo for permissions)
sudo ./dhcp_client
```

### Step 3: Setting Up the Server

```bash
# Navigate to the server directory
cd ../server/

# Compile the server
gcc -o dhcp_server main.c dhcp_server.c

# Run the server (use sudo for permissions)
sudo ./dhcp_server
```

> **Note:** The client and server must run on two separate machines within the same NAT network. VirtualBox virtual machines are recommended for this purpose.

## Usage

Each time the client is executed, it connects to the DHCP server to request an IP address and obtain other network configuration parameters such as **subnet mask** and **DNS server information**. The server keeps track of allocated IP addresses to ensure each connected client receives a unique IP.

The DHCP process typically follows these steps:
1. **Client Request**: The client sends a DHCP discover message to identify available servers.
2. **Server Offer**: The server replies with an available IP offer.
3. **Client Acceptance**: The client accepts the offered IP by sending a DHCP request message.
4. **Server Acknowledgement**: The server confirms and assigns the IP address to the client.

This implementation follows these steps to provide clients with network configuration dynamically.

## Features
- **Dynamic IP Address Assignment**
- **Subnet Mask and DNS Assignment**
- **UDP Socket Communication** for DHCP messages

## Author
- **MauricioCa07** - Original creator of this project.

