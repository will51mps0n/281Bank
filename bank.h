#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <algorithm>
#include "bankingSystem.h"

struct CompareTransactions
{
    bool operator()(const Transaction &a, const Transaction &b) const
    {
        if (a.executionDate == b.executionDate)
        {
            return a.transactionID > b.transactionID;
        }
        return a.executionDate > b.executionDate;
    }
};

void eatComment()
{
    std::string comment;
    getline(std::cin, comment, '\n');
    return;
}

class Bank
{
private:
    std::unordered_map<std::string, User> validIps;
    std::unordered_map<std::string, User> users;
    std::vector<Transaction *> transactionVec;
    // could also make a vector of transactions ordered by time
    std::vector<uint64_t> timeStamps;
    // comparison functor
    // maybe make a pq to transaction pointers
    std::priority_queue<Transaction, std::vector<Transaction>, CompareTransactions> transactions;
    void placeTransactionErrors(uint64_t timeStamp, uint64_t executionDate);
    bool validateParticipants(const std::string &sender, const std::string &recipient,
                              uint64_t timeStamp, uint64_t executionDate);
    bool fraudCheck(const std::string &sender, const std::string &ip);
    bool verbose;
    // q mode (after "$$$")
    void listTransactions();
    void bankRevenue();
    void customerHistory();

public:
    Bank(const std::unordered_map<std::string, User> &users, bool vb);
    void login();
    void logout();
    void placeTransaction();
    void addTransaction(const Transaction &transaction);
    bool validateTransaction(const Transaction &transaction);
    void processTransactions(uint64_t time);
    void processCommands();

    // for querying
};

Bank::Bank(const std::unordered_map<std::string, User> &users, bool vb)
    : users(users), verbose(vb) {}

void Bank::processCommands()
{
    std::string operation;
    std::cin >> operation;
    while (operation[0] != '$')
    {
        switch (operation[0])
        {
        // comment
        case '#':
            eatComment();
            break;
        // login
        case 'l':
            login();
            break;
        // logout
        case 'o':
            logout();
            break;
        // place transaction
        case 'p':
            placeTransaction();
            break;
        }
        std::cin >> operation;
    }
    processTransactions(std::numeric_limits<uint64_t>::max());
    // once $ encountered, check if pq empty,
    // if not::do the rest of the transactions
    // if we break from loop, we must have reached $$$,
    // so we go into query mode
     while (std::cin >> operation)
     {
         switch (operation[0])
         {
         case 'l':
             listTransactions();
             break;
         // logout
         case 'r':
             bankRevenue();
             break;
         // place transaction
         case 'h':
             eatComment();
             break;
         }
         std::cin >> operation;
     }
    // for debugging
    std::cout << "DONE" << std::endl;
}

void Bank::login()
{
    // TODO: change IPs to ints later
    // We have already read in first word "login:
    // will be followed by : <USER_ID> <PIN> <IP>
    std::string user_id, pin, ip;
    std::cin >> user_id >> pin >> ip;
    auto it = users.find(user_id);
    if ((it == users.end()) || it->second.PIN != pin)
    {
        if (verbose)
        {
            std::cout << "Failed to log in " << user_id << std::endl;
        }
        return;
    }
    validIps.insert({ip, it->second});
    it->second.loggedIn = true;
    if (verbose)
    {
        std::cout << "User " << user_id << " logged in." << std::endl;
    }
}

void Bank::logout()
{
    std::string user_id, ip;
    std::cin >> user_id >> ip;
    auto it = users.find(user_id);
    if ((it == users.end()) || it->second.userID != user_id)
    {
        if (verbose)
        {
            std::cout << "Failed to log out " << user_id << std::endl;
        }
        return;
    }
    validIps.erase(ip);
    it->second.loggedIn = false;
    if (verbose)
    {
        std::cout << "User " << user_id << " logged out." << std::endl;
    }
}
// TODO check for rounding errors
uint64_t calculateFee(uint64_t amount, bool discount)
{

    if (amount < 1000)
    {
        amount = 10;
    }
    else if (amount > 45000)
    {
        amount = 450;
    }
    else
    {
        amount = 100;
    }
    if (discount)
    {
        amount = (amount * 3) / 4;
    }
    return amount;
}

bool validateDiscount(uint64_t date)
{
    if (date >= 050000000000)
    {
        return true;
    }
    return false;
}

void Bank::placeTransaction()
{
    std::string timeStamp, ip, sender, recipient, amt, execDate;
    char feeType;
    std::cin >> timeStamp >> ip >> sender >> recipient >> amt >> execDate >> feeType;

    uint64_t execTimeInt = convertTimestamp(execDate);
    uint64_t timeStampInt = convertTimestamp(timeStamp);
    uint64_t amtInt = convertTimestamp(amt);

    // Error checks for transaction ... may need to return / exit here??
    placeTransactionErrors(timeStampInt, execTimeInt);

    // Store the timestamp for future checks
    timeStamps.push_back(timeStampInt);

    // user and fraud checks
    if (!validateParticipants(sender, recipient, timeStampInt, execTimeInt) || !fraudCheck(sender, ip))
    {
        return;
    }
    // feeType check if its o or s
    // verified they exist so we can just call this:
    User *senderUser = &users.at(sender); // Use pointers instead of references
    User *recipientUser = &users.at(recipient);

    Transaction newTransaction(execTimeInt, recipientUser, senderUser, amtInt, feeType);
    // transactions.push(newTransaction);

    // the transaction is now in the PQ of transactions
    // if this new transaction later, we can proces sup to this point
    // so we can run through the pq till this is at the top

    processTransactions(timeStampInt);

    // creation of transaction object
    if (verbose)
    {
        std::cout << "Transaction placed at " << timeStampInt << ": $" << amt << " from "
                  << sender << " to " << recipient << " at " << execTimeInt << "." << std::endl;
    }
    transactions.push(newTransaction);
}

