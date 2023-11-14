// 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
//  BankingSystem.h

#ifndef BANKINGSYSTEM_H
#define BANKINGSYSTEM_H

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <set>
#include <list>

// forward declare user for transaction struct --

struct Transaction;

struct User
{
    uint64_t timestamp;                                       // 8 bytes
    std::set<uint64_t> userIps;                               // Set of IPs - variable size, placed near the top to align with the timestamp
    std::deque<const Transaction *> userIncomingTransactions; // Deque of pointers to incoming transactions - variable size
    std::deque<const Transaction *> userOutgoingTransactions; // Deque of pointers to outgoing transactions - variable size
    std::string userID;                                       // Assuming a 24-byte string 8 byte for 64 
    uint32_t balance;                                         // 4 bytes
    uint32_t transactionCountSent = 0;                        // 4 bytes
    uint32_t transactionCountReceived = 0;                    // 4 bytes
    uint32_t PIN;                                             // 4 bytes

    bool isUserLoggedIn() const
    {
        return !userIps.empty();
    }
};

struct Transaction
{
    uint64_t executionDate; // 8 bytes
    User *recipient;        // Change to pointer
    User *sender;           // Change to pointer
    uint32_t amount;        // 4 bytes
    uint32_t transactionID; // 4 bytes
    char feeType;           // 1 byte

    Transaction(uint64_t execDate, User *receive, User *send, uint32_t amt, char ft)
        : executionDate(execDate), recipient(receive), sender(send), amount(amt),
          transactionID(getNextTransactionID()), feeType(ft) {}

    uint32_t calculateFee() const
    {
        
        uint32_t fee = 0;
        if (amount < 1000)
        {
            fee = 10;
        }
        else if (amount > 45000)
        {
            fee = 450;
        }
        else
        {
            fee = amount / 100;
        }
        if (executionDate - sender->timestamp > 5'00'00'00'00'00)
        {
            fee = (fee * 3) / 4;
        }
        return fee;
    }

private:
    static uint32_t getNextTransactionID()
    {
        static uint32_t nextTransactionID = 0;
        return nextTransactionID++;
    }
};



// convert the timestamp string into a uint64_t
uint64_t convertTimestamp(const std::string &timestamp);

uint32_t convertToUint32(const std::string &timestamp);
// parse a delimited string into a vector of strings
std::vector<std::string> parseInput(const std::string &input, char delimiter);

//  function to read the registration file and populate the hash table
bool readAndStoreUserData(const std::string &filename, std::unordered_map<std::string, User> &userTable);

#endif // BANKINGSYSTEM_H
