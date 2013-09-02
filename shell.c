#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

long computeTimeDifference(struct timeval beforeTime, struct timeval afterTime);
void printChildStatistics(struct rusage childStats, struct timeval beforeTime, struct timeval afterTime);

int main(int argc, char* argv[]) {
    while (1) {
        printf("-> ");
        char userInput[128];
        fgets(userInput, 128, stdin);

        char* arguments[32];
        char* argument = strtok(userInput, " \n");

        int i;
        for (i = 0; i < 32 && argument != NULL; i++) {
            arguments[i] = strdup(argument);
            argument = strtok(NULL, " \n");
        }

        if (i >= 32) {
            // There are too many arguments provided
            printf("You are only allowed to use 32 arguments!\n");
            continue;
        }

        // Check to see if a command was actually specified
        if (i < 1) {
            // No command specified, so print an error and exit
            printf("You must specify the command to run!\n");
            continue;
        }

        // Extract the command name and the list of arguments
        char* commandName = arguments[0];
        printf("this is what we think the command is: %s\n", commandName);
        int j;
        for (j = 0; j < i; j++) {
            printf("Argument %i: %s\n", j, arguments[j]);
        }

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

            // If the child terminated normally, print the statistics
            if (WEXITSTATUS(status) == 0) {
                // Get the time right after the child process finished
                gettimeofday(&afterTime, NULL);

                // Get the statistics of the child process that just finished
                struct rusage childStats;
                getrusage(RUSAGE_CHILDREN, &childStats);

                // Print the statistics
                printChildStatistics(childStats, beforeTime, afterTime);
            }

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
    }
    return 0;
}

// Compute the difference between the two specified timevals
long computeTimeDifference(struct timeval beforeTime, struct timeval afterTime) {
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

    // Convert to milliseconds and return the result
    return (difference / 1000);
}

// Print the statistics about the child process with the given rusage data
void printChildStatistics(struct rusage childStats, struct timeval beforeTime, struct timeval afterTime) {
    long difference = computeTimeDifference(beforeTime, afterTime);
    long userCPUTime = (childStats.ru_utime.tv_sec * 1000) + (childStats.ru_utime.tv_usec / 1000);
    long sysCPUTime = (childStats.ru_stime.tv_sec * 1000) + (childStats.ru_stime.tv_usec / 1000);

    printf("Wall-Clock time: %li milliseconds\n", difference);
    printf("User CPU time: %li milliseconds\n", userCPUTime);
    printf("System CPU time: %li milliseconds\n", sysCPUTime);
    printf("Voluntary context switches: %li\n", childStats.ru_nvcsw);
    printf("Involuntary context switches: %li\n", childStats.ru_nivcsw);
    printf("Page faults: %li\n", childStats.ru_minflt + childStats.ru_majflt);
    printf("Page faults that could be satisfied with unreclaimed pages: %li\n", childStats.ru_majflt);
}

