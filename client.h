#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <fstream>

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
    void listenForResponse();
    void requestFile(std::string filename, std::string client_name);
    void handleTCPConnection();

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

void Client::handleTCPConnection()
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Create a socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        exit(1);
    }

    // Set socket options to reuse address and port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Set socket options failed" << std::endl;
        exit(1);
    }

    // Bind the socket to a specific address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(this->TCP_PORT);
    std::cout << "Server listening on port " << this->TCP_PORT << std::endl;
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Socket binding failed" << std::endl;
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Socket listening failed" << std::endl;
        exit(1);
    }

    std::cout << "Server listening on port " << 8000 << std::endl;
 
    while(true){
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Socket accept failed" << std::endl;
            exit(0);
        }

        // Read data from the client
        valread = read(new_socket, buffer, 1024);
        std::cout << "Received from client: " << buffer << std::endl;

        std::ifstream file(buffer, std::ios::binary);
        if (file) {
            // If the requested file exists, send it to the other client
            while (file.good()) {
                file.read(buffer, CLIENT_BUFFER_SIZE);
                int bytesRead = file.gcount();
                write(new_socket, buffer, bytesRead);
            }
            file.close();
            const char* errorMsg = "File not found.";
            write(new_socket, errorMsg, strlen(errorMsg));
            std::cout << "11111111111 " << buffer << std::endl;
        } else {
            // If the requested file doesn't exist, send an error message
            const char* errorMsg = "File not found.";
            write(new_socket, errorMsg, strlen(errorMsg));
            std::cout << "222222222222 " << buffer << std::endl;
        }
        
        memset(buffer, 0, sizeof(buffer));
        // Close the socket
        close(new_socket);
    }   
}

void Client::requestFile(std::string filename, std::string client_name)
{
    int new_socket= socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        exit(1);
    }

    uint16_t SERVER_PORT;
    std::string SERVER_IP;
    std::vector<std::string> words;
    splitString(words, this->table, ' ');
    for(int i = 0;i < words.size();i++){
        if(words[i].substr(1,words[i].size()-1)==client_name){
            SERVER_IP = words[i+1].c_str();
            SERVER_PORT = std::stoi(words[i+2]);
            break;
        }
    }

    std::cout<<"IP is "<<SERVER_IP<<std::endl;
    std::cout<<"Port is "<<SERVER_PORT<<std::endl;
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(50013);

    // Connecting to the server
    if (connect(new_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cout<<"Connection Failed"<<std::endl;
        exit(1);
    }

    // Requesting files from other clients
    while (1) {
        // Sending the filename to the server to be forwarded to other clients
        send(new_socket, filename.c_str(), filename.size(), 0);
        
        // Receiving the requested file from another client
        int bytesReceived = 0;
        int valread;
        char buffer[CLIENT_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        std::ofstream file(filename, std::ios::binary);
        while ((valread = read(new_socket, buffer, CLIENT_BUFFER_SIZE)) > 0) {
            file.write(buffer, valread);
            bytesReceived += valread;
        }
        file.close();
        
        if (bytesReceived == 0) {
            // If no bytes were received, the other client either closed the connection or there was an error
            const char* errorMsg = "No response from peer.";
            std::cout << errorMsg << std::endl;
            exit(1);
        } else {
            std::cout << "File received: " << filename << " (" << bytesReceived << " bytes)" << std::endl;
            exit(1);
        }
    
        /*
        char buffer[1024] = {0};
        int bytes_received = recv(new_socket, buffer, 1024, 0);
        if (bytes_received < 0) {
            std::cerr << "Failed to receive message from server." << std::endl;
            exit(1);
        }

        std::cout << "Received message from server: " << buffer << std::endl;
        exit(1);
        */

    }
    // Close the socket
    close(new_socket);
}

// This is a while loop that constantly listen for response from server
// 1. registration success/fail 
// 2. offer file success
// 3. receive updated table
void Client::listenForResponse()
{
    char response[CLIENT_BUFFER_SIZE];
    while(true){
        memset(response, 0, sizeof(response));
        this->readFromUDPSocket(response);
        std::vector<std::string> words;
        splitString(words, response, ' ');
        if(words[0]=="registration"){
            if(words[1]=="success"){
                std::cout<<" Welcome, you are registered! "<<std::endl;
            }else if(words[1]=="fail"){
                std::cout<<" Registration failed. "<<std::endl;
            }
        }else if(words[0]=="offer"){
            if(words[1]=="success"){
                std::cout<<" Offer Message received by Server. "<<std::endl;
            }
        }else if(words[0]=="table"){
            this->table = std::string(response).substr(6,strlen(response)-strlen("table "));
            std::cout<<" [Client table updated.]"<<std::endl;
            // send ACK to server
            this->sendUDPMessage("Updated Table Received", this->server_addr);
        }
        std::cout << " >>> ";
        std::cout << std::flush;
    }
}

// send registration request to server
void Client::registerAccount()
{
    std::string message = "registration " + std::string(this->CLIENT_NAME) + " " + std::to_string(this->TCP_PORT);
    sendUDPMessage(message.c_str(), this->server_addr);
}

// Send file offering request to server
void Client::offerFile(std::string words){
    if(this->dir==nullptr){
        std::cout<<" >>> Please set a dir for files to share before offering files"<<std::endl;
        return;
    }
    this->sendUDPMessage(words.c_str(), this->server_addr);
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
            std::cout<<"===================="<<std::endl;
            std::cout<<"Client Name: "<<elements[0]<<std::endl;
            std::cout<<"Client IP: "<<elements[1]<<std::endl;
            std::cout<<"Client Port: "<<elements[2]<<std::endl;
            std::cout<<"Client Files: ";
            for(int i =3;i < elements.size();i++){
                std::cout<<elements[i]<<" ";
            }
            std::cout<<std::endl;
            std::cout<<"===================="<<std::endl;
        }
    }else{
        std::cout<<" >>> [No files available for download at the moment.]"<<std::endl;
    }
}

// Create the TCP and UDP socket
void Client::createSocket()
{
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



