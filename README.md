[b]How to Run[/b]:

To run the program, download all the files into the Linux operating system of your choice and port the files into their own directory. In this example, WSL Ubuntu is utlized to run the program.

With the Makefile, run the 'make' command to compile all of the files within the directory.

![makefilewhatevr](https://github.com/ThomasCholak/OS_Project4/assets/63080803/1b86b49e-8aae-4874-a365-82190ddaf14a)

To run the commands, you can use '-h' to output help information, '-n' is used to control the total number of workers intialized, '-s' is used to control how many workers are allowed to run simultanously, and '-t' controls the time limit per user (for example setting it as '5' would set a random interval between 1 and '5'), and '-i' sets the interval between which children launch in nanoseconds. The '-f' paramemeter is used for renaming the file you'd like the output messages from OSS to go to.

An example fo how to run this program is:

'./oss -n 5 -s 5 -i 100 -t 2 -n logfile'

Otherwise you can run as just './oss' and the system will generate defaults for the user.

![logfilegenerated](https://github.com/ThomasCholak/OS_Project4/assets/63080803/4fbdc648-6530-4373-89fa-dbd0973522de)

This will generate a logfile which you can read to anaylze system times, which processes are dead or alive, and the PIDs.

An example of one of the generated output tables is as follows:

![operatingsystems](https://github.com/ThomasCholak/OS_Project4/assets/63080803/d65e9117-1407-4a9f-8962-fd702606be13)
