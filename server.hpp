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
    uint16_t SERVER_PORT;
    sockaddr_in server_addr;
    std::vector<Client*> clients;
    Server(char* SERVER_PORT) : SERVER_PORT(atoi(SERVER_PORT)) 
    {
    }
    ~Server()
    {
    }
    void createSocket();
    void bindSocketToPort();
    void sendMessage(std::string message, sockaddr_in server_addr);
    void handleRequest();
    bool handleRegistration(std::vector<std::string> content, char* CLIENT_IP, uint16_t UDP_PORT,sockaddr_in client_addr);
    bool handleFileOffer(std::vector<std::string> content, char* CLIENT_IP, uint16_t UDP_PORT);
    void sendTable(sockaddr_in client_addr);
    void broadcastTable();
};



