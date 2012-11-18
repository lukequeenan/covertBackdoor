#include "client.h"

// Local prototypes
static int parseConfiguration(const char filePath[], netInfo *info);
static int getUserInput(char *commandData, int command);

int main (int argc, char **argv)
{
    int option = 0;
    int command = -1;
    char configFile[PATH_MAX] = {"../settings.conf"};
    char commandData[PATH_MAX];
    char *temp = NULL;
    netInfo info;
    
    // Change the UID/GID to 0 (raise to root)
	if ((setuid(0) == -1) || (setgid(0) == -1))
    {
        systemFatal("You need to be root for this");
    }
    
    // Process the command line arguments
    while ((option = getopt (argc, argv, "f:c:")) != -1)
    {
        switch (option)
        {
            case 'f':
                snprintf(configFile, sizeof(configFile), "%s", optarg);
                break;
                
            case 'c':
                command = strtol(optarg, &temp, 10);
                break;
                
            default:
            case '?':
                fprintf(stderr, "Usage: %s -f [config file] -c [command type]", argv[0]);
                return 1;
        }   
    }
    
    // Read the configuration file
    parseConfiguration(configFile, &info);
    
    // If the command requires user input, obtain it
    if ((command == EXECUTE_SYSTEM_CALL) || (command == FIND_FILE))
    {
        if (!getUserInput(commandData, command))
            systemFatal("Unable to get user input");
    }
    
    // Determine the next function to call based on the command
    switch (command)
    {
        case EXECUTE_SYSTEM_CALL:
            // EXECUTE
            break;
            
        case FIND_FILE:
            // find file
            break;
        case KEYLOGGER:
            // keylogger
            break;
        default:
            systemFatal("Unknown command, check README for usage");
    }
    
    return 0;
}

static int getUserInput(char *commandData, int command)
{
    char *temp = NULL;
    
    // Prompt the user for the correct input
    if (command == EXECUTE_SYSTEM_CALL)
        printf("Enter command, max %d characters\n", PATH_MAX);
    else
        printf("Enter filename, max %d characters\n", PATH_MAX);
    
    // Obtain the entry
    if (fgets(commandData, PATH_MAX, stdin) != NULL)
    {
        // Remove the newline character if it exists
        if ((temp = strchr(commandData, '\n')) != NULL)
            *temp = '\0';
    }
    else
        return 0;
    return 1;
}

static int parseConfiguration(const char filePath[], netInfo *info)
{
    return 1;
}