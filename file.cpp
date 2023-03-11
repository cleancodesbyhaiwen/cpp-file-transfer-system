#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

#include "client.h"
#include "server.h"

int main(int argc, char** argv)
{
    const char* MODE;
    switch(static_cast<char>(argv[1][1])){
        case 'c':
            MODE = "CLIENT";
            break;
        case 's':
            MODE = "SERVER";
            break;
        default:
            std::cerr<<"Use flag -c for client mode and -s for server mode"<<std::endl;
            return 1;
    }

    if(MODE == "SERVER")
    {
        if(argc != 3){
            std::cerr<<"Incorrect number of arguments"<<std::endl;
            return 1;
        }
        std::cout<<"You are in **SERVER** mode"<<std::endl;
        Server* server = new Server(argv[2]);
        server->createSocket();
        server->bindSocketToPort();
        server->readFromSocket();

        // Close the socket
        close(server->server_fd);
        return 0;
    }
    else if(MODE == "CLIENT")
    {
        if(argc != 7){
            std::cerr<<"Incorrect number of arguments"<<std::endl;
            return 1;
        }
        std::cout<<"You are in **CLIENT** mode"<<std::endl;
        std::cout<<"=================="<<std::endl;
        const char* SERVER_IP = argv[3];
        uint16_t SERVER_PORT = atoi(argv[4]);
        Client* client = new Client(argv[5],argv[6], argv[2]);
        std::cout<<"Cient Name: " << client->CLIENT_NAME<<std::endl;
        std::cout<<"Server IP: " << SERVER_IP<<std::endl;
        std::cout<<"Server PORT: " << SERVER_PORT<<std::endl;
        std::cout<<"Client UDP PORT: " << client->UDP_PORT<<std::endl;
        std::cout<<"Client TCP PORT: " << client->TCP_PORT<<std::endl;
        std::cout<<"=================="<<std::endl;

        client->createSocket();
        client->bindSocketToPort();
        client->setServerAddr(SERVER_IP, SERVER_PORT);
        client->registerAccount();
        client->readFromSocket();

        // Close the socket
        close(client->client_fd);
        return 0;
    }
}