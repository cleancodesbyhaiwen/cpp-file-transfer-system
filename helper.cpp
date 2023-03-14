#include <string>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>

#include "helper.hpp"

bool fileExists(const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}

bool isValidIPAddress(const char* ipAddress) {
    struct sockaddr_in sa = {0};
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

const char* determineMode(char** argv)
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
            exit(1);
    }
    return MODE;
}

void clientModeArgumentsCheck(int argc, char** argv)
{
    if(argc != 7){
        std::cerr<<"Incorrect number of arguments"<<std::endl;
        std::cerr<<"Usage: ./file -c [client name] [server IP] [server port] [client tcp port] [client udp port]"<<std::endl;
        exit(1);
    }
    if(!isValidIPAddress(argv[3])){
        std::cerr<<"Invalid IP address"<<std::endl;
        exit(1);
    }
    for(int i = 4;i <= 6;i++){
        if(atoi(argv[i]) < 1024 || atoi(argv[i]) > 65535){
            std::cerr<<"Port number out of range"<<std::endl;
            exit(1);        
        }
    }
}

void serverModeArgumentsCheck(int argc, char** argv)
{
    if(argc != 3){
        std::cerr<<"Incorrect number of arguments"<<std::endl;
        std::cerr<<"Usage: ./file -s [port number]"<<std::endl;
        exit(1);
    }
    if(atoi(argv[2]) < 1024 || atoi(argv[2]) > 65535){
        std::cerr<<"Port number out of range"<<std::endl;
        exit(1);  
    }
}

void splitString(std::vector<std::string> &words, std::string command, char delimiter)
{
    std::stringstream ss(command);
    std::string word;
    while(std::getline(ss, word, delimiter)) {
        words.push_back(word);
    }
}

bool directoryExists(const char* path) {
  DIR* dir = opendir(path);
  if (dir != nullptr) {
    closedir(dir);
    return true;
  } else {
    return false;
  }
}

void displayCommandList()
{
    std::cout<<std::endl;
    std::cout<<"  setdir [dir_name]:                    set the directory containing files to be shared"<<std::endl;
    std::cout<<"  offer [file1] [file2] ... :           send to server the names of the files to offer to peers"<<std::endl;
    std::cout<<"  request [filename] [client_name]:     request from peer a file and store in the current directory"<<std::endl;
    std::cout<<"  dereg [client_name]:                  change the status to offline, you won't receive table update and others won't be able to request file from you."<<std::endl;
    std::cout<<"  back [client_name]:                   change the status back to online"<<std::endl;
    std::cout<<std::endl;
}

size_t getFileSize(std::string file_path)
{
    FILE* file = fopen(file_path.c_str(), "rb");
    if (file == nullptr) {
        std::cout << "Failed to open file." << std::endl;
        return 0;
    }
    fseek(file, 0, SEEK_END);
    std::size_t file_size = ftell(file);
    fclose(file);
    return file_size;
}