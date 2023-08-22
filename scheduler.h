/*
 * Utilize "scheduler.h" and "scheduler.c" for all the utility functions students
 * intend to use for "interface.c".
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <pthread.h>

#include "interface.h"

#endif

#define FCFS    0
#define SRJF    1

typedef struct queue_entry {
    int tid;
    float current_time;
    int remaining_time;
    int device_id;
    struct queue_entry *prev;
    struct queue_entry *next;
} queue_entry_t;

typedef struct queue {
    queue_entry_t *first;
    queue_entry_t *last;
} queue_t;

/* scheduler fields */
float global_time;
float global_IO_1_time;
float global_IO_2_time;
int queued_thread_count;
int queued_IO_1_count;
int queued_IO_2_count;
int total_thread_count;
int CPU_tid;
int IO_1_tid;
int IO_2_tid;
int CPU_policy;
int IO_policy;

/* scheduler queues */
queue_t *CPU_queue;
queue_t *IO_1_queue;
queue_t *IO_2_queue;

/* pthread variables */
pthread_mutex_t end_me_mutex;
pthread_mutex_t schedule_next_mutex;
pthread_mutex_t mutex;
pthread_cond_t CPU_cond;
pthread_cond_t IO_1_cond;
pthread_cond_t IO_2_cond;
pthread_cond_t all_threads_arrived;

/* functions */
extern void schedule_next();
extern void schedule_CPU(queue_entry_t *CPU_task);
extern void schedule_IO(queue_entry_t *IO_task);
extern void enqueue(int policy, queue_t *queue, float current_time, int tid, int remaining_time, int device_id);
extern queue_entry_t *dequeue(int policy, queue_t *queue);
