#include "oss.h"

int main(int argc, char** argv) {

    int input1, input2;

    /* if parameters are set incorrectly by user, it sets defaults instead*/
    if (argc != 3) {
        input1 = 5;       // default seconds
        input2 = 500000;  // default nanoseconds
    } else {
        input1 = std::atoi(argv[1]);  // seconds
        input2 = std::atoi(argv[2]);  // nanoseconds
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

    int prev_sec = sharedMem->seconds;
    int prev_ns = sharedMem->nanoseconds;
    int target_sec = prev_sec + input1;
    int target_ns = prev_ns + input2;

    SharedTime init_Time{prev_sec, prev_ns};  // initializes clock with shared time
    sharedMem->seconds = init_Time.seconds;
    sharedMem->nanoseconds = init_Time.nanoseconds;
    int previousSeconds = -1;                 // initalize seconds with a time impossible to have

    int i = 0;  // used to track seconds

    do {
		incrementClock(init_Time);  // checks the simulated time

        // updates the shared time
        sharedMem->seconds = init_Time.seconds;
        sharedMem->nanoseconds = init_Time.nanoseconds;

        i++;                                  // increases seconds
        previousSeconds = init_Time.seconds;  // keeps track of most recent second which has passed
			
    } while (init_Time.seconds < target_sec || (init_Time.seconds == target_sec && init_Time.nanoseconds < target_ns));

    // updates shared time accordingly after process is finished
    sharedMem->seconds = init_Time.seconds;
    sharedMem->nanoseconds = init_Time.nanoseconds;

	// printf("Child %d is ending\n",getpid());

	return 0;

    shmdt(sharedMem);

    return EXIT_SUCCESS;
}
