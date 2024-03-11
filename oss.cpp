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

#include "oss.h"        /* imports header file "oss.cpp" */

struct PCB {
    int occupied;
    pid_t pid;
    int startSeconds;
    int startNano;
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

/* just used to initialize process table structure*/
void displayProcessTable(int seconds, int nanoseconds);

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
    int shmid = shmget(key, sizeof(SharedTime), IPC_CREAT | 0666); // creates shared memory
    
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

    // initializes clock using shared time
    SharedTime init_Time{0, 0};

    /* sets default options if user doesn't set one */
    int nValue = 5;     // # of users
    int sValue = 2;     // # of allowed users at a given time
    int tValue = 2;     // calls a rand time (sec and nano) between 1 and given #
    int iValue = 100;   // sets interval to launch children between
    srand(time(0));     // uses a seed for random numbers

    int previousSeconds = -1;
    int previousNanoseconds = -1;
    int randSec, randNano;  // empty ints to generate random numbers inside

    while (nValue > 0)
    {
        incrementClock(init_Time);

        int remainingUsers = std::min(nValue, sValue);  // determine number of remaining users

        for (int i = 0; i < remainingUsers; ++i)
        {
            pid_t childPid = fork();

            init_Time.nanoseconds += iValue;

            if (childPid == -1)
            {
                perror("Error: could not find user process");
                exit(EXIT_FAILURE);
            }
            else if (childPid == 0)
            {
                randSec = rand() % (tValue + 1);
                randNano = rand() % 1000000000;

                std::string randomSecStr = std::to_string(randSec);               // how long to run (secs)
                std::string randomNanoStr = std::to_string(randNano);             // how long to run (nano)
                std::string parentSec = std::to_string(init_Time.seconds);        // time provided from parent (sec)
                std::string parentNano = std::to_string(init_Time.nanoseconds);   // time provided form parent (nano)
                
                execl("./worker", "worker", randomSecStr.c_str(), randomNanoStr.c_str(), parentSec.c_str(), parentNano.c_str(), nullptr);
                exit(1);

            }
            else if (childPid > 0)
            {
                int emptyIndex = -1;
                for (int j = 0; j < sValue; ++j) // sets max allowed values in process table
                {
                    if (processtable[j].occupied == 0)
                    {
                        emptyIndex = j;  // allocated empty indexes
                        break;
                    }
                }
                if (emptyIndex != -1)
                {
                    processtable[emptyIndex].occupied = 1;
                    processtable[emptyIndex].pid = childPid;
                    processtable[emptyIndex].startSeconds = sharedMem->seconds;
                    processtable[emptyIndex].startNano = sharedMem->nanoseconds;
                }
                else
                {
                    std::cerr << "Error: Process table is full\n";
                    return 1;
                }
            }
            else
            {
                std::cerr << "Error: Fork failed\n";
                return 1;
            }
        }
        
        int finishedProcess = 0;

        while (finishedProcess < remainingUsers) {
            int status;
            int pid = waitpid(-1, &status, WNOHANG);  // non-blocking wait call

            if (pid == -1)
            {
                std::cerr << "Error: waitpid failure\n";  // error-handling
                return 1;
            }
            else if (pid > 0) {
                // frees up space in PCB once child process is finished
                for (int j = 0; j < sValue; ++j)
                {
                    if (processtable[j].pid == pid)
                    {
                        processtable[j].occupied = 0;
                        break;
                    }
                }
                ++finishedProcess;
            }
        }

        nValue -= remainingUsers;  // subtracts # of workers in batch from total workers
    }

    // cleans shared memory
    shmdt(sharedMem);

    return 0;
}