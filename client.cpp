#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>

#include "client.hpp"
#include "helper.hpp"

bool response_received = false;
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
        printMsg("Received from client request for: "+std::string(buffer),'a','w');
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
        printMsg("The client name cannot be found in the table",'s','r');
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
        printMsg("Connection Failed, pelase try again",'a','r');
        return;
    }
    printMsg("Connection with "+client_name+" established",'a','g');
    
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
    printMsg("Downloading "+client_name+"...",'a','w');
    while ((valread = read(new_socket, buffer, CLIENT_BUFFER_SIZE)) > 0) {
        if(buffer[0]=='F'){
            printMsg("The server cannot find the file you requested",'s','r');
            return;
        }
        file.write(buffer, valread);
        bytesReceived += valread;
    }
    file.close();
    if (bytesReceived == 0) {
        printMsg("No response from peer or the file is empty.",'s','r');
    } else {
        printMsg(filename+"("+std::to_string(bytesReceived)+"bytes)"+" downloaded successfully!",'a','g');
    }
    // Close the socket
    close(new_socket);
    printMsg("Connection with "+client_name+" closed",'a','w');
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
            response_received = true;
            if(words[1]=="success"){
                printMsg("Welcome, you are registered!",'s','w');
                printMsg("You can enter <help> to see a list of commands", 's', 'w');
            }
            else if(words[1]=="fail"){
                printMsg("Registration failed. Username taken",'s','r');
                exit(1);
            }
        }
        else if(words[0]=="offer"){
            response_received = true;
            if(words[1]=="success"){
                printMsg("Offer Message received by Server.",'s','g');
            }
            else if(words[1]=="fail"){
                printMsg("Offer file failed, try register again",'s','r');
            }
        }
        else if(words[0]=="table"){
            this->table = std::string(response).substr(6,strlen(response)-strlen("table "));
            printMsg("Client table updated.",'s','g');
        }
        else if(words[0]=="status"){
            response_received = true;
            if(words[2]=="success"){
                if(words[1]=="on"){
                    this->status = true;
                    printMsg("Welcome back!",'s','w');
                }
                else{
                    this->status = false;
                    printMsg("You are Offline. Bye. To register back, use command: back client_name",'s','w');
                }
            }
            else{
                printMsg("Status change failed. Please check if you entered the correct client name. You might need to reregister if server was down",'s','r');
            }
        }
    }
}

/*
 *   send registration request to server
*/
void Client::registerAccount()
{
    std::string message = "registration " + std::string(this->client_name) + " " + std::to_string(this->client_tcp_port);
    if(!sendUDPMessage(message.c_str(), this->server_addr)){
        printMsg("The server is not responding. Please try again later.",'s','r');
        exit(1);
    }
}

/*
 *   send registration request to server
*/
void Client::changeStatus(std::string client_name, bool status)
{
    std::string message;
    if(status==false){
        message = "status off " + std::string(client_name);
    }else{
        message = "status on " + std::string(client_name);
    }
    response_received = false;
    if(!sendUDPMessage(message.c_str(), this->server_addr)){
        printMsg("The server is not responding. Please try again later. If the server is down, you need to reregister.",'s','r');
    }
}

/*
 *   Send file offering request to server
*/
void Client::offerFile(std::vector<std::string>& words){
    if(this->dir==nullptr){
        printMsg("Please set a dir for files to share before offering files.",'s','r');
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
            printMsg(words[i]+" does not exist." ,'s','r');
        }
    }
    if(message.size()>5){
        response_received = false;
        if(!sendUDPMessage(message.c_str(), this->server_addr)){
            printMsg("The server is not responding. Please try again later. If the server is down, you need to reregister.",'s','r');
        }
    }else{
        printMsg("Nothing offered.",'s','w');
    }
}

/*
 *   set the file offering directory
*/
void Client::setDir(const char* dir)
{
    if(dir==nullptr){
        printMsg("Usage: setdir <directory name>.",'s','w');
        return;
    }
    if(directoryExists(dir)) {
        this->dir = (char*)malloc(strlen(dir)+1);
        std::strncpy(this->dir, dir, strlen(dir)+1);
        printMsg("Successfully set "+std::string(dir)+" as the directory for searching offered files.",'s','g');
    } else {
        printMsg("setdir failed: <dir> does not exist.",'s','r');
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
        for(auto entry = entries.begin() + 1;entry != entries.end() - 1;entry++){
            clients_total++;
            std::vector<std::string> elements;
            splitString(elements, *entry, ' ');
            std::cout<<"|| Client Name:      "<<elements[0]<<std::endl;
            std::cout<<"|| Client IP:        "<<elements[1]<<std::endl;
            std::cout<<"|| Client Port:      "<<elements[2]<<std::endl;
            std::cout<<"|| Client Files:     ";
            for(int i = 3;i < elements.size();i++){
                files_total++;
                std::cout<<elements[i]<<" ";
            }
            std::cout<<std::endl;
            std::cout<<"=============================="<<std::endl;
        }
        std::cout<<"Clients Total: "<<clients_total<<"  Files total: "<<files_total<<" Updated At: "<<entries[entries.size()-1]<<std::endl;
    }else{
        printMsg("No files available for download at the moment.",'s','w');
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


/*
 *   send a message to server
 *   time out for response is 500ms, if no response in 500ms, return false 
*/
bool Client::sendUDPMessage(std::string message, sockaddr_in server_addr)
{    
    extern bool response_received;
    response_received = false;
    ssize_t bytes_sent = sendto(this->client_fd_udp, message.c_str(), message.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bytes_sent == -1) {
        std::cerr << "Failed to send data" << std::endl;
        close(this->client_fd_udp);
        return false;
    }
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::milliseconds(500);
    while (std::chrono::steady_clock::now() < end_time) {
        if(response_received==true){
            return true;
        }
    }
    return false;
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
