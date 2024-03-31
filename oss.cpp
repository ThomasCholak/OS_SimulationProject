/*
    Project 4
    Author: Thomas Cholak
    Due Date: 21 March 2024
*/

#include <iomanip>      /* just used for PCB formatting */
#include <fcntl.h>      /* needed to import: O_CREAT */
#include <cstdlib>      /* needed to generate random numbers */
#include <ctime>        /* used for seeding the time */
#include <errno.h>      /* used for signal  error handling */
#include <signal.h>     /* used for signal handling */
#include <sys/time.h>   /* used for keeping track of real-life seconds */
#include <sys/msg.h>    /* message queues */

#include "oss.h"        /* imports header file "oss.cpp" */

/* global variables for share mem and msg queues */
int msgq;
int shmid;
SharedTime* sharedMem;

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
    int nValue = 2;     // # of users
    int sValue = 2;     // # of allowed users at a given time
    int tValue = 1;     // calls a rand time (sec and nano) between 1 and given #
    int iValue = 0;   // sets interval to launch children between
    srand(time(0));     // uses a seed for random numbers

    /* used for printing every half a second */
    bool flag = false;
    int previousSecond = 0;

    /* initialize shared time */
    SharedTime sharedTime{0, 0};

    /* highest priority (10 ms)
       medium priority  (20 ms)
       lowest priority  (40 ms) */

    int q0 = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    int q1 = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    int q2 = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

    while (true)
    {
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

        for (int i = 0; i <= sValue; ++i)            // 'sValue' used to set max allowed processes at a time
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
                for (int j = 0; j < 5; ++j) {
                    message msg;
                    msg.value = i * 5 + j;
                    if (i == 0)
                    {
                        msg.priority = 0;  // high priority
                        msgsnd(q0, &msg, sizeof(message) - sizeof(long), 0);
                        std::cout << "Sent message with value1 " << msg.value << " from Process " << getpid() << std::endl;
                    }
                    else if (i == 1)
                    {
                        msg.priority = 1;  // medium priority
                        msgsnd(q1, &msg, sizeof(message) - sizeof(long), 0);
                        std::cout << "Sent message with value2 " << msg.value << " from Process " << getpid() << std::endl;
                    }
                    else
                    {
                        msg.priority = 2;  // low priority
                        msgsnd(q2, &msg, sizeof(message) - sizeof(long), 0);
                        std::cout << "Sent message with value3 " << msg.value << " from Process " << getpid() << std::endl;
                    }
                }

                execlp("./worker", "worker", nullptr);
                exit(1);
            }
            else if (childPid > 0)
            {

            }
            else
            {
                std::cerr << "Error: Worker fork failed\n";
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
                message msg;
                msgrcv(msgq, &msg, sizeof(message) - sizeof(long), 1, 0); // Receive only messages of type 1
                std::cout << "Received message with value " << msg.value << " in Process " << getpid() << std::endl;
                wait(0);            // provide child time to clear out of the system

                ++finishedProcess;
                nValue--;           // subtract from total user number
            }
        }

        if (nValue == 0) {
            break;
        }
    }


    // clear all three message queues
    if (msgctl(q0, IPC_RMID, NULL) == -1) {
        perror("Failed to clear the low-priority message queue");
    }
    if (msgctl(q1, IPC_RMID, NULL) == -1) {
        perror("Failed to clear the medium-priority message queue");
    }
    if (msgctl(q2, IPC_RMID, NULL) == -1) {
        perror("Failed to clear the low-priority message queue");
    }

    // cleans shared memory
    shmdt(sharedMem);

    return 0;
}