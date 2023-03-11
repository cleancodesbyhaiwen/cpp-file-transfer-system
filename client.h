#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

class Client {
public:
    Client(char* UDP_PORT, char* TCP_PORT, char* CLIENT_NAME) : UDP_PORT(atoi(UDP_PORT)), TCP_PORT(atoi(TCP_PORT)), CLIENT_NAME(CLIENT_NAME)
    {
    }
    ~Client()
    {
    }
    void sendMessage(std::string message, int client_fd, sockaddr_in server_addr);
    void registerAccount();
    void createSocket();
    void setServerAddr(const char* SERVER_IP, uint16_t SERVER_PORT);
    void bindSocketToPort();
    std::string serialize();
    void readFromSocket();
    void deserialize(const std::string& s);
    const char* CLIENT_NAME;
    const char* CLIENT_IP;
    uint16_t UDP_PORT;
    uint16_t TCP_PORT;
    bool status;
    int client_fd;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
    char** file_names;
};

void Client::registerAccount()
{
    std::string message = "0";
    message += this->CLIENT_NAME;
    message += " ";
    message += std::to_string(this->TCP_PORT);
    sendMessage(message, this->client_fd, this->server_addr);
}
void Client::sendMessage(std::string message, int client_fd, sockaddr_in server_addr)
{
    ssize_t bytes_sent = sendto(client_fd, message.c_str(), message.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bytes_sent == -1) {
        std::cerr << "Failed to send data" << std::endl;
        close(client_fd);
        return;
    }
    return;
}

void Client::createSocket()
{
    // Create the socket
    this->client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    return;
}

void Client::setServerAddr(const char* SERVER_IP, uint16_t SERVER_PORT)
{
    // Set up the server address
    this->server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
}

void Client::bindSocketToPort()
{
    memset(&this->client_addr, 0, sizeof(this->client_addr));
    this->client_addr.sin_family = AF_INET;
    this->client_addr.sin_addr.s_addr = INADDR_ANY;
    this->client_addr.sin_port = htons(this->UDP_PORT);      
    if (bind(this->client_fd, (struct sockaddr*)&this->client_addr, sizeof(this->client_addr)) < 0) {
        std::cerr << "Failed to bind client socket to port\n";
        return;
    }    
}

void Client::readFromSocket()
{
    // Receive the response from the server
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    sockaddr_in server_response_addr;
    socklen_t server_response_addr_len = sizeof(server_response_addr);
    ssize_t bytes_received = recvfrom(this->client_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_response_addr, &server_response_addr_len);
    if (bytes_received == -1) {
        std::cerr << "Failed to receive data" << std::endl;
        close(this->client_fd);
        return;
    }
    std::cout << "Received " << bytes_received << " bytes from " << inet_ntoa(server_response_addr.sin_addr) << ":" << ntohs(server_response_addr.sin_port) << std::endl;
    std::cout << buffer << std::endl;    
}

