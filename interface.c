#include "interface.h"
#include "scheduler.h"
#include "stdio.h"
#include "pthread.h"
#include "string.h"

// Interface implementation

void init_scheduler(int thread_count) {

    // initialize fields
    printf("init_scheduler()\n");
    global_time = 0;
    global_IO_1_time = 0;
    global_IO_2_time = 0;
    queued_thread_count = 0;
    queued_IO_1_count = 0;
    queued_IO_2_count = 0;
    total_thread_count = thread_count;
    CPU_tid = -1;
    IO_1_tid = -1;
    IO_2_tid = -1;
    CPU_policy = SRJF;
    IO_policy = FCFS;
    // is_cpu_me_updated = true;

    // initialize CPU and IO queues
    CPU_queue = (queue_t *) calloc(1, sizeof(queue_t));
    CPU_queue->first = NULL;
    CPU_queue->last = NULL;
    IO_1_queue = (queue_t *) calloc(1, sizeof(queue_t));
    IO_1_queue->first = NULL;
    IO_1_queue->last = NULL;
    IO_2_queue = (queue_t *) calloc(1, sizeof(queue_t));
    IO_2_queue->first = NULL;
    IO_2_queue->last = NULL;

}


int cpu_me(float current_time, int tid, int remaining_time) {

    printf("cpu_me(%d)\n", tid);
    // do nothing if no actual burst
    if (remaining_time == 0) {
        printf("return cpu_me(%d) , b/c remaining_time = 0\n", tid);
        return current_time;
    }

    // add task to scheduler
    pthread_mutex_lock(&mutex);
    printf("\tT%d locked : CPU_mutex\n", tid);
    enqueue(CPU_policy, CPU_queue, current_time, tid, remaining_time, -1);
    queued_thread_count += 1;
    // if some threads not arrived yet, wait until all threads arrived
    if (queued_thread_count < total_thread_count) {
        printf("\tT%d unlocked : CPU_mutex\n", tid);
        printf("\twait for all threads to arrive : %d of %d\n", queued_thread_count, total_thread_count);
        pthread_cond_wait(&all_threads_arrived, &mutex);
        printf("\tT%d finish waiting : all_threads_arrived + CPU_mutex\n", tid);
    } else {
    // all threads arrived, trigger scheduler
        printf("\tall threads arrived : %d of %d\n", queued_thread_count, total_thread_count);
        pthread_cond_broadcast(&all_threads_arrived);
        printf("\tcall schedule_next()\n");
        // schedule_CPU();
        schedule_next();
    }
    
    // wait until scheduled
    while (CPU_tid != tid)  {
        printf("\tT%d unlocked : CPU_mutex\n", tid);
        printf("\tT%d wait for being scheduled : current CPU_tid = %d\n", tid, CPU_tid);
        if (CPU_tid == tid) break;
        pthread_cond_wait(&CPU_cond, &mutex);
    };

    // scheduled
    int ret_time = (int)global_time;
    queued_thread_count -= 1;
    CPU_tid = -1;
    printf("\tT%d finishes CPU at %d\n", tid, ret_time);
    printf("\tT%d unlocked : CPU_mutex\n", tid);
    pthread_mutex_unlock(&mutex);
    printf("return cpu_me(%d)\n", tid);
    
    return ret_time;

}


int io_me(float current_time, int tid, int device_id) {

    printf("io_me(tid=%d, device_id=%d)\n", tid, device_id);
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
        printf("\tInvalid Device_ID : %d\n", device_id);
        return -1;
    }

    // add task to scheduler
    pthread_mutex_lock(&mutex);
    printf("\tT%d locked : IO_%d_mutex\n", tid, device_id);

    enqueue(IO_policy, IO_queue, current_time, tid, -1, device_id);
    queued_thread_count += 1;
    *queued_IO_count += 1;
    // if some threads not arrived yet, wait until all threads arrived
    if (queued_thread_count < total_thread_count) {
        printf("\tT%d unlocked : IO_%d_mutex\n", tid, device_id);
        printf("\twait for all threads to arrive : %d of %d\n", queued_thread_count, total_thread_count);
        pthread_cond_wait(&all_threads_arrived, &mutex);
    } else {
    // all threads arrived, trigger scheduler
        printf("\tall threads arrived : %d of %d\n", queued_thread_count, total_thread_count);
        pthread_cond_broadcast(&all_threads_arrived);
        printf("\tcall schedule_next()\n");
        // schedule_IO(device_id);
        schedule_next();
    }
    // wait until scheduled
    while (*IO_tid != tid)  {
        
        printf("\tT%d locked : IO_%d_mutex\n", tid, device_id);
        printf("\tT%d wait for being scheduled : current IO_%d_tid = %d\n", tid, device_id, *IO_tid);
        if (*IO_tid == tid) break;
        pthread_cond_wait(IO_cond, &mutex);
        
    };
    // scheduled
    int ret_time = (int)*global_IO_time;
    queued_thread_count -= 1;
    queued_IO_count -= 1;
    *IO_tid = -1;

    printf("\tT%d finishes IO at %d\n", tid, ret_time);
    printf("\tT%d unlocked : IO_%d_mutex\n", tid, device_id);
    pthread_mutex_unlock(&mutex);
    printf("return io_me(tid=%d, device_id=%d)\n", tid, device_id);
    return ret_time;

}


void end_me(int tid) {
    
    printf("end_me()\n");
    pthread_mutex_lock(&end_me_mutex);
    pthread_mutex_lock(&mutex);
    printf("\tT%d locked : end_me_mutex\n", tid);
    total_thread_count -= 1;
    if (total_thread_count != 0 && queued_thread_count == total_thread_count) {
        pthread_cond_broadcast(&all_threads_arrived);
        // schedule_IO(1);
        // schedule_IO(2);
        // schedule_CPU();
        printf("\tcall schedule_next()\n");
        schedule_next();
    }
    pthread_mutex_unlock(&mutex);
    pthread_mutex_unlock(&end_me_mutex);
    printf("\tT%d unlocked : end_me_mutex\n", tid);
    printf("return end_me(%d)\n", tid);
    return;
    
}