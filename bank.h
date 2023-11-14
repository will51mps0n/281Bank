// 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <deque>

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
    // std::unordered_map<std::string, User> validIps;
    std::unordered_map<std::string, User> users;
    // could use a deque
    // use pointers
    std::deque<Transaction> transactionSet;
    // could also make a vector of transactions ordered by time
    uint64_t mostRecentTimeStamp = 0;
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
    void summarizeDay();
    // for querying
    void deleteNewTransactions();
    void printTransactionSet();
    Transaction findTransaction(int id);
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
        // std::cout << "Read operation: "<< operation << std::endl; // Debug output
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
            customerHistory();
            break;
        case 's':
            // std::cout << "summarize " <<std::endl;
            summarizeDay();
            break;
        }
    }
    // for debugging
    // std::cout << "DONE" << std::endl;
}

void Bank::login()
{
    // TODO: change IPs to ints later
    // We have already read in first word "login:
    // will be followed by : <USER_ID> <PIN> <IP>

    /*for (auto &[userId,user] : users) {

        std::cout << "registered: " << userId << " " << user.PIN << std::endl;
    }*/

    std::string user_id, pin, ip;
    std::cin >> user_id >> pin >> ip;
    uint32_t pinInt = static_cast<uint32_t>(std::stoull(pin));
    // std::cout << user_id << " " << pin << " " << ip << std::endl;
    auto it = users.find(user_id);
    if ((it == users.end()) || it->second.PIN != pinInt)
    {
        if (verbose)
        {
            std::cout << "Failed to log in " << user_id << "." << std::endl;
        }
        return;
    }
    it->second.userIps.insert(ip);
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
    if ((it == users.end()) || (!it->second.isUserLoggedIn()) || (it->second.userIps.find(ip) == it->second.userIps.end()))
    {
        if (verbose)
        {
            std::cout << "Failed to log out " << user_id << "." << std::endl;
        }
        return;
    }
    it->second.userIps.erase(ip);
    if (verbose)
    {
        std::cout << "User " << user_id << " logged out." << std::endl;
    }
}
// TODO check for rounding errors
uint32_t calculateFee(uint32_t amount, bool discount)
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
    if (discount)
    {
        fee = (fee * 3) / 4;
    }
    return fee;
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
    uint32_t amtInt = convertToUint32(amt);

    // Error checks for transaction ... may need to return / exit here??
    placeTransactionErrors(timeStampInt, execTimeInt);

    // Store the timestamp for future checks
    mostRecentTimeStamp = timeStampInt;

    // user and fraud checks
    if (!validateParticipants(sender, recipient, timeStampInt, execTimeInt) || !fraudCheck(sender, ip))
    {
        return;
    }
    // feeType check if its o or s
    // verified they exist so we can just call this:
    // User *senderUser = &users.at(sender); // Use pointers instead of references
    // User *recipientUser = &users.at(recipient);

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
    transactions.emplace(execTimeInt, std::move(recipient), std::move(sender), amtInt, feeType);
    // std::cout << "inserting new transaction" << std::endl;
    // delete newTransaction;
    // std::cout << "transaction: " << newTransaction.transactionID << std::endl;
    // std::cout << "transaction date: " << newTransaction.executionDate << std::endl;

    // std::cout << transactionSet.size() << std::endl;
}

/*void Bank::printTransactionSet()
{
    for (auto &item : transactionSet)
    {
        std::cout << "Transaction : ";
        std::cout << item.transactionID << " "
                  << item.executionDate << " "
                  << item.feeAmount << std::endl;
    }
}*/

