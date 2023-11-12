//292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
#include <iostream>
#include <unistd.h>
#include "bank.h"
#include "bankingSystem.h"

// Struct to hold the parsed command line arguments
struct CommandLineOptions {
    std::string registrationFile;
    bool verbose = false;
    bool help = false;
};

CommandLineOptions parseArgs(int argc, char* argv[]) {
    CommandLineOptions options;
    int opt;

    // Use getopt to parse the options
    while ((opt = getopt(argc, argv, "hvf:")) != -1) {
        switch (opt) {
            case 'h':  
                options.help = true;
                break;
            case 'v':  
                options.verbose = true;
                break;
            case 'f':  
                options.registrationFile = optarg;
                break;
            default:  
                break;
        }
    }

    return options;
}


int main(int argc, char* argv[]) {
    CommandLineOptions options = parseArgs(argc, argv);

    // debug:
    //std::cout << options.verbose <<std::endl;
    if (options.help) {
        std::cout << "Usage: bank [--help/-h] [--file/-f <filename>] [--verbose/-v]\n";
        exit(0);
    }

    if (options.registrationFile.empty()) {
        std::cerr << "Error: No registration file provided. Use -f option to specify the file.\n";
        exit(1);
    }

    std::unordered_map<std::string, User> userTable;
    if (!readAndStoreUserData(options.registrationFile, userTable)) {
        std::cerr << "Failed to read user data from file.\n";
        return 1;
    }

    /* debiug: print the contents of the hash table initilaized in readand store data
    for (const auto& [userID, user] : userTable) {
        std::cout << "UserID: " << userID
                  << ", Timestamp: " << user.timestamp
                  << ", PIN: " << user.PIN
                  << ", Balance: " << user.balance
                  << std::endl;
    }
    */
    
    Bank bank(userTable, options.verbose);
    bank.processCommands();
    //bank.deleteNewTransactions();
    return 0;
}