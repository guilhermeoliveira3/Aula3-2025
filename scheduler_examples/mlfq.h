#ifndef MLFQ_H
#define MLFQ_H

#include "queue.h"

#define NUM_QUEUES 4
#define QUANTUM_BASE_MS 5000
#define BOOST_INTERVAL_MS 1000
#define QUANTUM_MULTIPLIER 2

void mlfq_scheduler(uint32_t current_time_ms, queue_t *ready_queue, pcb_t **cpu);

#endif