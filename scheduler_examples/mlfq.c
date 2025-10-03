#include "mlfq.h"

#include <errno.h>
#include <stdio.h>

#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msg.h"


// Array estático para as filas de prioridade do MLFQ
static queue_t mlfq_queues[NUM_QUEUES];
static int initialized = 0;

void mlfq_scheduler(uint32_t current_time_ms, queue_t *ready_queue, pcb_t **cpu) {
    static uint32_t last_boost_time_ms = 0;

    // Inicializar filas internas do MLFQ na primeira chamada
    if (!initialized) {
        for (int i = 0; i < NUM_QUEUES; i++) {
            mlfq_queues[i].head = NULL;
            mlfq_queues[i].tail = NULL;
        }
        initialized = 1;
    }

    // Mover novos processos da ready_queue para a fila de maior prioridade
    while (ready_queue->head != NULL) {
        pcb_t *pcb = dequeue_pcb(ready_queue);
        if (pcb) {
            pcb->priority = 0; // Sempre iniciar na maior prioridade
            pcb->ellapsed_time_ms = 0; // Resetar tempo acumulado
            enqueue_pcb(&mlfq_queues[0], pcb);
            DBG("Process %d moved from ready_queue to MLFQ priority 0, time_ms=%u, status=%d\n", pcb->pid, pcb->time_ms, pcb->status);
        }
    }

    // Boost de prioridade periódico
    if (current_time_ms - last_boost_time_ms >= BOOST_INTERVAL_MS) {
        DBG("Performing priority boost at time %u ms\n", current_time_ms);
        for (int i = 1; i < NUM_QUEUES; i++) {
            while (mlfq_queues[i].head != NULL) {
                pcb_t *pcb = dequeue_pcb(&mlfq_queues[i]);
                if (pcb) {
                    pcb->priority = 0; // Mover para a fila de maior prioridade
                    pcb->ellapsed_time_ms = 0; // Resetar tempo acumulado no boost
                    enqueue_pcb(&mlfq_queues[0], pcb);
                    DBG("Process %d boosted to priority 0, time_ms=%u, status=%d\n", pcb->pid, pcb->time_ms, pcb->status);
                }
            }
        }
        last_boost_time_ms = current_time_ms;
    }

    // Se há um processo na CPU, atualizar seu tempo
    if (*cpu != NULL) {
        pcb_t *current_pcb = *cpu;
        uint32_t quantum = QUANTUM_BASE_MS * (1 << current_pcb->priority); // Quantum = QUANTUM_BASE_MS * 2^priority
        current_pcb->ellapsed_time_ms += TICKS_MS;
        current_pcb->time_ms = (current_pcb->time_ms > TICKS_MS) ? current_pcb->time_ms - TICKS_MS : 0;

        // Verificar se o processo terminou
        if (current_pcb->time_ms == 0 && current_pcb->status != TASK_BLOCKED) {
            msg_t msg = {
                .pid = current_pcb->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write(current_pcb->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                fprintf(stderr, "write failed for PID %d: %s\n", current_pcb->pid, strerror(errno));
            } else {
                DBG("Process %d finished execution, sent DONE at time %u ms, status=%d\n", current_pcb->pid, current_time_ms, current_pcb->status);
            }
            current_pcb->status = TASK_COMMAND;
            close(current_pcb->sockfd);
            free(current_pcb);
            *cpu = NULL;
        }
        // Verificar se o quantum foi excedido
        else if (current_pcb->ellapsed_time_ms >= quantum) {
            if (current_pcb->priority < NUM_QUEUES - 1) {
                current_pcb->priority++;
                DBG("Process %d demoted to priority %d, time_ms=%u, status=%d\n", current_pcb->pid, current_pcb->priority, current_pcb->time_ms, current_pcb->status);
            }
            current_pcb->ellapsed_time_ms = 0;
            current_pcb->slice_start_ms = current_time_ms;
            enqueue_pcb(&mlfq_queues[current_pcb->priority], current_pcb);
            *cpu = NULL;
        }
    }

    // Escolher o próximo processo da fila de maior prioridade
    if (*cpu == NULL) {
        for (int i = 0; i < NUM_QUEUES; i++) {
            if (mlfq_queues[i].head != NULL) {
                pcb_t *next_pcb = dequeue_pcb(&mlfq_queues[i]);
                if (next_pcb) {
                    next_pcb->slice_start_ms = current_time_ms;
                    next_pcb->ellapsed_time_ms = 0;
                    next_pcb->status = TASK_RUNNING;
                    *cpu = next_pcb;
                    DBG("Process %d scheduled from priority %d, time_ms=%u, status=%d\n", next_pcb->pid, i, next_pcb->time_ms, next_pcb->status);
                    break;
                }
            }
        }
    }
}