void Bank::processTransactions(uint64_t timeStampInt)
{
    // check function
    while (!transactions.empty() && timeStampInt >= transactions.top().executionDate)
    {
        Transaction currentTransaction = transactions.top();
        transactions.pop();
        // uint64_t amount = currentTransaction->amount;

        auto senderIt = users.find(currentTransaction.sender);
        auto recipientIt = users.find(currentTransaction.recipient);

        if (senderIt == users.end() || recipientIt == users.end())
        {
            std::cerr << "user error" << std::endl;
            exit(0); // Handle the case where one or both users are not found
        }

        User *transactionSender = &(senderIt->second);
        User *transactionRecipient = &(recipientIt->second);
        // Debug:
        // std::cout << "Transaction sender timestamp: " << transactionSender->timestamp << std::endl;
        uint64_t loyaltyTime = (currentTransaction.executionDate) - (transactionSender->timestamp);
        bool discount = validateDiscount(loyaltyTime);
        uint32_t fee = calculateFee(currentTransaction.amount, discount);
        bool noTransaction = false;

        //  std::cout << currentTransaction.transactionID << " " << currentTransaction.amount << " "
        //          << currentTransaction.feeType << " " << senderIt->second.userID << " " << senderIt->second.balance 
        //         << " " << fee << std::endl;
        switch (currentTransaction.feeType)
        {
        case ('o'):
            if (transactionSender->balance < (fee + currentTransaction.amount))
            {
                if (verbose)
                {
                    std::cout << "Insufficient funds to process transaction "
                              << currentTransaction.transactionID << "." << std::endl;
                }
                noTransaction = true;
                break;
            }
            transactionSender->balance -= fee;
            break;
        case ('s'):
            uint32_t senderFee = (fee + 1) / 2; // Round up for sender fee
            uint32_t recipientFee = fee / 2;    // Round down for recipient fee
            if (transactionSender->balance < (senderFee + currentTransaction.amount) || transactionRecipient->balance < (recipientFee))
            {
                if (verbose)
                {
                    std::cout << "Insufficient funds to process transaction "
                              << currentTransaction.transactionID << "." << std::endl;
                }
                noTransaction = true;
                break;
            }
            transactionSender->balance -= senderFee;
            transactionRecipient->balance -= recipientFee;
            break;
        }
        currentTransaction.feeAmount = fee;
        // std::cout << "transaciton placed fee: " << currentTransaction.feeAmount << std::endl;
        // break from while loop
        if (!noTransaction)
        {

            transactionSender->balance -= currentTransaction.amount;
            transactionRecipient->balance += currentTransaction.amount;
            // user incoming and outgoing transactions::
            transactionSet.push_back(currentTransaction);
            transactionSender->userOutgoingTransactions.push_back(&transactionSet.back());
            if (transactionSender->userOutgoingTransactions.size() > 10)
            {
                transactionSender->userOutgoingTransactions.pop_front();
            }
            transactionRecipient->userIncomingTransactions.push_back(&transactionSet.back());
            if (transactionRecipient->userIncomingTransactions.size() > 10)
            {
                transactionRecipient->userIncomingTransactions.pop_front();
            }

            // printTransactionSet();
            // printTransactionSet();
            // BIGBUGFIX
            if (verbose)
            {
                std::cout << "Transaction executed at " << currentTransaction.executionDate << ": $"
                          << currentTransaction.amount << " from " << transactionSender->userID << " to " << transactionRecipient->userID << "." << std::endl;
            }
        }
    }
}

