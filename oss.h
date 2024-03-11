// oss_h
#ifndef OSS_H
#define OSS_H

#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>

// creates shared time structure for 'oss.cpp' and 'worker.cpp' 
struct SharedTime {
    int seconds;
    int nanoseconds;
};

extern int activeChildren;  // global variable to keep track of the chld

void incrementClock(SharedTime& clock, int activeChildren);

/* creates loop to simulate system time */
void incrementClock(SharedTime& clock) {
    clock.nanoseconds += 250;

    if (clock.nanoseconds >= 1000000000) {
        clock.seconds += 1;
        clock.nanoseconds %= 1000000000;
    }
}

#endif // OSS_H