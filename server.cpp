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
#include <chrono>
#include <ctime>

#include "client.hpp"
#include "helper.hpp"
#include "server.hpp"

/*
 *    This is the loop that constantly accpet and handle user request
 *    The user request can either be REGISTRATION or FILE OFFERING or CHANGE STATUS
 */
void Server::handleRequest()
{
    char buffer[SERVER_BUFFER_SIZE];
    while (true) 
    {
        memset(buffer, 0, sizeof(buffer));
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        ssize_t bytes_received = recvfrom(this->server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (bytes_received == -1) 
        {
            std::cerr << "Failed to receive data" << std::endl;
            close(this->server_fd);
            exit(1);
        }
        /*
         *    Convert string received to a vector of words, seperated by space
         *    Registration format: "registration client_name tcp_port"
         *    File offering format: "offer file1 file2 ..."
         *    Change Staus format: "status on/off client_name"
         */
        std::vector<std::string> request;
        splitString(request, std::string(buffer), ' ');
        // Registration
        if(request[0] == "registration") {
            if(this->handleRegistration(request,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),client_addr)){
                this->sendMessage("registration success", client_addr);
                this->sendTable(client_addr);
            }
            else{
                this->sendMessage("registration fail", client_addr);
            }
        }
        // Filer Offering
        else if(request[0] == "offer"){
            if(this->handleFileOffer(request,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port))){
                this->sendMessage("offer success", client_addr);
                this->broadcastTable();
            }
            else{
                this->sendMessage("offer fail", client_addr);
            }
        }
        // Change Status
        else if(request[0] == "status"){
            bool status = (request[1] == "off") ? false : true;
            if(handleStatusChange(request[2], status)){
                this->sendMessage("status " + request[1] + " success", client_addr);
                this->broadcastTable();
            }
            else{
                this->sendMessage("status change failed", client_addr);
            }
        }
    }
}


/*
 *    Add client to this->clients
 */
bool Server::handleRegistration(std::vector<std::string> request, char* client_ip, uint16_t client_udp_port,sockaddr_in client_addr)
{
    std::string client_name = request[1];
    std::string client_tcp_port = request[2];
    // if username already exist, return false
    for(const auto& client : this->clients){
        if(client->client_name==client_name) return false;
    }
    Client* client = new Client(std::to_string(client_udp_port).c_str(), client_tcp_port.c_str(), client_name.c_str());
    client->client_ip = client_ip;
    client->client_addr_udp = client_addr;
    this->clients.push_back(client);
    return true;
}

/*
 *    set client's staus to false
 */
bool Server::handleStatusChange(std::string client_name, bool status)
{
    for(const auto& client : this->clients){
        if(std::strcmp(client->client_name,client_name.c_str())==0){
            client->status = status;
            return true;
        } 
    }
    return false;
}

/*
 *    Add filenames offered by client to client->filenames 
 */
bool Server::handleFileOffer(std::vector<std::string> files, char* client_ip, uint16_t UDP_PORT)
{
    // If no registered client, return false
    if(this->clients.size() == 0){
        return false;
    }
    Client* client;
    for(const auto& c : this->clients){
        if(c->client_ip == client_ip && c->client_udp_port == UDP_PORT){
            client = c;
        }
    }
    // If no client match IP and udp port, return false
    if(client == nullptr){
        return false;
    }
    for(int i = 1;i < files.size();i++){
        if(std::find(client->filenames.begin(), client->filenames.end(), files[i])==client->filenames.end()){
            client->filenames.push_back(files[i]);
        }
    }
    return true;
}

/*
 *    send current version of table to a client
 *    example table: 
 *    "table *client1 ip port file1 file2 ... *client2 ip port file1 ... *15:24"
 */
void Server::sendTable(sockaddr_in client_addr)
{
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::gmtime(&time_now);
    std::string table = "table "; 
    for(const auto& client : this->clients){
        if(client->status==true && client->filenames.size() != 0){
            table += "*" + std::string(client->client_name) + " ";
            table += std::string(client->client_ip) + " " + std::to_string(client->client_tcp_port) + " ";
            for(const auto& file : client->filenames){
                table += (file + " ");
            }
        }
    }
    table += ("*" + std::to_string(local_time.tm_hour-4) + ":" + std::to_string(local_time.tm_min));
    sendMessage(table, client_addr);
}

/*
 *    send current version of table to all online clients
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
    this->server_addr.sin_port = htons(this->server_port);
    if (bind(this->server_fd, (struct sockaddr *)&this->server_addr, sizeof(this->server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(this->server_fd);
        exit(1);
    }    
}