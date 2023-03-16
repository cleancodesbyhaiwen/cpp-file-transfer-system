#pragma once

#include <string>
#include <vector>

// Check whether a file exist given its file path
bool fileExists(const std::string& filename);

// Check whether an IP address input is valid
bool isValidIPAddress(const char* ipAddress);

// Determine mode by -c/-s
const char* determineMode(char** argv);

// Check whether command line arguments are valid
void clientModeArgumentsCheck(int argc, char** argv);

// Check whether command line arguments are valid
void serverModeArgumentsCheck(int argc, char** argv);

// Split a string into vector of words by a specific delimiter e.g. space or *
void splitString(std::vector<std::string> &words, std::string str, char delimiter);

// Check whether a given directory exist in the file system
bool directoryExists(const char* path);

// Disply a list of commands the user can input
void displayCommandList();

// Get the size of a file given the file path
size_t getFileSize(std::string file_path);

// Print message to the console.
// Type either 's' for square bracket or 'a' for angular bracket
// Color can either be 'g' for green, 'r' for red, or 'w' for white
void printMsg(std::string msg, char type, char color);