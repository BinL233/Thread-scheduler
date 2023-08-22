/*
 * This file represents the interface functions the students are supposed
 * to implement for this project.
 */
#ifndef INTERFACE_H
#define INTERFACE_H

#define IO_DEVICE_1_TICKS 2
#define IO_DEVICE_2_TICKS 5

/**
 * Initialize the Scheduler. This is invoked once in the beginning.
 * You can initialize anything you may want here.
 *
 * @param thread_count Total number of threads that will be run.
 */
void init_scheduler(int thread_count);

/**
 * A thread calls this function for CPU burst of 1 tick, with the remaining_time
 * in this burst. You must return from this function if the calling thread
 * should run on the CPU for at least the next tick. The thread will then
 * subsequently come back and call this function for the remaining time in
 * the burst (i.e. 1 tick less than the previous call).
 *
 * @param current_time  the time when the call is made by some thread (note, this can have float values)
 * @param tid Thread ID requesting the CPU burst of 1 tick.
 * @param remaining_time Remaining CPU ticks for this to complete. This will be non-zero if there is some
 * CPU burst remaining, otherwise it will be 0.
 *
 * @return the time when the thread has completed 1 tick of execution on the CPU.
 * Note that this may not necessarily be 1 more than the time that it was called with -
 * depends on whether some other thread will get to execute on the CPU in-between.
 *
 * For example, when only one thread0 arrived at time 1.1 with 'C3 E', the following calls to your
 * scheduler will be made:
 *
 * cpu_me(1.1, 0, 3); --> would be scheduled (by you) at time 2, return value should be 3 (since it used 1 CPU burst for time 2~3)
 * cpu_me(3, 0, 2); --> calls with current_time 3, return value should be 4
 * cpu_me(4, 0, 1); --> calls with current_time 4, return value should be 5
 * cpu_me(5, 0, 0); --> calls with current_time 5, return value ignored
 */
int cpu_me(float current_time, int tid, int remaining_time);

/**
 * A thread calls this function at the start of the IO operation, with the device_id
 * specified in the input file. This function must return only when the whole IO Device
 * is finished, and will be blocked until such a time.
 *
 * Implementation Notes:
 *  - It is possible that the IO device is busy when this call is made, in which case
 *  the request should get serviced only after all prior IO requests are serviced.
 *  - This means that across all threads the IO requests will be handled in a first come
 *  first serve basis.
 *  - For Device 1, `IO_DEVICE_1_TICKS` is the duration used, while for device 2 `IO_DEVICE_2_TICKS`
 *  will be used.
 *  - Each IO Device can service a request in parallel to other IO Devices/CPU.
 *
 * @param current_time the time when the call is made by some thread (note, this can have float values)
 * @param tid Thread ID requesting the IO.
 * @param device_id Device ID for this request.
 *
 * @return the time the IO Device completely finishes and returns
 *
 * For example, when only one thread0 arrived at time 0.3 with 'I2 C2 I1 E', the following calls to your scheduler
 * will be made:
 *
 * io_me(0.3, 0, 2); --> IO Device 2 would be scheduled (by you) at time 1, return value should be 6 (since IO Device was busy
 *   for 5 ticks from 1~6)
 * cpu_me(6, 0, 2); --> at time 6, the CPU burst will start (called)
 * cpu_me(7, 0, 1); --> calls with current_time 7, return value should be 8
 * cpu_me(8, 0, 0); --> calls with current_time 8, return value ignored
 * io_me(8, 0, 2);
 */
int io_me(float current_time, int tid, int device_id);

/**
 * Notify your scheduler that the calling thread tid is terminating.
 * @param tid Thread ID which has completed.
 */
void end_me(int tid);

#endif