void Bank::processTransactions(uint64_t timeStampInt)
{
    while (!transactions.empty() && timeStampInt >= transactions.top().executionDate)
    {
        Transaction *currentTransaction = const_cast<Transaction *>(&transactions.top());
        transactions.pop();
        // uint64_t amount = currentTransaction->amount;
        User &transactionSender = users.at(currentTransaction->sender->userID);
        User &transactionRecipient = users.at(currentTransaction->recipient->userID);
        // make a discount function -- Not sure how to check if longstanding bank member
        bool discount = validateDiscount(transactionSender.timestamp);
        uint64_t fee = calculateFee(currentTransaction->amount, discount);
        bool noTransaction = false;
        switch (currentTransaction->feeType)
        {
        case ('o'):
            if (transactionSender.balance < (fee + currentTransaction->amount))
            {
                if (verbose)
                {
                    std::cout << "Insufficient funds to process transaction <transaction_id>."
                              << currentTransaction->transactionID << std::endl;
                }
                noTransaction = true;
                break;
            }
            transactionSender.balance -= fee;
            // Ask about this -- office hours -- taken from c++ b ut dont undersatnd utility
            [[fallthrough]];
        case ('s'):
            uint64_t senderFee = (fee + 1) / 2; // Round up for sender fee
            uint64_t recipientFee = fee / 2;    // Round down for recipient fee
            if (transactionSender.balance < (senderFee + currentTransaction->amount) || transactionRecipient.balance < (recipientFee))
            {
                if (verbose)
                {
                    std::cout << "Insufficient funds to process transaction <transaction_id>."
                              << currentTransaction->transactionID << std::endl;
                }
                noTransaction = true;
                break;
            }
            transactionSender.balance -= senderFee;
            transactionRecipient.balance -= recipientFee;
        }
        currentTransaction->feeAmount = fee;

        // break from while loop
        if (noTransaction)
            break;
        transactionSender.balance -= currentTransaction->amount;
        transactionRecipient.balance += currentTransaction->amount;

        transactionVec.push_back(currentTransaction);
        transactionSender.userOutgoingTransactions.push_back(currentTransaction);
        transactionRecipient.userIncomingTransactions.push_back(currentTransaction);
        // Have to parse current transaction execution date to time
        std::cout << "Transaction executed at " << currentTransaction->executionDate << " $"
                  << currentTransaction->amount << " from " << transactionSender.userID << " to " << transactionRecipient.userID << "." << std::endl;
    }
}
void Bank::placeTransactionErrors(uint64_t timeStamp, uint64_t executionDate)
{
    // Check if current transaction timestamp is earlier than the previous transaction's timestamp
    if (!timeStamps.empty() && timeStamp < timeStamps.back())
    {
        std::cerr << "Invalid: A place command with a timestamp earlier than the previous place." << std::endl;
        exit(1);
    }
    // Check if execution date is before timestamp
    if (executionDate < timeStamp)
    {
        std::cerr << "Invalid: A place command which contains an execution date before its own timestamp." << std::endl;
        exit(1);
    }
    return;
}

bool Bank::validateParticipants(const std::string &sender, const std::string &recipient, uint64_t timeStamp, uint64_t executionDate)
{
    // added for debugging -- unused parameter now but may need to use later, if not change function
    //  need to compile now to test
    /*
    debug:
    std::cout << "ip is: " << ip << std::endl;
    */
    auto senderIt = users.find(sender);
    auto recipientIt = users.find(recipient);
    // Check if the execution date is more than three days after the timestamp
    //  1 day = 1000000  in timestamp, then 3 days = 3000000
    uint64_t threeDays = 3000000;
    uint64_t difference = executionDate - timeStamp;
    if (difference > threeDays)
    {
        if (verbose)
        {
            std::cout << "Select a time less than three days in the future."
                      << std::endl;
        }
        return false;
    }
    // Check if both sender and recipient are registered
    if (senderIt == users.end())
    {
        if (verbose)
        {
            std::cout << "Sender " << sender << " does not exist." << std::endl;
        }
        return false;
    }

    if (recipientIt == users.end())
    {
        if (verbose)
        {
            std::cout << "Recipient " << recipient << " does not exist." << std::endl;
        }
        return false;
    }
    if (executionDate < (senderIt->second.timestamp) ||
        (executionDate < recipientIt->second.timestamp))
    {
        if (verbose)
        {
            std::cout << "At the time of execution, sender and/or recipient have not registered." << std::endl;
        }
        return false;
    }
    //  sender has an active session
    if (!senderIt->second.loggedIn)
    {
        if (verbose)
        {
            std::cout << "Sender " << sender << " is not logged in." << std::endl;
        }
        return false;
    }

    return true;
}
bool Bank::fraudCheck(const std::string &sender, const std::string &ip)
{
    auto fraudIt = validIps.find(ip);
    if (fraudIt == validIps.end() || fraudIt->second.userID != sender)
    {
        if (verbose)
        {
            std::cout << "Fraudulent transaction detected, aborting request." << std::endl;
        }
        return false;
    }
    return true;
}

