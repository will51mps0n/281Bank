//292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
// BankingSystem.h

#ifndef BANKINGSYSTEM_H
#define BANKINGSYSTEM_H

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include<set>
#include <list>
//forward declare user for transaction struct --

struct Transaction
{
    uint64_t executionDate; // 8 bytes
    uint32_t amount;        // 8 bytes
    uint32_t feeAmount = 0; // 8 bytes, initialize to 0
    std::string recipient;  // Assuming a 32-byte string (small string optimization) - change to pointers
    std::string sender;     // Assuming a 32-byte string (small string optimization) - change to pointer
    uint32_t transactionID; // 4 bytes
    char feeType;           // 1 byte

    Transaction(uint64_t execDate, std::string receive, std::string send, uint32_t amt, char ft)
        : executionDate(execDate), amount(amt), recipient(std::move(receive)), sender(std::move(send)), transactionID(getNextTransactionID()), feeType(ft) {}
    
    private:
    static uint32_t getNextTransactionID()
    {
        static uint32_t nextTransactionID = 0;
        return nextTransactionID++;
    }
};

struct User {
    uint64_t timestamp; // 8 bytes
    uint32_t balance;   // 8 bytes
    uint32_t PIN;
    std::set<uint64_t> userIps; // Set of IPs -
    std::string userID; // Assuming a 32-byte string (small string optimization)
    std::deque<const Transaction*> userIncomingTransactions; // Deque of pointers to incoming transactions
    std::deque<const Transaction*> userOutgoingTransactions; // Deque of pointers to outgoing transactions

    bool isUserLoggedIn() const
    {
        return !userIps.empty();
    }
};

// convert the timestamp string into a uint64_t
uint64_t convertTimestamp(const std::string& timestamp);

uint32_t convertToUint32 (const std::string& timestamp);
// parse a delimited string into a vector of strings
std::vector<std::string> parseInput(const std::string& input, char delimiter);

//  function to read the registration file and populate the hash table
bool readAndStoreUserData(const std::string& filename, std::unordered_map<std::string, User>& userTable);

#endif // BANKINGSYSTEM_H
