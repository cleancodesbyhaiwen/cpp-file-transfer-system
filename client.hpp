#pragma once

#include <vector>
#include <string>
#include <cstring>

#define CLIENT_BUFFER_SIZE 1024

class Client {
private: 
public:
    Client(const char* client_udp_port, const char* client_tcp_port, const char* client_name) : client_udp_port(atoi(client_udp_port)), client_tcp_port(atoi(client_tcp_port))
    {
        this->client_name = (char*)malloc(strlen(client_name)+1);
        std::strncpy(this->client_name, client_name, strlen(client_name)+1);
        this->status = true;
    }
    ~Client()
    {
        free(this->client_name);
        free(this->dir);
        close(this->client_fd_udp);
        close(this->client_fd_tcp);
    }
    void createSocket();
    void bindSocketToPort(sockaddr_in* client_addr, uint16_t PORT, int fd);
    void setServerAddr(const char* SERVER_IP, uint16_t SERVER_PORT);
    bool sendUDPMessage(std::string message, sockaddr_in server_addr);
    void readFromUDPSocket(char* reply);
    void registerAccount();
    void setDir(const char* dir);
    void offerFile(std::vector<std::string>& words);
    void displayTable();
    void handleServerResponse();
    void requestFile(std::string filename, std::string client_name);
    void handlePeerRequest();
    void changeStatus(std::string client_name, bool status);

    char* client_name;
    char* dir;
    sockaddr_in client_addr_udp;
    sockaddr_in client_addr_tcp;
    sockaddr_in server_addr;
    int client_fd_udp;
    int client_fd_tcp;
    uint16_t client_udp_port;
    uint16_t client_tcp_port;
    std::string table;
    bool status;
    // For server use
    const char* client_ip;
    std::vector<std::string> filenames;
};