void Bank::listTransactions()
{
    std::string executionTimeX, executionTimeY;
    std::cin >> executionTimeX >> executionTimeY;

    uint64_t x = convertTimestamp(executionTimeX);
    uint64_t y = convertTimestamp(executionTimeY);
    int transactionCount = 0;

    auto comp = [](const Transaction *lhs, uint64_t rhs)
    { return lhs->executionDate < rhs; };

    // Use std::lower_bound with comparator
    auto it = std::lower_bound(transactionVec.begin(), transactionVec.end(), x, comp);

    for (auto i = it; i != transactionVec.end(); ++i)
    {
        if ((*i)->executionDate >= x && (*i)->executionDate < y)
        {
          if ((*i)->amount == 1) {
                std::cout << (*i)->transactionID << ": " << (*i)->sender
                           << " sent " << (*i)->amount << " dollar to "
                           << (*i)->recipient << " at " << (*i)->executionDate
                           << "." << std::endl;
          }
          else {
                std::cout << (*i)->transactionID << ": " << (*i)->sender
                           << " sent " << (*i)->amount << " dollars to "
                           << (*i)->recipient << " at " << (*i)->executionDate
                           << "." << std::endl;
          }
          transactionCount++;
        }
        if ((*i)->executionDate > y)
        {
            break;
        }
    }
    if (transactionCount == 1)
    {
        std::cout << "There was " << transactionCount << " transactions that were placed between time x to y."
                  << std::endl;
    }
    else
    {
        std::cout << "There were " << transactionCount << " transactions that were placed between time x to y."
                  << std::endl;
    }
}
void timeIntervalOutput(uint64_t time)
{
    uint64_t seconds = (time) % 100;
    uint64_t minutes = (time / 100) % 100;
    uint64_t hours = (time / 10000) % 100;
    uint64_t days = (time / 1000000) % 100;
    uint64_t months = (time / 100000000) % 100;

    if (months == 1)
    {
        std::cout << months << " month ";
    }
    else
    {
        std::cout << months << " months ";
    }
    if (days == 1)
    {
        std::cout << days << " day ";
    }
    else
    {
        std::cout << days << " days ";
    }
    if (hours == 1)
    {
        std::cout << hours << " hours ";
    }
    else
    {
        std::cout << hours << " hours ";
    }
    if (minutes == 1)
    {
        std::cout << minutes << " minute ";
    }
    else
    {
        std::cout << minutes << " minutes ";
    }
    if (seconds == 1)
    {
        std::cout << seconds << " second.";
    }
    else
    {
        std::cout << seconds << " seconds.";
    }
    std::cout << std::endl;
}
void Bank::bankRevenue()
{
    std::string executionTimeX, executionTimeY;
    std::cin >> executionTimeX >> executionTimeY;

    uint64_t x = convertTimestamp(executionTimeX);
    uint64_t y = convertTimestamp(executionTimeY);
    uint64_t bankRev = 0;
    auto comp = [](const Transaction *lhs, uint64_t rhs)
    { return lhs->executionDate < rhs; };

    // Use std::lower_bound with the corrected comparator
    auto it = std::lower_bound(transactionVec.begin(), transactionVec.end(), x, comp);

    for (auto i = it; i != transactionVec.end(); ++i)
    {
        if ((*i)->executionDate >= x && (*i)->executionDate < y)
        {
            bankRev += (*i)->feeAmount;
        }
        if ((*i)->executionDate > y)
        {
            break;
        }
    }
    uint64_t timeInterval = y - x;
    // function call to convert time interval to string
    // DONE: add function to convert time to correct format _-- day, months -- etc.
    std::cout << "281Bank has collected " << bankRev << " dollars in fees over ";
    timeIntervalOutput(timeInterval);
}

void Bank::customerHistory()
{
    std::string user_id;
    std::cin >> user_id;

    auto userIt = users.find(user_id);
    if (userIt == users.end())
    {
        std::cout << "User " << user_id << " does not exist." << std::endl;
        return;
    }
    for (size_t i = userIt->second.userIncomingTransactions.size() - 1;
        i < userIt->second.userIncomingTransactions.size(); i++)
    {
        Transaction *currentTransaction = userIt->second.userIncomingTransactions[i];
        std::cout << currentTransaction->transactionID
                  << ": " << currentTransaction->sender << " sent" << std::endl;
    }
}

// use a deque to store transactions
// pop from front and pushback
// keep under size ten!! better for memory :)