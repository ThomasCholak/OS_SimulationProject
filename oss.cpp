/*
    Project 4
    Author: Thomas Cholak
    Due Date: 02 April 2024
*/

#include <iomanip>      /* just used for PCB formatting */
#include <fcntl.h>      /* needed to import: O_CREAT */
#include <cstdlib>      /* needed to generate random numbers */
#include <errno.h>      /* used for signal  error handling */
#include <signal.h>     /* used for signal handling */
#include <sys/time.h>   /* used for keeping track of real-life seconds */
#include <sys/msg.h>    /* message queues */

#include "oss.h"        /* imports header file "oss.cpp" */

/* global variables for share mem and msg queues */
int msgq;
int shmid;
SharedTime* sharedMem;

int processNum = 0;

struct PCB {
    int occupied;         // either "0" or "1"
    pid_t pid;            // process id of this child
    int startSeconds;     // time when it was forked (sec)
    int startNano;        // time when it was forked (nano)
    int blocked;                // whether process is blocked
    int eventBlockedUntilSec;   // when will this process become unblocked (sec)
    int eventBlockedUntilNano;  // when will this process become unlbocked (nano)
};

struct PCB processtable[20];  // PCD is 20 entries long

void generateProcesses(queue<Process> queues[], int num_processes, int max_burst_time, int increment)
{
    srand(time(nullptr)); // seed for random number generation

    key_t key = 859047;  // child uses same key as parent
    int shmid = shmget(key, sizeof(SharedTime), IPC_CREAT | 0666);

    if (shmid == -1) {
        perror("Error in child: shmget");
    }

    SharedTime* sharedMem = (SharedTime*)shmat(shmid, nullptr, 0);
    if (sharedMem == (SharedTime*)-1) {
        perror("Error in child: shmat");
    }

    for (int i = 1; i <= num_processes; ++i)
    {
        processNum++;

        int burst_time = rand() % (max_burst_time*10000) + 1;  // generates a random burst time (between 1 and max_burst_time)
        int priority = rand() % 3;                             // randomly decides queue order
        
        // initializes a new process
        Process new_process;

        new_process.id = processNum;
        new_process.burst_time = burst_time;
        new_process.remaining_time = burst_time;
        new_process.priority = priority;

        sharedMem->nanoseconds+=100;

        cout << "OSS: Generating process with PID " << new_process.id << " and putting it in queue " << new_process.priority << " at time "
            << sharedMem->seconds << ":" << sharedMem->nanoseconds << "\n";

        // adds the process to the queue
        queues[priority].push(new_process);
    }

    shmdt(sharedMem);
}

int pidTracker = 0;  // stops the blocked processes and trakcs PIDs for logging

void roundRobinScheduling(queue<Process> queues[], int quantum)
{
    int total_time = 0;

    int priorityQuantum[] = {10, 20, 40};

    // Perform round-robin scheduling for each priority queue
    for (int i = 2; i >= 0; --i)  // Start with the highest priority
    { 
        int currentQuantum = priorityQuantum[i];

        while (!queues[i].empty())
        {
            Process current_process = queues[i].front();
            queues[i].pop();

            // five percent chance process is blocked
            if (rand() % 100 < 15)
            {
                if ((pidTracker + 1) < current_process.id)
                {
                    cout << "OSS: Putting process with PID " << current_process.id << " into blocked queue" << endl;
                    current_process.remaining_time = currentQuantum;             // resets its timer to the quantum time
                    sharedMem->nanoseconds += currentQuantum * 1000000;          // sleep for 1 ms

                    if (i < 2) {
                        queues[i + 1].push(current_process);    //adds blocked process to high priority queue (if not already in there)
                    }
                    else
                        cout << "Process " << current_process.id << " (priority " << current_process.priority << ") completed." << endl;
                }
            }

            // run for either quantum or remaining time depending on what is min
            int execution_time = min(quantum, current_process.remaining_time);

            // updates process time
            current_process.remaining_time -= execution_time;
            total_time += execution_time;

            if (current_process.remaining_time > 0)
                queues[i].push(current_process);
        }
    }

    cout << "OSS: total time of this dispatch was " << total_time << " nanoseconds" << endl;
}

// used signal-handling code per Hauschild's example
static void myhandler(int s) {
	printf("\nReceived signal %d. Process terminated.\n", s);

    // kills all the children stored in the process table
    for (int i = 0; i < 20; ++i)
    {
        if (processtable[i].pid > 0)
            kill(processtable[i].pid, SIGTERM);
    }
	exit(1);

    // clear all three message queues
    if (msgctl(msgq, IPC_RMID, NULL) == -1) {
        perror("Failed to clear message queue ('msgq')");
    }

    // cleans shared memory in case of early termination
    shmdt(sharedMem);
}

static int setupinterrupt(void) {
	struct sigaction act;
	act.sa_handler = myhandler;
	act.sa_flags = 0;
	return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL) || sigaction(SIGPROF, &act, NULL));
}

static int setupitimer(void) {
	struct itimerval value;
	value.it_interval.tv_sec = 60;  // terminate after 60 seconds
	value.it_interval.tv_usec = 0;
	value.it_value = value.it_interval;
	return (setitimer(ITIMER_PROF, &value, NULL));
}