void Bank::placeTransactionErrors(uint64_t timeStamp, uint64_t executionDate)
{
    // Check if current transaction timestamp is earlier than the previous transaction's timestamp
    if (timeStamp < mostRecentTimeStamp)
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
    if (executionDate <= (senderIt->second.timestamp) ||
        (executionDate <= recipientIt->second.timestamp))
    {
        if (verbose)
        {
            std::cout << "At the time of execution, sender and/or recipient have not registered." << std::endl;
        }
        return false;
    }
    //  sender has an active session
    if (!senderIt->second.isUserLoggedIn())
    {
        if (verbose)
        {
            std::cout << "Sender " << sender << " is not logged in." << std::endl;
        }
        return false;
    }

    return true;
}
bool Bank::fraudCheck(const std::string &senderStr, const std::string &ip)
{
    const User &sender = users.find(senderStr)->second;

    auto fraudIt = sender.userIps.find(ip);
    if (fraudIt == sender.userIps.end())
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

    if (x > y)
    {
        std::cerr << "Invalid input: start time must be less than end time." << std::endl;
        return;
    }
    // std::cout << "time 1: " << x << std::endl << "time 2: " << y << std::endl;

    int transactionCount = 0;
    auto it = std::lower_bound(transactionSet.begin(), transactionSet.end(),
                               x, [](const Transaction &t, uint64_t value)
                               { return value > t.executionDate; });
    for (; it != transactionSet.end(); it++)
    {
        auto &transPtr = *it;
        // debugging:
        /*
        std::cout << "Transaction : ";
        std::cout << transPtr.transactionID << " "
                << transPtr.executionDate << " "
                <<  std::endl; */

        if (transPtr.executionDate >= y)
        {
            break;
        }
        std::cout << transPtr.transactionID << ": " << transPtr.sender
                  << " sent " << transPtr.amount << (transPtr.amount == 1 ? " dollar to " : " dollars to ")
                  << transPtr.recipient << " at " << transPtr.executionDate
                  << "." << std::endl;
        transactionCount++;
    }

    if (transactionCount == 1)
    {
        std::cout << "There was 1 transaction that was placed between time " << x << " to " << y << "." << std::endl;
    }
    else
    {
        std::cout << "There were " << transactionCount << " transactions that were placed between time " << x << " to " << y << "." << std::endl;
    }
}

void timeIntervalOutput(uint64_t time)
{
    uint64_t seconds = (time) % 100;
    uint64_t minutes = (time / 100) % 100;
    uint64_t hours = (time / 10000) % 100;
    uint64_t days = (time / 1000000) % 100;
    uint64_t months = (time / 100000000) % 100;
    uint64_t years = (time / 10000000000) % 100;

    if (years == 1)
    {
        std::cout << " " << years << " year";
    }
    else if (years > 1)
    {
        std::cout << " " << years << " years";
    }
    if (months == 1)
    {
        std::cout << " " << months << " month";
    }
    else if (months > 1)
    {
        std::cout << " " << months << " months";
    }
    if (days == 1)
    {
        std::cout << " " << days << " day";
    }
    else if (days > 1)
    {
        std::cout << " " << days << " days";
    }
    if (hours == 1)
    {
        std::cout << " " << hours << " hours";
    }
    else if (hours > 1)
    {
        std::cout << " " << hours << " hours";
    }
    if (minutes == 1)
    {
        std::cout << " " << minutes << " minute";
    }
    else if (minutes > 1)
    {
        std::cout << " " << minutes << " minutes";
    }
    if (seconds == 1)
    {
        std::cout << " " << seconds << " second";
    }
    else if (seconds > 1)
    {
        std::cout << " " << seconds << " seconds";
    }
    std::cout << ".";
    std::cout << std::endl;
}

