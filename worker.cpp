#include "oss.h"

int main(int argc, char** argv) {

    int randSec, randNano;

    /* if parameters are set incorrectly by user, it sets defaults instead*/
    if (argc < 5) {
        randSec = 5;       // default seconds
        randNano = 500000;  // default nanoseconds
    } else {
        randSec = std::atoi(argv[1]);  // seconds
        randNano = std::atoi(argv[2]);  // nanoseconds
    }

    key_t key = 859047;  // child uses same key as parent
    int shmid = shmget(key, sizeof(SharedTime), IPC_CREAT | 0666);

    if (shmid == -1) {
        perror("Error in child: shmget");
        return 1;
    }

    SharedTime* sharedMem = (SharedTime*)shmat(shmid, nullptr, 0);
    if (sharedMem == (SharedTime*)-1) {
        perror("Error in child: shmat");
        return 1;
    }

    int parentSec = std::stoi(argv[3]);
    int parentNano = std::stoi(argv[4]);

    int target_sec = parentSec + randSec;
    int target_ns = parentNano + randNano;

    SharedTime init_Time{parentSec, parentNano};
    int previousSeconds = -1;   // initalize seconds with a time impossible to have

    int i = 0;  // used to track seconds

    // initial time
    std::cout << "WORKER PID:" << getpid() << " PPID:" << getppid() << " SysClockS: " << init_Time.seconds <<
        " SysclockNano: " << init_Time.nanoseconds << " TermTimeS: " << randSec << " TermTimeNano: " << randNano << "ns" << 
        " --Just Starting\n";

    while (init_Time.seconds < target_sec || (init_Time.seconds == target_sec && init_Time.nanoseconds < target_ns)) {
        incrementClock(init_Time);

        if (init_Time.seconds != previousSeconds) {
            if (i >= 1) {
                std::cout << "WORKER PID:" << getpid() << " PPID:" << getppid() << " SysClockS: " << init_Time.seconds <<
                    " SysclockNano: " << init_Time.nanoseconds << " TermTimeS: " << randSec << " TermTimeNano: " << randNano << "ns" << 
                    " --" << i << " seconds have passed since starting\n";
            }
            i++;        // increases seconds
            previousSeconds = init_Time.seconds;
        }
    }

    // final time
    std::cout << "WORKER PID:" << getpid() << " PPID:" << getppid() << " SysClockS: " << init_Time.seconds <<
        " SysclockNano: " << init_Time.nanoseconds << " TermTimeS: " << randSec << " TermTimeNano: " << randNano << "ns" << 
        " --Terminating\n";

    // worker reports back ending time
    sharedMem->seconds = init_Time.seconds;
    sharedMem->nanoseconds = init_Time.nanoseconds;

    return EXIT_SUCCESS;
}
