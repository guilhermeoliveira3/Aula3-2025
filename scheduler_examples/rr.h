//
// Created by cardo on 02/10/2025.
//

#ifndef RR_H
#define RR_H

#include "queue.h"
#include "msg.h"

void rr_scheduler(uint32_t current_time_ms, queue_t *ready_queue, pcb_t **CPU);

#endif //RR_H
