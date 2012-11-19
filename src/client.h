#ifndef CLIENT_H
#define CLIENT_H

#include "sharedLibrary.h"

// Packet length
#define PCKT_LEN 8192

typedef struct
{
    char destHost[16];
    int *destPort;
    char srcHost[16];
    int *srcPort;
} netInfo;

#endif