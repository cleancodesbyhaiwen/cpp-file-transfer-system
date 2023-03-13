#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <cctype>
#include <algorithm>

#include "client.hpp"
#include "helper.hpp"
#include "server.hpp"

/*
    This is the loop that constantly accpet and handle user request
    The user request can either be REGISTRATION or FILE OFFERING
*/
void Server::handleRequest()
{
    char buffer[SERVER_BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        ssize_t bytes_received = recvfrom(this->server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (bytes_received == -1) {
            std::cerr << "Failed to receive data" << std::endl;
            close(this->server_fd);
            exit(1);
        }
        // Convert string received to a vector of words, seperated by space
        // Registration format: "registration client_name tcp_port"
        // File offering format: "offer file1 file2 ..."
        std::vector<std::string> request;
        splitString(request, std::string(buffer), ' ');
        // Registration
        if(request[0]=="registration") 
        {
            if(this->handleRegistration(request,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),client_addr))
            {
                this->sendMessage("registration success", client_addr);
                this->sendTable(client_addr);
            }else{
                this->sendMessage("registration fail", client_addr);
            }
        }
        // Filer Offering
        else if(request[0]=="offer")
        {
            if(this->handleFileOffer(request,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port))){
                this->sendMessage("offer success", client_addr);
                this->broadcastTable();
            }
        }
    }
}


/*
    Add client to this->clients
*/
bool Server::handleRegistration(std::vector<std::string> request, char* CLIENT_IP, uint16_t CLIENT_UDP_PORT,sockaddr_in client_addr)
{
    std::string CLIENT_NAME = request[1];
    std::string CLIENT_TCP_PORT = request[2];
    // if username already exist, return false
    for(const auto& client : this->clients){
        if(client->CLIENT_NAME==CLIENT_NAME) return false;
    }
    Client* client = new Client(std::to_string(CLIENT_UDP_PORT).c_str(), CLIENT_TCP_PORT.c_str(), CLIENT_NAME.c_str());
    client->status = true;
    client->CLIENT_IP = CLIENT_IP;
    client->client_addr_udp = client_addr;
    this->clients.push_back(client);
    std::cout<<"=================="<<std::endl;
    std::cout<<"Successfully Registered"<<std::endl;
    std::cout<<"Client Name: "<<client->CLIENT_NAME<<std::endl;
    std::cout<<"Client IP: "<<client->CLIENT_IP<<std::endl;
    std::cout<<"Client TCP PORT: "<<client->TCP_PORT<<std::endl;
    std::cout<<"Client UDP PORT: "<<client->UDP_PORT<<std::endl;
    std::cout<<"Client Status: "<<client->status<<std::endl;
    std::cout<<"=================="<<std::endl;
    return true;
}


/*
    Add filenames offered by client to client->filenames 
*/
bool Server::handleFileOffer(std::vector<std::string> request, char* CLIENT_IP, uint16_t UDP_PORT)
{
    Client* client;
    for(auto& c : this->clients){
        if(c->CLIENT_IP == CLIENT_IP && c->UDP_PORT == UDP_PORT){
            client = c;
        }
    }
    for(int i = 1;i < request.size();i++){
        if(std::find(client->filenames.begin(), client->filenames.end(), request[i])==client->filenames.end()){
            client->filenames.push_back(request[i]);
        }
    }
    return true;
}

/*
    send current version of table to a client
*/
void Server::sendTable(sockaddr_in client_addr)
{
    std::string table = "table "; 
    for(const auto& client : this->clients){
        if(client->filenames.size()!=0){
            table += "*" + std::string(client->CLIENT_NAME) + " ";
            table += std::string(client->CLIENT_IP) + " " + std::to_string(client->TCP_PORT) + " ";
        }
        for(const auto& file : client->filenames){
            table += (file + " ");
        }
    }
    sendMessage(table, client_addr);
}

/*
    send current version of table to all online clients
*/
void Server::broadcastTable()
{
    for(const auto& client : this->clients){
        if(client->status == true){
            this->sendTable(client->client_addr_udp);
        }
    }
}

void Server::sendMessage(std::string message, sockaddr_in client_addr)
{
    ssize_t bytes_sent = sendto(this->server_fd, message.c_str(), message.size(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    if (bytes_sent == -1) {
        std::cerr << "Failed to send data" << std::endl;
        close(this->server_fd);
        exit(1);
    }
}

void Server::createSocket()
{
    this->server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
}

void Server::bindSocketToPort()
{
    memset(&this->server_addr, 0, sizeof(this->server_addr));
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_addr.s_addr = INADDR_ANY;
    this->server_addr.sin_port = htons(this->SERVER_PORT);
    if (bind(this->server_fd, (struct sockaddr *)&this->server_addr, sizeof(this->server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(this->server_fd);
        exit(1);
    }    
}