void Bank::bankRevenue()
{
    // std::cout << "MADE IT" << std::endl;
    std::string executionTimeX, executionTimeY;
    std::cin >> executionTimeX >> executionTimeY;

    uint64_t x = convertTimestamp(executionTimeX);
    uint64_t y = convertTimestamp(executionTimeY);
    uint64_t bankRev = 0;

    // printTransactionSet();

    auto it = std::lower_bound(transactionSet.begin(), transactionSet.end(),
                               x, [](const Transaction &t, uint64_t value)
                               { return value > t.executionDate; });

    for (auto i = it; i != transactionSet.end(); i++)
    {
        if (i->executionDate >= y)
        {
            break;
        }
        bankRev += i->feeAmount;
    }
    uint64_t timeInterval = y - x;
    // function call to convert time interval to string
    // DONE: add function to convert time to correct format _-- day, months -- etc.
    std::cout << "281Bank has collected " << bankRev << " dollars in fees over";
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
    std::cout << "Customer " << userIt->second.userID << " account summary:" << std::endl;
    std::cout << "Balance: $" << userIt->second.balance << std::endl;
    uint64_t userTransactionInt = userIt->second.userIncomingTransactions.size() + userIt->second.userOutgoingTransactions.size();
    std::cout << "Total # of transactions: " << userTransactionInt << std::endl;
    std::cout << "Incoming " << userIt->second.userIncomingTransactions.size() << ":" << std::endl;

    for (auto *transaction : userIt->second.userIncomingTransactions)
    {
        std::cout << transaction->transactionID
                  << ": " << transaction->sender << " sent "
                  << transaction->amount << (transaction->amount == 1 ? " dollar to " : " dollars to ") << transaction->recipient
                  << " at " << transaction->executionDate << "." << std::endl;
    }
    std::cout << "Outgoing " << userIt->second.userOutgoingTransactions.size() << ":" << std::endl;

    for (auto *transaction : userIt->second.userOutgoingTransactions)
    {

        std::cout << transaction->transactionID
                  << ": " << transaction->sender << " sent "
                  << transaction->amount << (transaction->amount == 1 ? " dollar to " : " dollars to ") << transaction->recipient
                  << " at " << transaction->executionDate << "." << std::endl;
    }
}

uint64_t roundDay(uint64_t time)
{
    return (time / 1000000) * 1000000;
}
void Bank::summarizeDay()
{
    std::string time;
    std::cin >> time;
    uint64_t day = convertTimestamp(time);
    day = roundDay(day);
    uint64_t day2 = day + 1000000;
    std::cout << "Summary of [" << day << ", " << day2 << "):"
              << std::endl;
    // initlaize to -1 for debugging
    int transactionCount = 0;
    uint64_t bankProfit = 0;
    auto it = std::lower_bound(transactionSet.begin(), transactionSet.end(),
                               day, [](const Transaction &t, uint64_t value)
                               { return value > t.executionDate; });
    for (auto &transPtr = it;transPtr != transactionSet.end(); transPtr++)
    {
        // debugging!!:
        /*std::cout << "Transaction : ";
        std::cout << transPtr.transactionID << " "
                << transPtr.executionDate << " "
                <<  std::endl;*/
        if (transPtr->executionDate >= day && transPtr->executionDate < day2)
        {
            std::string amountStr = std::to_string(transPtr->amount) + (transPtr->amount == 1 ? " dollar" : " dollars");
            std::cout << transPtr->transactionID << ": " << transPtr->sender
                      << " sent " << amountStr << " to "
                      << transPtr->recipient << " at " << transPtr->executionDate
                      << "." << std::endl;

            transactionCount++;
            bankProfit += transPtr->feeAmount;
        }
    }
    std::cout << "There" << (transactionCount == 1 ? " was " : " were ") << "a total of " << transactionCount << " transaction" << (transactionCount == 1 ? "" : "s")
              << ", 281Bank has collected " << bankProfit << (bankProfit == 1 ? " dollar " : " dollars ") << "in fees." << std::endl;
}

// just keep deque of transactions instead
// pointers to transactions in deque

/*Transaction Bank::findTransaction(int transactionId)
{
    int low = 0;
    int high = static_cast<int>(transactionSet.size() - 1);

    while (low <= high)
    {
        int mid = low + (high - low) / 2;
        if (static_cast<int>(transactionSet[mid].transactionID) == transactionId)
        {
            return transactionSet[mid];
        }
        else if (static_cast<int>(transactionSet[mid].transactionID) < transactionId)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    // Handle the case where the transaction is not found.
    throw std::runtime_error("Transaction with the given ID not found.");
}*/

// use a deque to store transactions
// pop from front and pushback
// keep under size ten!! better for memory :)