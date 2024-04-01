#include "oss.h"

int main(int argc, char** argv) {

    int randSec, randNano;

    /* if parameters are set incorrectly by user, it sets defaults instead*/
    if (argc < 5) {
        randSec = 5;        // default seconds
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

    shmdt(sharedMem);

    return EXIT_SUCCESS;
}
