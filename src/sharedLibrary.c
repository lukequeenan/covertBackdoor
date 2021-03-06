#include "sharedLibrary.h"

/*
 -- FUNCTION: systemFatal
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Aman Abdulla
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: static void systemFatal(const char* message);
 --
 -- RETURNS: void
 --
 -- NOTES:
 -- This function displays an error message and shuts down the program.
 */
void systemFatal(const char *message)
{
#ifdef DEBUG
    perror(message);
#endif
    exit(EXIT_FAILURE);
}

char *encrypt_data(char *input, char *key, int inputLength)
{
    int i, x, y;
    
    if (inputLength == 0)
        x = strlen(input);
    else
        x = inputLength;
    
    y = strlen(key);
    
    for (i = 0; i < x; ++i)
    {
        input[i] ^= key[(i%y)];
    }
    return input;
}

// Simple checksum function, may use others such as Cyclic Redundancy Check, CRC
unsigned short csum(unsigned short *buf, int len)
{
    unsigned long sum;
    for (sum = 0; len > 0; len--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short) (~sum);
}
