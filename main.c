/*
 * This is the main execution file which handles your parsing of the
 * input file and execution of the interfaces as defined. Please peruse
 * this file for reference and understanding of how the interfaces are called.
 *
 * NOTE: DO NOT CHANGE THIS FILE. THIS WILL BE OVERWRITTEN EVEN
 * IF ANY CHANGES ARE MADE HERE.
 */
#include <sys/stat.h>
#include <libgen.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "interface.h"

#define MAX_LINE_LEN 1024
FILE *gantt_file;

struct thread_struct {
    pthread_t p_t;           // pthread identifier
    int tid;                 // tid
    char *line; // tid's operations
};

void *thread_start(void *);

int get_line_count(char *file_name);

// Main function
// Read input file and create threads accordingly
int main(int argc, char **argv) {
    printf("%s: Hello!\n", __func__);
    if (argc != 2) {
        fprintf(stderr, "Not enough parameters specified. Usage: ./proj1 <input_file>\n");
        return -EINVAL;
    }

    // Get parameters
    int num_lines = get_line_count(argv[1]);
    if (num_lines <= 0) {
        fprintf(stderr, "%s: invalid input file.\n", __func__);
        return -EINVAL;
    }

    int num_threads = num_lines;
    printf("%s: number of threads: %d\n", __func__, num_threads);

    // Allocate thread_struct
    struct thread_struct *threads;
    threads = (struct thread_struct *) malloc(sizeof(*threads) * num_threads);
    if (!threads) {
        perror("malloc() error");
        return errno;
    }
    memset(threads, 0, sizeof(*threads) * num_threads);

    // Read each line and save inside threads[].line
    FILE *fp = fopen(argv[1], "r");
    ssize_t nread;
    size_t len = 0;
    int i = 0;
    while((nread = getline(&(threads[i].line), &len, fp)) != -1) {
        if (threads[i].line[nread - 1] == '\n')
            threads[i].line[nread - 1] = '\0'; // remove newline

        i++;
        if (i >= num_threads) {
            break;
        }
    }
    fclose(fp);

    // Open file for Gantt chart
    char temp[512] = {0};
    mkdir("output", 0755);
    sprintf(temp, "output/gantt-%s", basename(argv[1]));
    gantt_file = fopen(temp, "w");
    if (gantt_file == NULL) {
        perror("fopen() error");
        return errno;
    }

    // Init scheduler
    init_scheduler(num_threads);

    // Assign tid and create threads using threads[]
    int ret = 0;
    for (int i = 0; i < num_threads; ++i) {
        threads[i].tid = i;
        ret = pthread_create(&(threads[i].p_t), NULL, thread_start, &(threads[i]));
        if (ret) {
            fprintf(stderr, "%s: pthread_create() error!\n", __func__);
            return -EPERM;
        }
    }

    // Join threads
    for (int i = 0; i < num_threads; ++i) {
        ret = pthread_join(threads[i].p_t, NULL);
        if (ret) {
            fprintf(stderr, "%s: pthread_join() error!\n", __func__);
            return -EPERM;
        }
    }

    fclose(gantt_file);
    free(threads);

    printf("%s: Output file: %s\n", __func__, temp);
    printf("%s: Bye!\n", __func__);
    return 0;
}

// Thread starting point
// Independently read each line and call C/I/E
void *thread_start(void *arg) {
    struct thread_struct *my_info = (struct thread_struct *) arg;
    int tid = my_info->tid;

    // read tokens
    char *token = NULL;
    char delim[3] = "\t ";
    char *saveptr;

    // arrival time
    token = strtok_r(my_info->line, delim, &saveptr);
    float arrival_time = atof(token);

    // the first operation (C/I/P/V) call from this thread is the arrival time in input file
    float schedule_time = arrival_time;

    // tid
    token = strtok_r(NULL, delim, &saveptr);
    if (tid != atoi(token)) {
        fprintf(stderr, "%s: tid: %d, incorrect tid\n", __func__, tid);
        exit(EXIT_FAILURE);
    }

    // loop until 'E'
    token = strtok_r(NULL, delim, &saveptr);
    while (token) {
        // save the return value (time) of C/I
        int ret_time = 0;

        // parse token
        if (token[0] == 'C') {
            int duration = atoi(&(token[1]));
            while (duration >= 0) {
                ret_time = cpu_me(schedule_time, tid, duration);
                // return from cpu_me()
                if (duration > 0) {
                    // only print when CPU is actually requested
                    // (if duration is 0, we are just notifying the scheduler)
                    // this tid had cpu from 'ret_time-1' to 'ret_time'
                    fprintf(gantt_file, "%3d~%3d: T%d, CPU\n", ret_time - 1, ret_time, tid);
                    printf("%3d~%3d: T%d, CPU\n", ret_time - 1, ret_time, tid);
                }

                // values for the next cpu_me() call
                schedule_time = ret_time;
                duration = duration - 1;
            }
        } else if (token[0] == 'I') {
            int device_id = atoi(&(token[1]));
            ret_time = io_me(schedule_time, tid, device_id);
            // return from io_me()
            // this tid finished IO at time 'ret_time'
            fprintf(gantt_file, "   ~%3d: T%d, Return from IO Device %d\n", ret_time, tid, device_id);
            printf("   ~%3d: T%d, Return from IO Device %d\n", ret_time, tid, device_id);
        } else if (token[0] == 'E') {
            // this thread is finished, notify scheduler
            end_me(tid);

            // end this thread normally
            return NULL;
        } else {
            fprintf(stderr, "%s: Error, tid: %d, invalid token: %c%c\n", __func__, tid, token[0], token[1]);
            exit(EXIT_FAILURE);
        }

        // get next token
        token = strtok_r(NULL, delim, &saveptr);

        // call the next operation without any time delay
        schedule_time = ret_time;
    }

    // No 'E' found in input file
    fprintf(stderr, "%s: Error, tid: %d, thread finished without 'E' operation\n", __func__, tid);
    exit(EXIT_FAILURE);
}

// From file_name, get the number of lines and do error check
int get_line_count(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    if (!fp) {
        perror("fopen() error");
        return -ENOENT;
    }

    int num_lines = 0;
    char arrival_time[512];
    int temp;

    ssize_t nread;
    size_t len = sizeof(char) * MAX_LINE_LEN;

    char *buf = (char *) malloc(sizeof(char) * MAX_LINE_LEN);
     while((nread = getline(&(buf), &len, fp)) != -1) {
        // Check tid of the input file. tid starts from 0
        sscanf(buf, "%s %d", arrival_time, &temp);
        if (temp != num_lines) {
            return -EINVAL;
        }
        ++num_lines;
    }
    free(buf);

    fclose(fp);
    return num_lines;
}
