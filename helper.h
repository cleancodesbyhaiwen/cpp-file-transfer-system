#pragma once

#include <string>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>

// Check whether a file exist given its file path
bool fileExists(const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}

// Check whether an IP address input is valid
bool isValidIPAddress(const char* ipAddress) {
    struct sockaddr_in sa = {0};
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

// Determine mode by -c/-s
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

// Check whether command line arguments are valid
bool clientModeArgumentsCheck(int argc, char** argv)
{
    if(argc != 7){
        std::cerr<<"Incorrect number of arguments"<<std::endl;
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

// Check whether command line arguments are valid
bool serverModeArgumentsCheck(int argc, char** argv)
{
    if(argc != 3){
        std::cerr<<"Incorrect number of arguments"<<std::endl;
        exit(1);
    }
    if(atoi(argv[2]) < 1024 || atoi(argv[2]) > 65535){
        std::cerr<<"Port number out of range"<<std::endl;
        exit(1);  
    }
}

// Split a string into words by a specific delimiter e.g. space or *
void splitString(std::vector<std::string> &words, std::string command, char delimiter)
{
    std::stringstream ss(command);
    std::string word;
    while(std::getline(ss, word, delimiter)) {
        words.push_back(word);
    }
}

// Check whether a given directory exist in the file system
bool directoryExists(const char* path) {
  DIR* dir = opendir(path);
  if (dir != nullptr) {
    closedir(dir);
    return true;
  } else {
    return false;
  }
}