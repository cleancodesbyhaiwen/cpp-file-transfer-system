#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "client.hpp"
#include "helper.hpp"

/*
 *   This is the function constantly listen on TCP welcome socket 
 *   and handle peer's file request
*/
void Client::handlePeerRequest()
{

    int new_socket, valread;
    char buffer[CLIENT_BUFFER_SIZE];
    int addrlen = sizeof(this->client_addr_tcp);
    memset(buffer, 0, sizeof(buffer));
    // Listen for incoming connections
    if (listen(this->client_fd_tcp, 3) < 0) {
        std::cerr << "Socket listening failed" << std::endl;
        exit(1);
    }
    // Listen for peer request if status is true
    while(this->status){
        // create scoket for serving file to peer
        if ((new_socket = accept(this->client_fd_tcp, (struct sockaddr *)&this->client_addr_tcp, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Socket accept failed" << std::endl;
            exit(1);
        }
        // getting the file name requested
        valread = read(new_socket, buffer, 1024);
        std::cout << "  < Received from client request for: " << buffer << " > "<<std::endl;
        std::string file_path = std::string(this->dir) + "/" + std::string(buffer);
        // If the requested file exists, send it to the peer
        if (fileExists(file_path)) {
            std::ifstream file(file_path, std::ios::binary);
            while (file.good()) {
                file.read(buffer, CLIENT_BUFFER_SIZE);
                int bytesRead = file.gcount();
                write(new_socket, buffer, bytesRead);
            }
            file.close();
        } else {
            const char* errorMsg = "File not found.";
            write(new_socket, errorMsg, strlen(errorMsg));
        }
        memset(buffer, 0, sizeof(buffer));
        // Close the socket
        close(new_socket);
        std::cout << ">>> ";
        std::cout << std::flush;
    } 

}

/*
 *   This function send file request to peer and receive file and store the file 
 *   to the same directory
*/
void Client::requestFile(std::string filename, std::string client_name)
{
    // create the socket for requesting and receiving file
    int new_socket= socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket == -1) {
        std::cerr << "Failed to create new socket." << std::endl;
        exit(1);
    }
    /*
     *    Resolving the peer IP and port given his/her client nanme
     *    Also check if the file is indeed offered by him/her 
    */
    uint16_t SERVER_PORT;
    const char* SERVER_IP;
    std::vector<std::string> words;

    splitString(words, this->table, ' ');
    auto result = std::find(words.begin(), words.end(), "*"+client_name);
    if (result != words.end()) {
        SERVER_IP = words[static_cast<int>(std::distance(words.begin(), result))+1].c_str();
        SERVER_PORT = std::stoi(words[static_cast<int>(std::distance(words.begin(), result))+2]);
    }
    else {
        std::cout << ">>> [The client name cannot be found in the table]" << std::endl;
        return;
    }

    // constructing the address of the serving peer using IP and port
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    // establish connection to the serving peer
    if (connect(new_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cout<< "\033[31m" <<"  < Connection Failed, pelase try again >"<<"\033[0m" <<std::endl;
        return;
    }
    std::cout<<"  < Connection with client "<<client_name<<" established. >"<<std::endl;
    
    // Sending the filename to the serving peer
    size_t index = filename.find('(');
    filename = filename.substr(0,index-1);
    send(new_socket, filename.c_str(), filename.size(), 0);
    // Receiving the requested file from another client
    int bytesReceived = 0;
    int valread;
    char buffer[CLIENT_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    std::ofstream file(filename, std::ios::binary);
    std::cout<<"  < Downloading "<<filename<<" ... >"<<std::endl;
    while ((valread = read(new_socket, buffer, CLIENT_BUFFER_SIZE)) > 0) {
        if(buffer[0]=='F'){
            std::cout << ">>> "<< "\033[31m"<<"[The server cannot find the file you requested]" << "\033[0m" <<std::endl;
            return;
        }
        file.write(buffer, valread);
        bytesReceived += valread;
    }
    file.close();
    if (bytesReceived == 0) {
        std::cout << ">>> [No response from peer or the file is empty.]" << std::endl;
    } else {
        std::cout << "\033[32m"<< "  < "<<filename<<"("<<bytesReceived<<"bytes)"<<" downloaded successfully! > "<<"\033[0m" <<std::endl;
    }
    // Close the socket
    close(new_socket);
    std::cout << "  < Connection with "<<client_name<<" closed. >" << std::endl;
}


/*
 *   This is a while loop that constantly listen for response from server
 *   1. registration success/fail 
 *   2. offer file success
 *   3. receive updated table
*/
void Client::handleServerResponse()
{
    char response[CLIENT_BUFFER_SIZE];
    while(true){
        memset(response, 0, sizeof(response));
        this->readFromUDPSocket(response);
        std::vector<std::string> words;
        splitString(words, response, ' ');
        if(words[0]=="registration"){
            if(words[1]=="success"){
                std::cout<<"[Welcome, you are registered!] "<<std::endl;
            }else if(words[1]=="fail"){
                std::cout<<" [Registration failed. Username taken] "<<std::endl;
            }
        }else if(words[0]=="offer"){
            if(words[1]=="success"){
                std::cout<<"[Offer Message received by Server.] "<<std::endl;
            }
        }else if(words[0]=="table"){
            this->table = std::string(response).substr(6,strlen(response)-strlen("table "));
            std::cout<<"[Client table updated.]"<<std::endl;
            // send ACK to server
            this->sendUDPMessage("Updated Table Received", this->server_addr);
        }else if(words[0]=="status"){
            if(words[2]=="success"){
                if(words[1]=="on"){
                    this->status = true;
                    std::cout<<"[Welcome back!] "<<std::endl;
                }else{
                    this->status = false;
                    std::cout<<"[You are Offline. Bye. To register back, ENTER back client_name] "<<std::endl;
                }
            }else{
                std::cout<<"[Status change failed. Please check if you entered the correct client name] "<<std::endl;
            }
        }
        std::cout << ">>> ";
        std::cout << std::flush;
    }
}

/*
 *   send registration request to server
*/
void Client::registerAccount()
{
    std::string message = "registration " + std::string(this->CLIENT_NAME) + " " + std::to_string(this->TCP_PORT);
    sendUDPMessage(message.c_str(), this->server_addr);
}

/*
 *   send registration request to server
*/
void Client::changeStatus(std::string client_name, bool status)
{
    if(client_name.size()==0){
        std::cout<<"[Usage: dereg/back client_name] "<<std::endl;
    }
    std::string message;
    if(status==false){
        message = "status off " + std::string(client_name);
    }else{
        message = "status on " + std::string(client_name);
    }
    sendUDPMessage(message.c_str(), this->server_addr);
}

/*
 *   Send file offering request to server
*/
void Client::offerFile(std::vector<std::string>& words){
    if(this->dir==nullptr){
        std::cout<<">>> [Please set a dir for files to share before offering files]"<<std::endl;
        return;
    }
    std::string file_path;
    std::string message = "offer";
    for(int i = 1;i < words.size();i++){
        file_path = std::string(this->dir) + "/" + std::string(words[i]);
        if (fileExists(file_path)){
            size_t file_size = getFileSize(file_path);
            message += " " + (words[i]) + "(" + std::to_string(file_size) + "bytes)";
        }else{
            std::cout<<">>> "<< "\033[31m"<<"["<<words[i]<<"> does not exist.]"<<"\033[0m" <<std::endl;
        }
    }
    if(message.size()>5){
        this->sendUDPMessage(message.c_str(), this->server_addr);
    }else{
        std::cout<<">>> [Nothing offered.]"<<std::endl;
    }
}

/*
 *   set the file offering directory
*/
void Client::setDir(const char* dir)
{
    if(dir==nullptr){
        std::cout << ">>> [Usage: setdir <directory name>.]" <<std::endl;
        return;
    }
    if(directoryExists(dir)) {
        this->dir = (char*)malloc(strlen(dir)+1);
        std::strncpy(this->dir, dir, strlen(dir)+1);
        std::cout << ">>> [Successfully set <"<< dir <<"> as the directory for searching offered files.]" << std::endl;
    } else {
        std::cout << ">>> [setdir failed: <dir> does not exist.]" <<std::endl;
    }
}

/*
 *   display current version of file table
 *   table format: "*haiwen 127.0.0.1 50013 h1.txt h2.txt *peter 127.0.0.1 50015 p1.txt"
 *   entries are seperated by "*", elements in entries are seperated by [SPACE]
*/
void Client::displayTable()
{
    if(this->table.size() != 0){
        int clients_total = 0;
        int files_total = 0;
        std::vector<std::string> entries;
        splitString(entries, this->table, '*');
        std::cout<<"=============================="<<std::endl;
        for(auto entry = entries.begin() + 1;entry != entries.end();entry++){
            clients_total++;
            std::vector<std::string> elements;
            splitString(elements, *entry, ' ');
            std::cout<<"|| Client Name:      "<<elements[0]<<std::endl;
            std::cout<<"|| Client IP:        "<<elements[1]<<std::endl;
            std::cout<<"|| Client Port:      "<<elements[2]<<std::endl;
            std::cout<<"|| Client Files:     ";
            for(int i =3;i < elements.size();i++){
                files_total++;
                std::cout<<elements[i]<<" ";
            }
            std::cout<<std::endl;
            std::cout<<"=============================="<<std::endl;
        }
        std::cout<<"Clients Total: "<<clients_total<<"  Files total: "<<files_total<<std::endl;
    }else{
        std::cout<<">>> [No files available for download at the moment.]"<<std::endl;
    }
}

/*
 *   Create the TCP and UDP socket
*/
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
    // Set socket options to reuse address and port
    int opt = 1;
    if (setsockopt(this->client_fd_tcp, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Set socket options failed" << std::endl;
        exit(1);
    }
}

/*
 *   Bind the socket to the port
*/
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

/*
 *   construct the server addr
*/
void Client::setServerAddr(const char* SERVER_IP, uint16_t SERVER_PORT)
{
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
