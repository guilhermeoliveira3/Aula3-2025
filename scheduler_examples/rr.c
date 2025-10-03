#include "queue.h"
#include "msg.h"
#include "debug.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define QUANTUM_MS 500   // Tamanho do quantum em ms

void rr_scheduler(uint32_t current_time_ms, queue_t *ready_queue, pcb_t **CPU) {
    static uint32_t slice_start_time = 0; // Quando começou a rodar
    static pcb_t *current_pcb = NULL;

    // Se não tem processo rodando, pega da fila
    if (*CPU == NULL && ready_queue->head != NULL) {
        queue_elem_t *elem = ready_queue->head;
        remove_queue_elem(ready_queue, elem);

        current_pcb = elem->pcb;
        free(elem);

        current_pcb->status = TASK_RUNNING;
        *CPU = current_pcb;
        slice_start_time = current_time_ms;

        DBG("[RR] Processo %d entrou na CPU (tempo %d ms restante)\n",
            current_pcb->pid, current_pcb->time_ms);
        return;
    }

    // Se tem processo rodando, verificar se terminou ou se expirou quantum
    if (*CPU != NULL) {
        pcb_t *pcb = *CPU;
        (*CPU)->ellapsed_time_ms = current_time_ms - slice_start_time;

        // Determina quanto tempo rodar neste tick
        uint32_t run_time = (pcb->time_ms < QUANTUM_MS) ? pcb->time_ms : QUANTUM_MS;

        // Se já passou o quantum ou o processo termina antes do quantum
        if ((*CPU)->ellapsed_time_ms >= run_time) {
            // Decrementa corretamente
            if (pcb->time_ms > run_time)
                pcb->time_ms -= run_time;
            else
                pcb->time_ms = 0;

            DBG("[RR] Quantum expirou ou processo terminou para %d (resta %d ms)\n",
                pcb->pid, pcb->time_ms);

            // Remove da CPU
            *CPU = NULL;
            current_pcb = NULL;

            // Se ainda tem tempo, volta para READY
            if (pcb->time_ms > 0) {
                enqueue_pcb(ready_queue, pcb);
                DBG("[RR] Processo %d voltou para a READY queue\n", pcb->pid);
            } else {
                DBG("[RR] Processo %d terminou sua execução\n", pcb->pid);
                // envia PROCESS_REQUEST_DONE para o app
            }
        }
    }
}
