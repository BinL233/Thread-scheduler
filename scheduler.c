#include "scheduler.h"
#include "pthread.h"

/* scheduler fields */


/* scheduler queues */


/* pthread fields */
pthread_mutex_t schedule_next_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t end_me_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CPU_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t IO_1_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t IO_2_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_threads_arrived = PTHREAD_COND_INITIALIZER;


/* scheduler functions */

void schedule_next() {
    pthread_mutex_lock(&schedule_next_mutex);
    // retrieve task from CPU, IO1, IO2
    queue_entry_t *CPU_task = dequeue(CPU_policy, CPU_queue);
    queue_entry_t *IO_1_task = dequeue(IO_policy, IO_1_queue);
    queue_entry_t *IO_2_task = dequeue(IO_policy, IO_2_queue);
    // sort queues by the min global time
    queue_entry_t *task_list[3];
    float global_time_list[3];
    if (global_time <= global_IO_1_time) {
        if (global_time <= global_IO_2_time) {
            if (global_IO_1_time <= global_IO_2_time) {
                task_list[0] = CPU_task;
                task_list[1] = IO_1_task;
                task_list[2] = IO_2_task;
                global_time_list[0] = global_time;
                global_time_list[1] = global_IO_1_time;
                global_time_list[2] = global_IO_2_time;
            } else {  // global_IO_1_time > global_IO_2_time
                task_list[0] = CPU_task;
                task_list[1] = IO_2_task;
                task_list[2] = IO_1_task;
                global_time_list[0] = global_time;
                global_time_list[1] = global_IO_2_time;
                global_time_list[2] = global_IO_1_time;
            }
        } else {  // global_time > global_IO_2_time
                task_list[0] = IO_2_task;
                task_list[1] = CPU_task;
                task_list[2] = IO_1_task;
                global_time_list[0] = global_IO_2_time;
                global_time_list[1] = global_time;
                global_time_list[2] = global_IO_1_time;             
        }
    } else {  //  global_time > global_IO_1_time
        if (global_time <= global_IO_2_time) {
            task_list[0] = IO_1_task;
            task_list[1] = CPU_task;
            task_list[2] = IO_2_task;
            global_time_list[0] = global_IO_1_time;
            global_time_list[1] = global_time;
            global_time_list[2] = global_IO_2_time;  
        } else {  // global_time > global_IO_2_time
            if (global_IO_1_time <= global_IO_2_time) {
                task_list[0] = IO_1_task;
                task_list[1] = IO_2_task;
                task_list[2] = CPU_task;
                global_time_list[0] = global_IO_1_time;
                global_time_list[1] = global_IO_2_time;
                global_time_list[2] = global_time;  
            } else {  // global_IO_1_time > global_IO_2_time
                task_list[0] = IO_2_task;
                task_list[1] = IO_1_task;
                task_list[2] = CPU_task;
                global_time_list[0] = global_IO_2_time;
                global_time_list[1] = global_IO_1_time;
                global_time_list[2] = global_time;  
            }
        }
    }
    // schedule the queue w/ min global time
    // if the following happen, find the queue w/ the next min global time :
    // * queue is empty
    // * task has current_time later than its global time
    int i;
    queue_entry_t *scheduled_task = NULL;
    for (i=0; i<3; i++) {
        if ((task_list[i] == NULL) || ((task_list[i]->current_time >= global_time_list[i]) && ((i<2 && task_list[i]->current_time > global_time_list[i+1]) || (i==2)))) {
            continue;
        } else {
            scheduled_task = task_list[i];
            break;
        }
    }
    // if all queues are in the above situatios, find nearest current_time
    float nearest_current_time = -1;
    if (scheduled_task == NULL) {
        for (i=0; i<3; i++) {
            if (task_list[i] != NULL) {
                if (nearest_current_time == -1) {
                    nearest_current_time = task_list[i]->current_time;
                    scheduled_task = task_list[i];
                } else if (task_list[i]->current_time < nearest_current_time) {
                    nearest_current_time = task_list[i]->current_time;
                    scheduled_task = task_list[i];
                }
            }    
        }
    }
    // schedule task
    if (scheduled_task->device_id == -1) {
        if (IO_1_task != NULL) {
            enqueue(IO_policy, IO_1_queue, IO_1_task->current_time, IO_1_task->tid, IO_1_task->remaining_time, IO_1_task->device_id);
        }
        if (IO_2_task != NULL) {
            enqueue(IO_policy, IO_2_queue, IO_2_task->current_time, IO_2_task->tid, IO_2_task->remaining_time, IO_2_task->device_id);
        }
        schedule_CPU(scheduled_task);
    } else {
        if (CPU_task != NULL) {
            enqueue(CPU_policy, CPU_queue, CPU_task->current_time, CPU_task->tid, CPU_task->remaining_time, CPU_task->device_id);
        }
        if (scheduled_task->device_id == 1 && IO_2_task != NULL) {
            enqueue(IO_policy, IO_2_queue, IO_2_task->current_time, IO_2_task->tid, IO_2_task->remaining_time, IO_2_task->device_id);
        } else if (scheduled_task->device_id == 2 && IO_1_task != NULL) {
            enqueue(IO_policy, IO_1_queue, IO_1_task->current_time, IO_1_task->tid, IO_1_task->remaining_time, IO_1_task->device_id);
        }
        schedule_IO(scheduled_task);
    }
    pthread_mutex_unlock(&schedule_next_mutex);
    return;
}

