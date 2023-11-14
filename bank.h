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
            summarizeDay();
            break;
        }
    }

}

void Bank::login()
{
    std::string user_id, pin, ip;
    std::cin >> user_id >> pin >> ip;
    uint32_t pinInt = static_cast<uint32_t>(std::stoull(pin));
    auto it = users.find(user_id);
    if ((it == users.end()) || it->second.PIN != pinInt)
    {
        if (verbose)
        {
            std::cout << "Failed to log in " << user_id << ".\n";
        }
        return;
    }
    uint64_t ipInt = convertTimestamp(ip);
    it->second.userIps.insert(ipInt);
    if (verbose)
    {
        std::cout << "User " << user_id << " logged in.\n";
    }
}

void Bank::logout()
{
    std::string user_id, ip;
    std::cin >> user_id >> ip;
    uint64_t ipInt = convertTimestamp(ip);
    auto it = users.find(user_id);
    if ((it == users.end()) || (!it->second.isUserLoggedIn()) || (it->second.userIps.find(ipInt) == it->second.userIps.end()))
    {
        if (verbose)
        {
            std::cout << "Failed to log out " << user_id << ".\n";
        }
        return;
    }
    it->second.userIps.erase(ipInt);
    if (verbose)
    {
        std::cout << "User " << user_id << " logged out.\n";
    }
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

    processTransactions(timeStampInt);

    // creation of transaction object
    if (verbose)
    {
        std::cout << "Transaction placed at " << timeStampInt << ": $" << amt << " from "
                  << sender << " to " << recipient << " at " << execTimeInt << ".\n";
    }
    User *senderPtr = &users.at(sender);
    User *recipientPtr = &users.at(recipient);
    transactions.emplace(execTimeInt, recipientPtr, senderPtr, amtInt, feeType);

}


void Bank::processTransactions(uint64_t timeStampInt)
{
    // check function
    for (; !transactions.empty() && timeStampInt >= transactions.top().executionDate; transactions.pop())
    {
        const Transaction &currentTransaction = transactions.top();

        User *transactionSender = currentTransaction.sender;
        User *transactionRecipient = currentTransaction.recipient;

        uint32_t fee = currentTransaction.calculateFee();
        bool noTransaction = false;

        switch (currentTransaction.feeType)
        {
        case ('o'):
            if (transactionSender->balance < (fee + currentTransaction.amount))
            {
                if (verbose)
                {
                    std::cout << "Insufficient funds to process transaction "
                              << currentTransaction.transactionID << ".\n";
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
                              << currentTransaction.transactionID << ".\n";
                }
                noTransaction = true;
                break;
            }
            transactionSender->balance -= senderFee;
            transactionRecipient->balance -= recipientFee;
            break;
        }
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
            transactionRecipient->transactionCountReceived++;
            transactionSender->transactionCountSent++;
            if (verbose)
            {
                std::cout << "Transaction executed at " << currentTransaction.executionDate << ": $"
                          << currentTransaction.amount << " from " << transactionSender->userID << " to " << transactionRecipient->userID << ".\n";
            }
        }
    }
}

void Bank::placeTransactionErrors(uint64_t timeStamp, uint64_t executionDate)
{
    if (timeStamp < mostRecentTimeStamp)
    {
        std::cerr << "Invalid: A place command with a timestamp earlier than the previous place.\n";
        exit(1);
    }
    if (executionDate < timeStamp)
    {
        std::cerr << "Invalid: A place command which contains an execution date before its own timestamp.\n";
        exit(1);
    }
    return;
}

