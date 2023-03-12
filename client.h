#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

#include "helper.h"

#define CLIENT_BUFFER_SIZE 1024

class Client {
private: 
public:
    Client(const char* UDP_PORT, const char* TCP_PORT, const char* CLIENT_NAME) : UDP_PORT(atoi(UDP_PORT)), TCP_PORT(atoi(TCP_PORT))
    {
        this->CLIENT_NAME = (char*)malloc(sizeof(CLIENT_NAME));
        std::strncpy(this->CLIENT_NAME, CLIENT_NAME, sizeof(CLIENT_NAME));
    }
    ~Client()
    {
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
    void listenForUpdate();

    char* CLIENT_NAME;
    sockaddr_in client_addr_udp;
    sockaddr_in client_addr_tcp;
    int client_fd_udp;
    int client_fd_tcp;
    uint16_t UDP_PORT;
    uint16_t TCP_PORT;
    bool status;
    const char* CLIENT_IP;
    sockaddr_in server_addr;
    const char* dir;
    std::vector<std::string> filenames;
    std::string table;
};

// send registration request to server
void Client::registerAccount()
{
    std::string message = "registration " + std::string(this->CLIENT_NAME) + " " + std::to_string(this->TCP_PORT);
    sendUDPMessage(message.c_str(), this->server_addr);
    char reply[CLIENT_BUFFER_SIZE];
    this->readFromUDPSocket(reply);
    std::cout<<" >>> "<< reply <<std::endl;
    if(reply[0]=='S'){
        exit(1);
    }
}

// Send file offering request to server
void Client::offerFile(std::string words){
    if(this->dir==nullptr){
        std::cout<<" >>> Please set a dir for files to share before offering files"<<std::endl;
        return;
    }
    this->sendUDPMessage(words.c_str(), this->server_addr);
    // Accept ACK
    char reply[CLIENT_BUFFER_SIZE];
    this->readFromUDPSocket(reply);
    std::cout<<" >>> "<< reply <<std::endl;
    // Accept table
    char table[CLIENT_BUFFER_SIZE];
    this->readFromUDPSocket(table);
    this->table = std::string(table);
    std::cout<<" >>> [Client table updated.]"<<std::endl;
    // send ACK to server
    this->sendUDPMessage("Updated Table Received", this->server_addr);
}

// set the file offering directory
void Client::setDir(const char* dir)
{
    if(dir==nullptr){
        std::cout << " >>> [Usage: setdir [directory name].]" <<std::endl;
        return;
    }
    if(directoryExists(dir)) {
        this->dir = dir;
        std::cout << " >>> [Successfully set "<< dir <<" as the directory for searching offered files.]" << std::endl;
    } else {
        std::cout << " >>> [setdir failed: <dir> does not exist.]" <<std::endl;
    }
}

// display current version of file table
// table entry format: "client_name client_IP client_tcp_port file1 file2 ..."
void Client::displayTable()
{
    if(this->table.size() != 0){
        std::vector<std::string> entries;
        splitString(entries, this->table, '*');
        for(auto entry = entries.begin() + 1;entry != entries.end();entry++){
            std::vector<std::string> elements;
            splitString(elements, *entry, ' ');
            std::cout<<"================="<<std::endl;
            std::cout<<"Client Name: "<<elements[0]<<std::endl;
            std::cout<<"Client IP: "<<elements[1]<<std::endl;
            std::cout<<"Client Port: "<<elements[2]<<std::endl;
            std::cout<<"Client Files: ";
            for(int i =3;i < elements.size();i++){
                std::cout<<elements[i]<<" ";
            }
            std::cout<<std::endl;
        }
    }else{
        std::cout<<" >>> [No files available for download at the moment.]"<<std::endl;
    }
}

// Create the TCP and UDP socket
void Client::createSocket()
{
    // set timeout to 5ms
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000;
    setsockopt(this->client_fd_udp, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    this->client_fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd_udp == -1) {
        std::cerr << "Failed to create UDP socket" << std::endl;
        exit(1);
    }
    this->client_fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd_tcp == -1) {
        std::cerr << "Failed to create TCP socket" << std::endl;
        exit(1);
    }
}

// Bind the socket to the port
void Client::bindSocketToPort(sockaddr_in* client_addr, uint16_t PORT, int fd)
{
    memset(client_addr, 0, sizeof(*client_addr));
    client_addr->sin_family = AF_INET;
    client_addr->sin_addr.s_addr = INADDR_ANY;
    client_addr->sin_port = htons(PORT);      
    if (bind(fd, (struct sockaddr*)client_addr, sizeof(*client_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(fd);
        exit(1);
    }    
}

// construct the server addr
void Client::setServerAddr(const char* SERVER_IP, uint16_t SERVER_PORT)
{
    this->server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
}

void Client::sendUDPMessage(std::string message, sockaddr_in server_addr)
{
    ssize_t bytes_sent = sendto(this->client_fd_udp, message.c_str(), message.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bytes_sent == -1) {
        std::cerr << "Failed to send data" << std::endl;
        close(this->client_fd_udp);
        return;
    }
    return;
}

void Client::readFromUDPSocket(char* reply)
{
    char buffer[CLIENT_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    sockaddr_in server_response_addr;
    socklen_t server_response_addr_len = sizeof(server_response_addr);
    ssize_t bytes_received = recvfrom(this->client_fd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_response_addr, &server_response_addr_len);
    if (bytes_received == -1) {
        std::cerr << "Failed to receive data" << std::endl;
        close(this->client_fd_udp);
        exit(1);
    } 
    std::strcpy(reply, buffer);
}

void Client::listenForUpdate()
{
    char table[CLIENT_BUFFER_SIZE];
    while(true){
        memset(table, 0, sizeof(table));
        this->readFromUDPSocket(table);
        if(table[0]=='*'){
            this->table = std::string(table);
            std::cout<<" >>> [Client table updated.]"<<std::endl;
            // send ACK to server
            this->sendUDPMessage("Updated Table Received", this->server_addr);
        }
    }
}

