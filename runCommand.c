#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void printTimeDifference(struct timeval beforeTime, struct timeval afterTime);

int main(int argc, char* argv[]) {
	if (argc < 2) {
		// No command specified
		printf("You must specify the command to run!\n");
		exit(1);
	}

	char* commandName = argv[1];
	char** arguments = &argv[1];
	
	int pid = fork();
	if (pid != 0) {
		int status;
		struct timeval beforeTime, afterTime;
		gettimeofday(&beforeTime, NULL);
		waitpid(pid, &status, 0);
		gettimeofday(&afterTime, NULL);
		printTimeDifference(beforeTime, afterTime);
	} else {
		execvp(commandName, arguments);
	}
	
	return 0;
}

void printTimeDifference(struct timeval beforeTime, struct timeval afterTime) {
	long difference = (long) ((afterTime.tv_sec - beforeTime.tv_sec) * 1000000);
	long microDifference = (long) (afterTime.tv_usec - beforeTime.tv_usec);
	if (microDifference < 0) {
		microDifference += 1000000;
	}
	difference += microDifference;
	printf("Clock time: %li\n", difference);
}