/* used to display the process table (initialized with only 0's) */
void displayProcessTable(int seconds, int nanoseconds)
{
    std::cout << "\nOSS PID: " << getpid() << " SysClockS: " << seconds
        << " SysClockNano: " << nanoseconds << "\n";
    std::cout << "Process Table:\n";
    std::cout << "-----------------------------------------\n";
    std::cout << "Entry Occupied  PID     StartS     StartN\n";
    std::cout << "-----------------------------------------\n";

    for (int i = 0; i < 20; ++i) {                       // creates process 20 entries long
        std::cout << i << (i < 10 ? "      " : "     ")  // changes formatting for "i" < 10
            << std::setw(1) << processtable[i].occupied
            << std::setw(12) << processtable[i].pid
            << std::setw(7) << processtable[i].startSeconds << "s"
            << std::setw(10) << processtable[i].startNano << "ns\n";
    }

    std::cout << "-----------------------------------------\n";
}


int main(int argc, char** argv)
{
    if (signal(SIGTERM, myhandler) == SIG_ERR) {  // sets up sighandler
        perror("signal");
        return 1;
    }

    if (setupinterrupt() == -1) {                // sets up interrupt
		perror("Failed to set up handler for SIGPROF");
		return 1;
	}

	if (setupitimer() == -1) {                   // sets up real-life timer
		perror("Failed to set up the ITIMER_PROF interval timer");
		return 1;
	}

    int opt;
    key_t key = 859047;                                            // child and parent agree on key
    shmid = shmget(key, sizeof(SharedTime), IPC_CREAT | 0666); // creates shared memory
    
    if (shmid == -1) {  // outputs if there was an error connecting to shared memory
        perror("Error in parent: shmget");
        return 1;
    }

    // attaches shared memory to the process
    SharedTime* sharedMem = (SharedTime*)shmat(shmid, nullptr, 0);
    if (sharedMem == (SharedTime*)-1) {
        perror("Error in parent: shmat");
        return 1;
    }

    /* sets default options if user doesn't set one */
    int nValue = 5;       // # of users
    int sValue = 5;       // # of allowed users at a given time
    int tValue = 1;       // calls a rand time (sec and nano) between 1 and given #
    int iValue = 100;     // sets interval to launch children between
    srand(time(0));       // uses a seed for random numbers

    int max_burst_time = 10;
    int quantum = 5000000;  

    /* used for printing every half a second */
    bool flag = false;
    int previousSecond = 0;

    sharedMem->seconds = 0;
    sharedMem->nanoseconds = 0;

    SharedTime sharedTime{0, 0};

    /* highest priority (10 ms), medium priority  (20 ms), lowest priority  (40 ms) */
    const int NUM_QUEUES = 3;
    queue<Process> queues[NUM_QUEUES];

    int message_q = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

    generateProcesses(queues, nValue, max_burst_time, iValue);

    while (true)
    {

        if (nValue <= 0) {
            break;
        }

        /* prints every half second */
        if (sharedTime.seconds >= previousSecond)
        {
            std::cout << "Time: " << sharedTime.seconds << " seconds, " << sharedTime.nanoseconds << " nanoseconds" << std::endl;
            previousSecond++;  // updates the seconds
            flag = true;
        }
        else if(sharedTime.nanoseconds >= 500000000  && flag)
        {
            std::cout << "Time: " << sharedTime.seconds << " seconds, " << sharedTime.nanoseconds << " nanoseconds" << std::endl;
            flag = false;
        }

        incrementClock(sharedTime);                  // increments shared Time

        for (int i = 0; i < sValue; ++i)            // 'sValue' used to set max allowed processes at a time
        {
            sharedTime.nanoseconds += iValue;        // processes enter system at intervals of 'i' nanoseconds

            pid_t childPid = fork();

            if (childPid == -1)
            {
                perror("Error: could not locate 'worker' process");
                exit(EXIT_FAILURE);
            }
            else if (childPid == 0)
            {
                message msg;
                msg.value = pidTracker;

                msgsnd(message_q, &msg, sizeof(message) - sizeof(long), 0);
                cout << "OSS: Dispatching process with PID " << msg.value + 1 << " (" << getpid() << ") at time "  <<
                    sharedTime.seconds << ":" << sharedTime.nanoseconds << endl;

                roundRobinScheduling(queues, quantum);

                execlp("./worker", "worker", nullptr);
                exit(1);
            }
            else if (childPid > 0)
            {
                
            }
            else
            {
                cerr << "Error: Worker fork failed\n";
                return 1;
            }
        }
        
        int finishedProcess = 0;

        while (finishedProcess < sValue) {
            int status;

            int pid = waitpid(-1, &status, WNOHANG);  // non-blocking wait call
            if (pid == -1)
            {
                std::cerr << "Error: waitpid failure (closing)\n";  // error-handling for ending processes
                return 1;
            }
            else if (pid > 0)
            {
                ++finishedProcess;

                message msg;
                msgrcv(msgq, &msg, sizeof(message) - sizeof(long), 1, 0);

                cout << "OSS: Receiving message with PID " << msg.value + finishedProcess << 
                    " after running for " << sharedTime.nanoseconds <<  " nanoseconds" << endl;

                pidTracker++;
                nValue--;           // subtract from total user number
            }
        }

    }

    // clear all three message queues
    if (msgctl(message_q, IPC_RMID, NULL) == -1) {
        perror("failed to clear the message queue");
    }

    // cleans shared memory
    shmdt(sharedMem);

    return 0;
}