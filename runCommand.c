#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

void printTimeDifference(struct timeval beforeTime, struct timeval afterTime);

int main(int argc, char* argv[]) {
	// Check to see if a command was actually specified
	if (argc < 2) {
		// No command specified, so print an error and exit
		printf("You must specify the command to run!\n");
		exit(1);
	}

	// Extract the command name and the list of arguments
	char* commandName = argv[1];
	char** arguments = &argv[1];
	
	// Fork a child process and get its PID
	int pid = fork();

	// Determine whether we are in the parent or child process
	if (pid != 0) {
		// We are in the parent process
		int status;
		struct timeval beforeTime, afterTime;

		// Get the time information before running the command
		gettimeofday(&beforeTime, NULL);

		// Wait for the child to finish running the command
		waitpid(pid, &status, 0);

		// Get the time information after running the command and print the info
		gettimeofday(&afterTime, NULL);
		printTimeDifference(beforeTime, afterTime);
	} else {
		// We are in the child process
		// Run the command and get its result
		int result = execvp(commandName, arguments);

		// Check if the command failed
		if (result == -1) {
			// The command failed, so print out he error number and exit
			printf("Invalid command!\nError Number: %i\n", errno);
			exit(1);
		}
	}
	
	return 0;
}

// Print out the difference between the two specified timevals
void printTimeDifference(struct timeval beforeTime, struct timeval afterTime) {
	// Get the seconds difference and convert to microseconds
	long difference = (long) ((afterTime.tv_sec - beforeTime.tv_sec) * 1000000);

	// Get the microseconds difference
	long microDifference = (long) (afterTime.tv_usec - beforeTime.tv_usec);

	// Correct the difference when the after microseconds > before microseconds
	if (microDifference < 0) {
		microDifference += 1000000;
	}

	// Add the corrected microsecond difference to the total
	difference += microDifference;

	// Convert to milliseconds and print the result
	difference /= 1000;
	printf("Wall-Clock time: %li milliseconds\n", difference);
}

