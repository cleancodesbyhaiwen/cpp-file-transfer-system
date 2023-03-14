#pragma once

#include <vector>
#include <string>
#include <cstring>

#define CLIENT_BUFFER_SIZE 1024

class Client {
private: 
public:
    Client(const char* UDP_PORT, const char* TCP_PORT, const char* CLIENT_NAME) : UDP_PORT(atoi(UDP_PORT)), TCP_PORT(atoi(TCP_PORT))
    {
        this->CLIENT_NAME = (char*)malloc(strlen(CLIENT_NAME)+1);
        std::strncpy(this->CLIENT_NAME, CLIENT_NAME, strlen(CLIENT_NAME)+1);
        this->status = true;
    }
    ~Client()
    {
        free(this->CLIENT_NAME);
        free(this->dir);
    }
    void createSocket();
    void bindSocketToPort(sockaddr_in* client_addr, uint16_t PORT, int fd);
    void setServerAddr(const char* SERVER_IP, uint16_t SERVER_PORT);
    void sendUDPMessage(std::string message, sockaddr_in server_addr);
    void readFromUDPSocket(char* reply);
    void registerAccount();
    void setDir(const char* dir);
    void offerFile(std::string words);
    void displayTable();
    void handleServerResponse();
    void requestFile(std::string filename, std::string client_name);
    void handlePeerRequest();
    void changeStatus(std::string client_name, bool status);

    char* CLIENT_NAME;
    char* dir;
    sockaddr_in client_addr_udp;
    sockaddr_in client_addr_tcp;
    sockaddr_in server_addr;
    int client_fd_udp;
    int client_fd_tcp;
    uint16_t UDP_PORT;
    uint16_t TCP_PORT;
    std::string table;
    bool status;
    // For server use
    const char* CLIENT_IP;
    std::vector<std::string> filenames;
};




