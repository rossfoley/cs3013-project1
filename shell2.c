#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define TRUE 1
#define FALSE 0

long computeTimeDifference(struct timeval beforeTime, struct timeval afterTime);
void printChildStatistics(struct rusage childStats, struct timeval beforeTime, struct timeval afterTime);

struct job {
    int pid;
    char* command;
};

struct jobList {
    struct job *job;
    struct jobList *nextJob;
};

int main(int argc, char* argv[]) {
    struct jobList backgroundJobs;

    while (1) {
        printf("-> ");

        char userInput[129];

        // If fgets returns NULL, then we reached the end of file
        if (fgets(userInput, 129, stdin) == NULL) {
            printf("\n");
            exit(0);
        }

        if (strlen(userInput) == 1 && userInput[strlen(userInput) - 1] == '\n') {
            // No command specified, so print an error and exit
            printf("You must specify the command to run!\n");
            continue;
        }

        // If the last user input character isn't a new line then:
        // - The string is more than 128 characters, or
        // - We reached the end of file
        if (userInput[strlen(userInput)-1] != '\n') {
            int tooLong = 0;
            char nextChar;

            // Loop until we reach the end of line or end of file
            while (((nextChar = getchar()) != '\n') && (nextChar != EOF)) {
                tooLong = 1;
            }

            // Check to see if the end of line was reached
            // If so, print an error and ask for new user input
            if (tooLong) {
                printf("Commands must be 128 characters or less!\n");
                continue;
            }
        }

        // Replace the new line character with a null terminator
        userInput[strlen(userInput)-1] = '\0';

        char* arguments[33];
        char* argument = strtok(userInput, " \n");
        int lastArg = 0, foundLastArg = 0;

        // Read in the arguments using strtok()
        for (int i = 0; i < 33; i++) {
            arguments[i] = argument;
            argument = strtok(NULL, " \n");
            if (argument == NULL && !foundLastArg) {
                foundLastArg = TRUE;
                lastArg = i;
            }
        }

        if (arguments[32] != NULL) {
            // There are too many arguments provided
            printf("You are only allowed to use 32 arguments!\n");
            continue;
        }

        int inBackground = FALSE;
        if (strcmp(arguments[lastArg], "&") == 0) {
            printf("You want to run this in the background!\n");
            inBackground = TRUE;
        }

        // Extract the command name and the list of arguments
        char* commandName = arguments[0];

        // If the user typed the exit command, exit the shell
        if (strcmp(commandName, "exit") == 0) {
            printf("Goodbye!\n");
            exit(0);   
        }

        // If the user typed the cd command, switch to the specified directory
        if (strcmp(commandName, "cd") == 0) {
            chdir(arguments[1]);
            continue;
        }

        // Fork a child process and get its PID
        int pid = fork();

        // Determine whether we are in the parent or child process
        if (pid != 0) {
            // We are in the parent process
            int status;
            struct timeval beforeTime, afterTime;
            struct rusage childStats;

            // Get the time information before running the command
            gettimeofday(&beforeTime, NULL);

            // Wait for the child to finish running the command
            if (!inBackground) {
                wait4(pid, &status, 0, &childStats);
            } else {
                wait4(pid, &status, WNOHANG, &childStats);
            }

            // If the child terminated normally, print the statistics
            if (WEXITSTATUS(status) == 0) {
                // Get the time right after the child process finished
                gettimeofday(&afterTime, NULL);

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
                printf("Invalid command!\nError Number: %i\nError Message: %s\n", errno, strerror(errno));
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
    long difference, userCPUTime, sysCPUTime, volContext, involContext, pageFaults, unrecPageFaults;
    difference = computeTimeDifference(beforeTime, afterTime);
    userCPUTime = (childStats.ru_utime.tv_sec * 1000) + (childStats.ru_utime.tv_usec / 1000);
    sysCPUTime = (childStats.ru_stime.tv_sec * 1000) + (childStats.ru_stime.tv_usec / 1000);
    volContext = childStats.ru_nvcsw;
    involContext = childStats.ru_nivcsw;
    pageFaults = childStats.ru_majflt;
    unrecPageFaults = childStats.ru_minflt;

    printf("\n***********************************************************************\n");
    printf("Wall-Clock time: %li milliseconds\n", difference);
    printf("User CPU time: %li milliseconds\n", userCPUTime);
    printf("System CPU time: %li milliseconds\n", sysCPUTime);
    printf("Voluntary context switches: %li\n", volContext);
    printf("Involuntary context switches: %li\n", involContext);
    printf("Page faults: %li\n", pageFaults);
    printf("Page faults that could be satisfied with unreclaimed pages: %li\n", unrecPageFaults);
    printf("***********************************************************************\n\n");
}
