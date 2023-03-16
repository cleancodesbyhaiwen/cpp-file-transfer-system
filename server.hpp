#pragma once

#include <netinet/in.h>
#include <vector>
#include <string>

#include "client.hpp"

#define SERVER_BUFFER_SIZE 1024

class Server 
{
public:
    int server_fd;
    uint16_t server_port;
    sockaddr_in server_addr;
    std::vector<Client*> clients;
    Server(char* server_port) : server_port(atoi(server_port)) 
    {
    }
    ~Server()
    {
        close(this->server_fd);
    }
    void createSocket();
    void bindSocketToPort();
    void sendMessage(std::string message, sockaddr_in server_addr);
    void handleRequest();
    bool handleRegistration(std::vector<std::string> content, char* client_ip, uint16_t client_udp_port,sockaddr_in client_addr);
    bool handleStatusChange(std::string client_name, bool status);
    bool handleFileOffer(std::vector<std::string> content, char* client_ip, uint16_t client_udp_port);
    void sendTable(sockaddr_in client_addr);
    void broadcastTable();
};



