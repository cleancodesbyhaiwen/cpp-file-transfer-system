#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>

#include "client.h"
#include "server.h"
#include "helper.h"


int main(int argc, char** argv)
{
    const char* MODE = determineMode(argv);
    if(std::strcmp(MODE, "SERVER")==0)
    {
        serverModeArgumentsCheck(argc,argv);
        std::cout<<"You are in **SERVER** mode"<<std::endl;
        Server* server = new Server(argv[2]);
        server->createSocket();
        server->bindSocketToPort();
        server->handleRequest();
        close(server->server_fd);
        return 0;
    }
    else if(std::strcmp(MODE, "CLIENT")==0)
    {
        clientModeArgumentsCheck(argc, argv);
        Client* client = new Client(argv[5],argv[6], argv[2]);
        client->setServerAddr(argv[3], atoi(argv[4]));

        std::cout<<"You are in **CLIENT** mode"<<std::endl;
        std::cout<<"====================="<<std::endl;
        std::cout<<"Cient Name: " << client->CLIENT_NAME<<std::endl;
        std::cout<<"Server IP: " << argv[3]<<std::endl;
        std::cout<<"Server PORT: " << argv[4]<<std::endl;
        std::cout<<"Client UDP PORT: " << client->UDP_PORT<<std::endl;
        std::cout<<"Client TCP PORT: " << client->TCP_PORT<<std::endl;
        std::cout<<"====================="<<std::endl;

        client->createSocket();
        client->bindSocketToPort(&client->client_addr_udp, client->UDP_PORT,client->client_fd_udp);
        //client->bindSocketToPort(&client->client_addr_tcp, client->TCP_PORT,client->client_fd_tcp);
        client->registerAccount();

        std::thread t(&Client::listenForResponse, client);
        std::thread t2(&Client::handleTCPConnection, client);

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
                if(words.size()!=3) std::cout<<" >>> Incorrect Numebr of arguments"<<std::endl;
                else client->requestFile(words[1], words[2]);
            }
            else{
                std::cout<<"Your command cannot be understand"<<std::endl;
            }
        }
        // Close the sockets
        close(client->client_fd_udp);
        close(client->client_fd_tcp);
        return 0;
    }
}