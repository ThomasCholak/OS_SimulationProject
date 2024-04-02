// oss_h
#ifndef OSS_H
#define OSS_H

#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>   // used for inter-process communication flags
#include <ctime>        /* used for seeding the time */
#include <sys/shm.h>
#include <sys/wait.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <queue>       // used for the round-robin queue
#include <sys/msg.h>
#include <fstream>     // used for checking line number for logfile
#include <sys/ipc.h>   // ftok key generation

using namespace std;

/* global variable for keeping track of process numbers */
extern int processNum;

// 'S_IRUSR' for read-permissions and 'S_IWUSR' for write
const int PERMS = S_IRUSR | S_IWUSR;

// generates a unique key based on filepath of 'oss.h'
const int msq_key = ftok("oss.h", 0);

// structure for messages which are sent
struct message {
    int value;      // placeholder value for messages
};

struct Process {
    int id;              // process time
    int burst_time;      // burst time
    int remaining_time;  // remaining execution time
    int priority;        // queue priority (q0, q1, q2)
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

// boolean function to check if logfile has exceeded 10000 lines
bool fileLimit(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "error: could not open log file" << std::endl;
        return false;
    }

    int lineCount = 0;
    std::string line;
    while (std::getline(file, line))
    {
        ++lineCount;
        if (lineCount > 10000)  // checks current line count
        {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

#endif // OSS_H