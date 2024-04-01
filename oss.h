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
#include <queue>
#include <sys/msg.h>
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
    long priority;   // can be q0, q1, or q2
    int value;      // actual message
};

struct Process {
    int id;              // process time
    int burst_time;      // burst time
    int remaining_time;  // remaining time to execute
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

void roundRobin(queue<Process>& processes, int quantum)
{
    int total_time = 0;

    while (!processes.empty())
    {
        Process current_process = processes.front();
        processes.pop();

        // execute for either quantum or remaing time depending on what's smaller
        int execution_time = min(quantum, current_process.remaining_time);

        // calculate the remaining process time
        current_process.remaining_time -= execution_time;

        // updates the current time (total_time)
        total_time += execution_time;

        //std::cout << "Executing process " << current_process.id << " for " << execution_time << " nanoseconds" << std::endl;

        // pushes back to queue if process is not yet finished
        if (current_process.remaining_time > 0)
            processes.push(current_process);
        //else
            //std::cout << "Process " << current_process.id << " completed." << std::endl;
    }

    cout << "OSS: total time of this dispatch was " << total_time << " nanoseconds" << endl;
}

#endif // OSS_H