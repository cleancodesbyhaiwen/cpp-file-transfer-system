#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <cctype>
#include <unordered_map>

#include "client.h"

class Server 
{
public:
    int server_fd;
    uint16_t SERVER_PORT;
    sockaddr_in server_addr;
    std::vector<Client*> clients;
    Server(char* SERVER_PORT) : SERVER_PORT(atoi(SERVER_PORT)) {std::cout<<this->SERVER_PORT<< std::endl;};
    void createSocket();
    std::unordered_map<std::string, std::vector<const char*>> file_directory;
    void registerClient(char buffer[], char* CLIENT_IP, uint16_t UDP_PORT);
    void bindSocketToPort();
    void readFromSocket();
};

void Server::registerClient(char buffer[], char* CLIENT_IP, uint16_t UDP_PORT)
{
    std::string CLIENT_NAME;
    std::string TCP_PORT;
    int index = -1;
    for(int i = 0;i < strlen(buffer);i++){
        if(isspace(buffer[i])){
            index = i;
        }
    }
    CLIENT_NAME.assign(buffer, 1, index - 1);
    TCP_PORT.assign(buffer, index + 1, strlen(buffer)-index);
    Client* client = new Client(const_cast<char*>(std::to_string(UDP_PORT).c_str()), const_cast<char*>(TCP_PORT.c_str()), const_cast<char*>(CLIENT_NAME.c_str()));
    client->status = true;
    client->CLIENT_IP = CLIENT_IP;
    this->clients.push_back(client);
    std::cout<<"=================="<<std::endl;
    std::cout<<"Successfully Registered"<<std::endl;
    std::cout<<"Client Name: "<<client->CLIENT_NAME<<std::endl;
    std::cout<<"Client IP: "<<client->CLIENT_IP<<std::endl;
    std::cout<<"Client TCP PORT: "<<client->TCP_PORT<<std::endl;
    std::cout<<"Client UDP PORT: "<<client->UDP_PORT<<std::endl;
    std::cout<<"Client Status: "<<client->status<<std::endl;
    std::cout<<"=================="<<std::endl;
}

void Server::createSocket()
{
    // Create the socket
    this->server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
}

void Server::readFromSocket()
{
    // Receive data and print it out
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        ssize_t bytes_received = recvfrom(this->server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (bytes_received == -1) {
            std::cerr << "Failed to receive data" << std::endl;
            close(this->server_fd);
            return;
        }
        std::cout << "Received " << bytes_received << " bytes from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;
        std::cout << buffer << std::endl;
        if(buffer[0]=='0') this->registerClient(buffer,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    }
}

void Server::bindSocketToPort()
{
    // Bind the socket to the port
    this->server_addr;
    memset(&this->server_addr, 0, sizeof(this->server_addr));
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_addr.s_addr = INADDR_ANY;
    this->server_addr.sin_port = htons(this->SERVER_PORT);
    std::cout<<htons(this->SERVER_PORT)<< std::endl;
    if (bind(this->server_fd, (struct sockaddr *)&this->server_addr, sizeof(this->server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(this->server_fd);
        return;
    }    
}