void schedule_CPU(queue_entry_t *CPU_task) {
    printf("\tschedule_CPU()\n");

    // update CPU_tid
    CPU_tid = CPU_task->tid;    
    printf("\t\tscheduled T%d\n", CPU_tid);
    printf("\t\t\t...tid=%d, current=%f, remaining=%d\n", CPU_task->tid, CPU_task->current_time, CPU_task->remaining_time);

    // tick global_time to the time the cpu burst finished
    float temp = global_time;
    if (global_time < CPU_task->current_time) {
        global_time = ceil(CPU_task->current_time) + 1;
    } else {
        global_time += 1;
    }
    printf("\t\tticked global_time : %f -> %f\n", temp, global_time);
    // notify corresponding cpu_me() to get ret_time
    pthread_cond_broadcast(&CPU_cond);
    // corresponding cpu_me() updated, finish this tick
    free(CPU_task);
        printf("\t\tremaining tasks:\n");
        queue_entry_t *current_entry = CPU_queue->first;
        while (current_entry != NULL) {
            printf("\t\t\t...tid=%d, current=%f, remaining=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time);
            current_entry = current_entry->next;
        }
    printf("\treturn schedule_CPU(), CPU_tid=%d\n", CPU_tid);
    return;
}

void schedule_IO(queue_entry_t *IO_task) {
    int device_id = IO_task->device_id;
    printf("\tschedule_IO(%d)\n", device_id);
    // determine fields based on device_id
    pthread_cond_t *IO_cond;
    int *IO_tid;
    int IO_device_tick;
    int *queued_IO_count;
    queue_t *IO_queue;
    float *global_IO_time;
    if (device_id == 1) {
        IO_cond = &IO_1_cond;
        IO_tid = &IO_1_tid;
        IO_device_tick = IO_DEVICE_1_TICKS;
        queued_IO_count = &queued_IO_1_count;
        IO_queue = IO_1_queue;
        global_IO_time = &global_IO_1_time;
    } else if (device_id == 2) {
        IO_cond = &IO_2_cond;
        IO_tid = &IO_2_tid;
        IO_device_tick = IO_DEVICE_2_TICKS;
        queued_IO_count = &queued_IO_2_count;
        IO_queue = IO_2_queue;
        global_IO_time = &global_IO_2_time;
    } else {
        printf("\t\tInvalid Device_ID : %d\n", device_id);
        printf("\tend schedule_IO(%d)\n", device_id);
        return;
    }
    
    // update IO_tid
    *IO_tid = IO_task->tid;
        printf("\t\tscheduled T%d\n", *IO_tid);
        printf("\t\t\t...tid=%d, current=%f, device_id=%d\n", IO_task->tid, IO_task->current_time, IO_task->device_id);

    // tick global_IO_x_time to the time the IO burst finished
    float temp = *global_IO_time;
    if (*global_IO_time < ceil(IO_task->current_time)) {
        *global_IO_time = ceil(IO_task->current_time) + IO_device_tick;
    } else {
        *global_IO_time += IO_device_tick;
    }
    
    // notify corresponding io_me() to get ret_time
    pthread_cond_broadcast(IO_cond);
    // corresponding io_me() updated, finish this tick
    free(IO_task);
        printf("\t\tremaining tasks:\n");
        queue_entry_t *current_entry = IO_queue->first;
        while (current_entry != NULL) {
            printf("\t\t...tid=%d, current=%f, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->device_id);
            current_entry = current_entry->next;
        }
    printf("\treturn schedule_IO(%d)\n", device_id);
    return;
}


/* queue functions */

void enqueue(int policy, queue_t *queue, float current_time, int tid, int remaining_time, int device_id) {
    printf("\tenqueue()\n");
            printf("\t\tqueue before enqueue:\n");
            queue_entry_t *current_entry = queue->first;
            while (current_entry != NULL) {
                printf("\t\t\t...tid=%d, current=%f, remaining=%d, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time, current_entry->device_id);
                current_entry = current_entry->next;
            }
    // construct new entry
    queue_entry_t *new_entry = (queue_entry_t *)malloc(sizeof(queue_entry_t));
    new_entry->current_time = current_time;
    new_entry->tid = tid;
    new_entry->remaining_time = remaining_time;
    new_entry->device_id = device_id;
    new_entry->prev = NULL;
    new_entry->next = NULL;
            printf("\t\tnew entry : tid=%d, current=%f, remaining=%d, device_id=%d\n", new_entry->tid, new_entry->current_time, new_entry->remaining_time, new_entry->device_id);

    // enqueue
    current_entry = queue->first;
    // empty queue
    if (current_entry == NULL) {
        printf("\t\t(queue is empty)\n");
        queue->first = new_entry;
        queue->last = new_entry;
        printf("\t\tentry added to queue. resultant queue :\n");
                current_entry = queue->first;
                while (current_entry != NULL) {
                    printf("\t\t\t...tid=%d, current=%f, remaining=%d, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time, current_entry->device_id);
                    current_entry = current_entry->next;
                }
                printf("\tend enqueue()\n");
        return;
    }
    printf("\t\t(queue not empty)\n");
    // non-empty queue, find the appropriate position
    queue_entry_t *prev_entry = NULL;
    current_entry = queue->first;
    // FCFS
    if (policy == FCFS) {
        printf("\t\tFCFS\n");
        while ( (current_entry != NULL) && (
                (current_entry->current_time < current_time) || (current_entry->current_time == current_time &&
                current_entry->tid < tid)
        )) {
            printf("\t\t\t...tid=%d, current=%f, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->device_id);
            prev_entry = current_entry;
            current_entry = current_entry->next;
        }
    }
    // SRJF
    else if (policy == SRJF) {
        printf("\t\tSRJF\n");
        while ( (current_entry != NULL) && 
                ((current_entry->remaining_time < remaining_time) || (current_entry->remaining_time == remaining_time &&
                current_entry->tid < tid)
        )){
            printf("\t\t\t...tid=%d, current=%f, remaining=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time);
            prev_entry = current_entry;
            current_entry = current_entry->next;
        }
    } else {
        printf("\t\tInvalid Policy : %d\n", policy);
        return;
    }
    printf("\t\tposition found\n");

    // insert new entry to that position
    new_entry->next = current_entry;
    new_entry->prev = prev_entry;
    if (prev_entry != NULL) {
        prev_entry->next = new_entry;
        printf("\t\t\tprev_entry : tid=%d, current=%f, remaining=%d, device_id=%d\n", prev_entry->tid, prev_entry->current_time, prev_entry->remaining_time, prev_entry->device_id);
    } else {  // insert at head of queue
        printf("\t\t\tprev_entry : NULL\n");
        queue->first = new_entry;
    }
    if (current_entry != NULL) {
        current_entry->prev = new_entry;
        printf("\t\t\tcurrent_entry : tid=%d, current=%f, remaining=%d, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time, current_entry->device_id);
    } else {  // insert at tail of queue
        printf("\t\t\t\tcurrent_entry : NULL\n");
        queue->last = new_entry;
    }

            printf("\t\t\tentry added to queue. resultant queue :\n");
            current_entry = queue->first;
            while (current_entry != NULL) {
                printf("\t\t\t...tid=%d, current=%f, remaining=%d, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time, current_entry->device_id);
                current_entry = current_entry->next;
            }
    printf("\tend enqueue()\n");
    return;
}

queue_entry_t *dequeue(int policy, queue_t *queue) {
    printf("\t\tdequeue()\n");
    // queue must not be empty
    if (queue->first == NULL) {
        printf("\t\t\tDequeueing an empty queue!\n");
        printf("\t\tend dequeue()\n");
        return NULL;
    }
    // retrieve the first valid task
    queue_entry_t *ret_entry = NULL;
    queue_entry_t *current_entry = NULL;
    queue_entry_t *prev_entry = NULL;
    if (policy == FCFS) {
        printf("\t\t\tFCFS\n");
        ret_entry = queue->first;
    } 
    else if (policy == SRJF) {
        printf("\t\t\tSRJF\n");
        current_entry = queue->first;
        // try to find the first task that is scheduled no later than the global time
        // (enqueue has ensured increasing remaining_time order)
        while (current_entry != NULL) {
            if (current_entry->current_time <= global_time) {
                ret_entry = current_entry;
                break;
            } else {
                prev_entry = current_entry;
                current_entry = current_entry->next;
            }
        }
        // if all tasks are scheduled after the global time,
        // find the task arrived at the nearest tick & has shortest remaining_time among all tasks scheduled during that tick
        if (ret_entry == NULL) {
            printf("\t\t\t\tall tasks are scheduled after the global time\n");
            current_entry = queue->first;
                printf("\t\t\t\t...tid=%d, current=%f, remaining=%d, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time, current_entry->device_id);
            ret_entry = current_entry;
            int nearest_current_time = ceil(current_entry->current_time);
            int shortest_renmaining_time = current_entry->remaining_time;
            int minimum_tid = current_entry->tid;
            current_entry = current_entry->next;
            while (current_entry != NULL) {
                    printf("\t\t\t\t...tid=%d, current=%f, remaining=%d, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time, current_entry->device_id);
                if (    (ceil(current_entry->current_time) < nearest_current_time) || 
                        (ceil(current_entry->current_time) == nearest_current_time && current_entry->remaining_time < shortest_renmaining_time) || 
                        (ceil(current_entry->current_time) == nearest_current_time && current_entry->remaining_time == shortest_renmaining_time && current_entry->tid < minimum_tid)
                ) {
                    nearest_current_time = ceil(current_entry->current_time);
                    shortest_renmaining_time = current_entry->remaining_time;
                    minimum_tid = current_entry->tid;
                    ret_entry = current_entry;
                }
                current_entry = current_entry->next;
            }
        }
    } else {
        printf("\t\t\tInvalid Policy : %d\n", policy);
        return NULL;
    }
    // update adjacent entries' neighbor pointers
    current_entry = ret_entry->next;
    prev_entry = ret_entry->prev;
    if (prev_entry != NULL) {
        prev_entry->next = current_entry;
        printf("\t\t\t\tprev_entry : tid=%d, current=%f, remaining=%d, device_id=%d\n", prev_entry->tid, prev_entry->current_time, prev_entry->remaining_time, prev_entry->device_id);
    } else {  // insert at head of queue
        printf("\t\t\t\tprev_entry : NULL\n");
        queue->first = current_entry;
    }
    if (current_entry != NULL) {
        current_entry->prev = prev_entry;
        printf("\t\t\t\tcurrent_entry : tid=%d, current=%f, remaining=%d, device_id=%d\n", current_entry->tid, current_entry->current_time, current_entry->remaining_time, current_entry->device_id);
    } else {  // insert at tail of queue
        printf("\t\t\t\tcurrent_entry : NULL\n");
        queue->last = prev_entry;
    }
    ret_entry->prev = NULL;
    ret_entry->next = NULL;
    printf("\t\tend dequeue()\n");
    return ret_entry;
}
