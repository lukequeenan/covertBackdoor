#ifndef CLIENT_H
#define CLIENT_H

#include "sharedLibrary.h"

typedef struct
{
    char destHost[16];
    int destPort;
    char srcHost[16];
    int srcPort;
} netInfo;

#endif

