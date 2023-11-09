//292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
// BankingSystem.h

#ifndef BANKINGSYSTEM_H
#define BANKINGSYSTEM_H

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
//forward declare user for transaction struct --

struct User;

struct Transaction
{
    uint64_t executionDate;
    uint32_t transactionID;
    User *recipient;
    User *sender;
    uint64_t amount;
    char feeType;
    uint64_t feeAmount = 0;


    Transaction(uint64_t execDate, User *receive, User *send, uint64_t amt, char ft)
        : executionDate(execDate), transactionID(getNextTransactionID()), recipient(receive), sender(send), amount(amt), feeType(ft) {}
    
    private:
    static uint32_t getNextTransactionID(){
        static uint32_t nextTransactionID = 0;
        return nextTransactionID++;
    }
};

struct User {
    //may make a hash table later of logins and timestamps if
    //users can log in and outmultiple times
    uint64_t timestamp;
    std::string userID;
    std::string PIN;
    bool loggedIn = false;
    uint64_t balance;
    std::deque<Transaction*> userIncomingTransactions;
    std::deque<Transaction*> userOutgoingTransactions;
};

// convert the timestamp string into a uint64_t
uint64_t convertTimestamp(const std::string& timestamp);

// parse a delimited string into a vector of strings
std::vector<std::string> parseInput(const std::string& input, char delimiter);

//  function to read the registration file and populate the hash table
bool readAndStoreUserData(const std::string& filename, std::unordered_map<std::string, User>& userTable);

#endif // BANKINGSYSTEM_H
