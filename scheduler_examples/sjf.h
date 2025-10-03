//
// Created by cardo on 03/10/2025.
//
#ifndef SJF_H
#define SJF_H

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>
#include "queue.h"

/**
 * @brief Shortest Job First (SJF) scheduling algorithm.
 *
 * This function implements the non-preemptive SJF scheduling algorithm.
 * If the CPU is not idle, it checks if the current task has finished.
 * If the CPU is idle, it selects the task with the shortest burst time
 * from the ready queue.
 *
 * @param current_time_ms The current time in milliseconds.
 * @param rq Pointer to the ready queue containing tasks that are ready to run.
 * @param cpu_task Double pointer to the currently running task. This will be updated
 *                 to point to the next task to run.
 */
void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);

/**
 * @brief Finds the PCB with the shortest job time in the ready queue.
 *
 * This function searches through the ready queue to find the PCB with
 * the smallest remaining time (time_ms). It returns the queue element
 * containing that PCB.
 *
 * @param rq Pointer to the ready queue.
 * @return queue_elem_t* Pointer to the queue element with the shortest job time,
 *                       or NULL if the queue is empty.
 */
queue_elem_t* find_shortest_job(queue_t *rq);

#endif // SJF_H
