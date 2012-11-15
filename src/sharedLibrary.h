#ifndef SHARED_LIBRARY_H
#define SHARED_LIBRARY_H

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <netinet/udp.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE_ETHERNET 14
#define CONNECTION_PORT 12000
#define PASSPHRASE "&|\\!"

// Debug flag
#define DEBUG 1

void systemFatal(const char *message);
char *encrypt_data(char *input, char *key);

#endif
