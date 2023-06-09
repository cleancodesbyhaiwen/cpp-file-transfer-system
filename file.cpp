/*
 * Program Name: CPP File Sharing System
 * Author: Haiwen Zhang
 * Date Created: 2023/3/10
 * Last Modified: 2023/3/16
 * Description: 
 *         This program runs in two modes: server and client
 *         Server: The server accept registration request and file offerings from client
 *                 It maintains a table of files and its owner's IP and port number.
 *                 It broadcast to all clients registered whenever the table is updated.
 *         Client: The client can consult with the table and initiate file request to the peers
 *                 It also constantly waits for peers' requests and send the files
 * Permission: You are free to use this program. c  
 */
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <stdexcept>

#include "client.hpp"
#include "server.hpp"
#include "helper.hpp"

int main(int argc, char** argv)
{
    const char* MODE = determineMode(argv);
    if(std::strcmp(MODE, "SERVER")==0)
    {
        serverModeArgumentsCheck(argc,argv);
        Server* server = new Server(argv[2]);
        server->createSocket();
        server->bindSocketToPort();
        server->handleRequest();

        delete server;

        return 0;
    }
    else if(std::strcmp(MODE, "CLIENT")==0)
    {
        clientModeArgumentsCheck(argc, argv);
        Client* client = new Client(argv[5],argv[6], argv[2]);
        client->setServerAddr(argv[3], atoi(argv[4]));
        client->createSocket();
        client->bindSocketToPort(&client->client_addr_udp, client->client_udp_port,client->client_fd_udp);
        client->bindSocketToPort(&client->client_addr_tcp, client->client_tcp_port,client->client_fd_tcp);
        
        std::thread t1(&Client::handleServerResponse, client);
        std::thread t2(&Client::handlePeerRequest, client);

        client->registerAccount();

        while(true)
        {
            std::string command;
            std::getline(std::cin, command);
            std::vector<std::string> words;
            splitString(words, command, ' ');
            if(words[0]=="setdir"){
                client->setDir(words[1].c_str());
            }
            else if(words[0]=="offer"){
                client->offerFile(words);
            }
            else if(words[0]=="list"){
                client->displayTable();
            }
            else if(words[0]=="request"){
                try {
                    client->requestFile(words[1], words[2]);
                    if(words[2].size()==0) throw std::runtime_error("");
                }
                catch (std::exception& e) {
                    printMsg("Usage: request <filename> <client_name>", 's', 'w');
                }
            }
            else if(words[0]=="dereg"){
                try {
                    client->changeStatus(words[1], false);
                }
                catch (std::exception& e) {
                    printMsg("Usage: dereg <client_name>", 's', 'w');
                }
            }
            else if(words[0]=="back"){
                try {
                    client->changeStatus(words[1], true);
                }
                catch (std::exception& e) {
                    printMsg("Usage: dereg <client_name>", 's', 'w');
                }
            }
            else if(words[0]=="help"){
                displayCommandList();   
            }
            else{
                printMsg("Your command cannot be understand", 's', 'w');
            }
        }
        
        delete client;

        return 0;
    }
}