bool Bank::validateParticipants(const std::string &sender, const std::string &recipient, uint64_t timeStamp, uint64_t executionDate)
{

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
            std::cout << "Select a time less than three days in the future.\n";
        }
        return false;
    }
    // Check if both sender and recipient are registered
    if (senderIt == users.end())
    {
        if (verbose)
        {
            std::cout << "Sender " << sender << " does not exist.\n";
        }
        return false;
    }

    if (recipientIt == users.end())
    {
        if (verbose)
        {
            std::cout << "Recipient " << recipient << " does not exist.\n";
        }
        return false;
    }
    if (executionDate <= (senderIt->second.timestamp) ||
        (executionDate <= recipientIt->second.timestamp))
    {
        if (verbose)
        {
            std::cout << "At the time of execution, sender and/or recipient have not registered.\n";
        }
        return false;
    }
    //  sender has an active session
    if (!senderIt->second.isUserLoggedIn())
    {
        if (verbose)
        {
            std::cout << "Sender " << sender << " is not logged in.\n";
        }
        return false;
    }

    return true;
}
bool Bank::fraudCheck(const std::string &senderStr, const std::string &ip)
{
    const User &sender = users.find(senderStr)->second;

    uint64_t ipInt = convertTimestamp(ip);
    auto fraudIt = sender.userIps.find(ipInt);
    if (fraudIt == sender.userIps.end())
    {
        if (verbose)
        {
            std::cout << "Fraudulent transaction detected, aborting request.\n";
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
        std::cerr << "Invalid input: start time must be less than end time.\n";
        return;
    }

    int transactionCount = 0;
    auto it = std::lower_bound(transactionSet.begin(), transactionSet.end(),
                               x, [](const Transaction &t, uint64_t value)
                               { return value > t.executionDate; });
    for (; it != transactionSet.end(); it++)
    {
        auto &transPtr = *it;


        if (transPtr.executionDate >= y)
        {
            break;
        }
        std::cout << transPtr.transactionID << ": " << transPtr.sender->userID
                  << " sent " << transPtr.amount << (transPtr.amount == 1 ? " dollar to " : " dollars to ")
                  << transPtr.recipient->userID << " at " << transPtr.executionDate
                  << ".\n";
        transactionCount++;
    }

    if (transactionCount == 1)
    {
        std::cout << "There was 1 transaction that was placed between time " << x << " to " << y << ".\n";
    }
    else
    {
        std::cout << "There were " << transactionCount << " transactions that were placed between time " << x << " to " << y << ".\n";
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
        std::cout << " " << hours << " hour";
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
    std::cout << ".\n";
}

void Bank::bankRevenue()
{
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
        bankRev += i->calculateFee();
    }
    uint64_t timeInterval = y - x;
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
        std::cout << "User " << user_id << " does not exist.\n";
        return;
    }
    std::cout << "Customer " << userIt->second.userID << " account summary:\n";
    std::cout << "Balance: $" << userIt->second.balance << '\n';
    uint32_t transactionCount = userIt->second.transactionCountSent + userIt->second.transactionCountReceived;
    std::cout << "Total # of transactions: " << transactionCount <<'\n';
    std::cout << "Incoming " << userIt->second.transactionCountReceived << ":\n";

    for (auto *transaction : userIt->second.userIncomingTransactions)
    {
        std::cout << transaction->transactionID
                  << ": " << transaction->sender->userID << " sent "
                  << transaction->amount << (transaction->amount == 1 ? " dollar to " : " dollars to ") << transaction->recipient->userID
                  << " at " << transaction->executionDate << ".\n";
    }
    std::cout << "Outgoing " << userIt->second.transactionCountSent << ":\n";

    for (auto *transaction : userIt->second.userOutgoingTransactions)
    {

        std::cout << transaction->transactionID
                  << ": " << transaction->sender->userID << " sent "
                  << transaction->amount << (transaction->amount == 1 ? " dollar to " : " dollars to ") << transaction->recipient->userID
                  << " at " << transaction->executionDate << ".\n";
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
    std::cout << "Summary of [" << day << ", " << day2 << "):\n";
    // initlaize to -1 for debugging
    int transactionCount = 0;
    uint64_t bankProfit = 0;
    auto it = std::lower_bound(transactionSet.begin(), transactionSet.end(),
                               day, [](const Transaction &t, uint64_t value)
                               { return value > t.executionDate; });
    for (auto &transPtr = it; transPtr != transactionSet.end(); transPtr++)
    {

        if (transPtr->executionDate >= day && transPtr->executionDate < day2)
        {
            std::string amountStr = std::to_string(transPtr->amount) + (transPtr->amount == 1 ? " dollar" : " dollars");
            std::cout << transPtr->transactionID << ": " << transPtr->sender->userID
                      << " sent " << amountStr << " to "
                      << transPtr->recipient->userID << " at " << transPtr->executionDate
                      << ".\n";

            transactionCount++;
            bankProfit += transPtr->calculateFee();
        }
    }
    std::cout << "There" << (transactionCount == 1 ? " was " : " were ") << "a total of " << transactionCount << " transaction" << (transactionCount == 1 ? "" : "s")
              << ", 281Bank has collected " << bankProfit << (bankProfit == 1 ? " dollar " : " dollars ") << "in fees.\n";
}
