// oss_h
#ifndef OSS_H
#define OSS_H

#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>   // used for inter-process communication flags
#include <sys/shm.h>
#include <sys/wait.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>   // ftok key generation

#define TIME_QUANTUM (500000) // half a second in ns

// 'S_IRUSR' for read-permissions and 'S_IWUSR' for write
const int PERMS = S_IRUSR | S_IWUSR;

// generates a unique key based on filepath of 'oss.h'
const int msq_key = ftok("oss.h", 0);

// structure for messages which are sent
struct message {
    long priority;   // can be q0, q1, or q2
    int value;      // actual message
};

// creates shared time structure for 'oss.cpp' and 'worker.cpp' 
struct SharedTime {
    int seconds;
    int nanoseconds;
};

/* creates loop to simulate system time */
void incrementClock(SharedTime& clock) {
    clock.nanoseconds += 500000;

    if (clock.nanoseconds >= 1000000000) {
        clock.seconds += 1;
        clock.nanoseconds %= 1000000000;
    }
}

#endif // OSS_H