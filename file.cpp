/*
 * Program Name: CPP File Sharing System
 * Author: Haiwen Zhang
 * Date Created: 2023/3/10
 * Description: 
 *         This program runs in two modes: server and client
 *         Server: The server accept registration request and file offerings from client
 *                 It maintains a table of files and its owner's IP and port number.
 *                 It broadcast to all clients registered whenever the table is updated.
 *         Client: The client can consult with the table and initiate file request to the peers
 *                 It also constantly waits for peers' requests and send the files
 * Permission: You are free to use this program.
 */
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>

#include "client.hpp"
#include "server.hpp"
#include "helper.hpp"

int main(int argc, char** argv)
{
    const char* MODE = determineMode(argv);
    if(std::strcmp(MODE, "SERVER")==0)
    {
        std::cout<<"You are in **SERVER** mode"<<std::endl;

        serverModeArgumentsCheck(argc,argv);
        Server* server = new Server(argv[2]);
        server->createSocket();
        server->bindSocketToPort();
        server->handleRequest();

        // close socket and free memory
        close(server->server_fd);
        delete server;

        return 0;
    }
    else if(std::strcmp(MODE, "CLIENT")==0)
    {
        std::cout<<"You are in **CLIENT** mode"<<std::endl;

        clientModeArgumentsCheck(argc, argv);
        Client* client = new Client(argv[5],argv[6], argv[2]);
        client->setServerAddr(argv[3], atoi(argv[4]));
        client->createSocket();
        client->bindSocketToPort(&client->client_addr_udp, client->UDP_PORT,client->client_fd_udp);
        client->bindSocketToPort(&client->client_addr_tcp, client->TCP_PORT,client->client_fd_tcp);
        client->registerAccount();

        std::thread t1(&Client::handleServerResponse, client);
        std::thread t2(&Client::handlePeerRequest, client);

        while(true)
        {
            std::string command;
            std::cout << " >>> ";
            std::getline(std::cin, command);
            std::vector<std::string> words;
            splitString(words, command, ' ');
            if(words[0]=="setdir"){
                client->setDir(words[1].c_str());
            }
            else if(words[0]=="offer"){
                client->offerFile(command);
            }
            else if(words[0]=="list"){
                client->displayTable();
            }else if(words[0]=="request"){
                try {
                    client->requestFile(words[1], words[2]);
                }
                catch (std::exception& e) {
                    std::cout << " >>> Usage: request filename client_name" << std::endl;
                }
            }else if(words[0]=="dereg"){
                client->changeStatus(words[1], false);
            }else if(words[0]=="back"){
                client->changeStatus(words[1], true);
            }else{
                std::cout<<" >>> Your command cannot be understand"<<std::endl;
            }
        }
        // Close the sockets and free memory
        close(client->client_fd_udp);
        close(client->client_fd_tcp);
        delete client;

        return 0;
    }
}