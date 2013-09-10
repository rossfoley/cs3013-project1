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

struct job {
    int pid;
    char* command;
    struct timeval startTime;
};

struct jobList {
    struct job *job;
    struct jobList *nextJob;
};

long computeTimeDifference(struct timeval beforeTime, struct timeval afterTime);
void printChildStatistics(struct rusage childStats, struct timeval beforeTime, struct timeval afterTime);
struct jobList* storeBackgroundJob(struct jobList *jobs, int pid, char* command, struct timeval startTime);
void printBackgroundJobs(struct jobList *jobs, int index);
void processBackgroundJobs(struct jobList *jobs, struct jobList **firstJob);
void printJobInfo(struct jobList *current, struct jobList *target, int index);
void removeBackgroundJob(struct jobList *target, struct jobList **current);

int main(int argc, char* argv[]) {
    struct jobList *backgroundJobs = malloc(sizeof(struct jobList));

    while (1) {

        /*****************************
        * User Prompt and User Input *
        *****************************/

        printf("-> ");

        char userInput[129];

        // If fgets returns NULL, then we reached the end of file
        if (fgets(userInput, 129, stdin) == NULL) {
            processBackgroundJobs(backgroundJobs, &backgroundJobs);
            if (!(backgroundJobs->job == NULL && backgroundJobs->nextJob == NULL)) {
                printf("There are still background jobs that haven't completed.\n");
                printf("Waiting for them to complete.\n\n");
                while (!(backgroundJobs->job == NULL && backgroundJobs->nextJob == NULL)) {
                    processBackgroundJobs(backgroundJobs, &backgroundJobs);
                }
            }
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

        /*******************
        * Special Commands *
        *******************/

        int inBackground = FALSE;

        // Check to see if this is a background command
        if (strcmp(arguments[lastArg], "&") == 0) {
            arguments[lastArg] = NULL;
            inBackground = TRUE;
        }

        // Extract the command name and the list of arguments
        char* commandName = arguments[0];

        // If the user typed the exit command, exit the shell
        if (strcmp(commandName, "exit") == 0) {
            processBackgroundJobs(backgroundJobs, &backgroundJobs);
            if (!(backgroundJobs->job == NULL && backgroundJobs->nextJob == NULL)) {
                printf("There are still background jobs that haven't completed.\n");
                printf("Waiting for them to complete.\n\n");
                while (!(backgroundJobs->job == NULL && backgroundJobs->nextJob == NULL)) {
                    processBackgroundJobs(backgroundJobs, &backgroundJobs);
                }
            }
            printf("Goodbye!\n");
            exit(0);   
        }

        // If the user typed the cd command, switch to the specified directory
        if (strcmp(commandName, "cd") == 0) {
            processBackgroundJobs(backgroundJobs, &backgroundJobs);
            chdir(arguments[1]);
            continue;
        }

        // If the user typed the jobs command, display the list of background jobs
        if (strcmp(commandName, "jobs") == 0) {
            processBackgroundJobs(backgroundJobs, &backgroundJobs);
            printBackgroundJobs(backgroundJobs, 1);
            continue;
        }

        /**********************************
        * Forking and Running the Command *
        **********************************/

        // Fork a child process and get its PID
        int pid = fork();

        // Determine whether we are in the parent or child process
        if (pid != 0) {
            // We are in the parent process
            int status, result, showStats = TRUE;
            struct timeval beforeTime, afterTime;
            struct rusage childStats;

            // Check to see if any of the background jobs finished
            processBackgroundJobs(backgroundJobs, &backgroundJobs);

            // Get the time information before running the command
            gettimeofday(&beforeTime, NULL);

            // Wait for the child to finish running the command
            if (!inBackground) {
                result = wait4(pid, &status, 0, &childStats);
            } else {
                result = wait4(pid, &status, WNOHANG, &childStats);
                if (result <= 0) {
                    // Background task isn't done, so store it in the linked list
                    struct jobList *newJob = storeBackgroundJob(backgroundJobs, pid, commandName, beforeTime);

                    // Print the info about the newly created job
                    printJobInfo(backgroundJobs, newJob, 1);

                    // Don't show the statistics since it hasn't finished
                    showStats = FALSE;
                }
            }

            // If the child terminated normally, print the statistics
            if (showStats && WEXITSTATUS(status) == 0) {
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

/************
* Functions *
************/

// Add a new job to the linked list of existing background jobs
struct jobList* storeBackgroundJob(struct jobList *jobs, int pid, char* command, struct timeval startTime) {
    // Malloc a new job and set its PID and command
    struct job *newJob = malloc(sizeof(struct job));
    newJob->pid = pid;
    newJob->command = strdup(command);
    newJob->startTime = startTime;

    // Check to see if there is already a job in the job list
    if (jobs->job != NULL) {
        // If there is a job already, find the last job
        struct jobList *current = jobs;
        while (current->nextJob != NULL) {
            current = current->nextJob;
        }

        // Malloc a new jobList and set it as the next job in the linked list
        struct jobList *newJobList = malloc(sizeof(struct jobList));
        newJobList->job = newJob;
        newJobList->nextJob = NULL;

        current->nextJob = newJobList;
        return newJobList;
    } else {
        // This is the first job, so just set the values of the specified jobList
        jobs->job = newJob;
        jobs->nextJob = NULL;
        return jobs;
    }
}

// Print the list of background jobs
void printBackgroundJobs(struct jobList *current, int index) {
    if (current != NULL && current->job != NULL) {
        printf("[%i] %i %s\n", index, current->job->pid, current->job->command);
        printBackgroundJobs(current->nextJob, index + 1);
    }
}

void printJobInfo(struct jobList *current, struct jobList *target, int index) {
    if (current == target) {
        printf("[%i] %i %s\n", index, target->job->pid, target->job->command);
    } else if (current != NULL) {
        printJobInfo(current->nextJob, target, index + 1);
    }
}

void processBackgroundJobs(struct jobList *jobs, struct jobList **firstJob) {
    if ((jobs != NULL) && (jobs->job != NULL)) {
        int status, result;
        struct rusage stats;
        struct timeval endTime;
        result = wait4(jobs->job->pid, &status, WNOHANG, &stats);
        if (result > 0) {
            // Print the stats
            gettimeofday(&endTime, NULL);
            printf("Job \"%s\" with PID %i has finished.\n", jobs->job->command, jobs->job->pid);
            printChildStatistics(stats, jobs->job->startTime, endTime);
            // Remove from linked list
            removeBackgroundJob(jobs, firstJob);
        }
        processBackgroundJobs(jobs->nextJob, firstJob);
    }
}

void removeBackgroundJob(struct jobList *target, struct jobList **current) {
    if ((*current)->nextJob == NULL) {
        // There is only one item in the list
        (*current)->job = NULL;
    } else if (target == (*current)) {
        // We are removing the first item
        *current = (*current)->nextJob;
    } else if (target == (*current)->nextJob) {
        (*current)->nextJob = target->nextJob;
        free(target->job->command);
        free(target->job);
        free(target);
    } else {
        removeBackgroundJob(target, &(*current)->nextJob);
    }
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
