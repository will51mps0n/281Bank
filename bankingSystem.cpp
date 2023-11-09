// 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "bankingSystem.h"

// Function to convert the timestamp string into a uint64_t
uint64_t convertTimestamp(const std::string &timestamp)
{
    //  colons and convert the  string into a large integer
    std::string digits;
    for (char c : timestamp)
    {
        if (isdigit(c))
            digits.push_back(c);
    }
    return std::stoull(digits);
}

// Function to parse a delimited string into a vector of strings
std::vector<std::string> parseInput(const std::string &input, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        result.push_back(token);
    }

    return result;
}

// Main function to read the registration file and populate the hash table
bool readAndStoreUserData(const std::string &filename, std::unordered_map<std::string, User> &userTable)
{
    std::ifstream regFile(filename);
    std::string line;

    if (!regFile.is_open())
    {
        std::cerr << "Registration file failed to open" << filename << std::endl;
        return false;
    }

    while (std::getline(regFile, line))
    {
        std::vector<std::string> userData = parseInput(line, '|');

        if (userData.size() != 4)
        {
            std::cerr << "Invalid data format: " << line << std::endl;
            continue; // Skip this line and move to the next
        }
        User newUser;
        try
        {
            /*
            unsigned long long balanceULL = std::stoull(userData[3]);
            if (balanceULL > std::numeric_limits<uint32_t>::max())
            {
                std::cerr << "Error: Balance exceeds maximum value for uint32_t." << std::endl;
                return false; // or handle the error as appropriate
            }
            
            newUser.balance = static_cast<uint32_t>(balanceULL);
            */
            newUser.timestamp = convertTimestamp(userData[0]);
            newUser.userID = userData[1];
            newUser.PIN = userData[2];
            newUser.balance = std::stoull(userData[3]);
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Error parsing data: " << e.what() << std::endl;
            continue; // Skip this line and move to the next
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "Error: Number out of range encountered: " << e.what() << std::endl;
            continue; // Skip this line and move to the next
        }

        // Insert the new user into the hash table, checking for duplicates
        auto result = userTable.insert({newUser.userID, newUser});
        if (!result.second)
        {
            std::cerr << "Duplicate user ID found: " << newUser.userID << ". This entry will be ignored." << std::endl;
        }
    }
    return true;
}
