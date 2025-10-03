//
// Created by cardo on 03/10/2025.
//
#include "mlfq.h"
#include "debug.h"

// Estrutura para armazenar informações específicas do MLFQ para cada PCB
typedef struct {
    uint8_t current_queue;      // Fila atual (0-2)
    uint32_t time_used;         // Tempo usado no current time slice
    uint32_t last_queue_time;   // Último tempo em que mudou de fila
} mlfq_pcb_data_t;

/**
 * @brief Inicializa a estrutura MLFQ
 */
void mlfq_init(mlfq_t *mlfq) {
    for (int i = 0; i < MLFQ_QUEUES; i++) {
        mlfq->queues[i].head = NULL;
        mlfq->queues[i].tail = NULL;
    }
    mlfq->last_boost_time = 0;
}

/**
 * @brief Versão simplificada para compatibilidade com interface existente
 */

void mlfq_scheduler_simple(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    static mlfq_t mlfq;
    static int initialized = 0;

    if (!initialized) {
        mlfq_init(&mlfq);
        initialized = 1;
        DBG("MLFQ: Initialized with %d queues\n", MLFQ_QUEUES);
    }

    // Move tarefas da ready queue para o MLFQ (fila 0 - mais alta prioridade)
    while (rq->head != NULL) {
        pcb_t *pcb = dequeue_pcb(rq);
        enqueue_pcb(&mlfq.queues[0], pcb);
        DBG("MLFQ: Added process %d to queue 0\n", pcb->pid);
    }

    // Chama o escalonador MLFQ
    mlfq_scheduler(current_time_ms, rq, cpu_task, &mlfq);
}

/**
 * @brief Adiciona uma tarefa ao MLFQ (sempre na fila de prioridade mais alta)
 */
void mlfq_enqueue(mlfq_t *mlfq, pcb_t *pcb) {
    // Tarefas novas sempre entram na fila de prioridade mais alta
    enqueue_pcb(&mlfq->queues[0], pcb);

    // Inicializa dados específicos do MLFQ
    mlfq_pcb_data_t *data = malloc(sizeof(mlfq_pcb_data_t));
    data->current_queue = 0;
    data->time_used = 0;
    data->last_queue_time = 0;

    // Armazena ponteiro para dados do MLFQ (usamos um campo genérico ou adicionamos ao PCB)
    // Por simplicidade, vamos usar um array global ou adicionar ao PCB
    // Nesta implementação, vamos gerenciar externamente
}

/**
 * @brief Encontra a próxima tarefa para executar
 */
pcb_t* mlfq_dequeue(mlfq_t *mlfq) {
    // Procura da fila de maior prioridade (0) para a mais baixa (2)
    for (int i = 0; i < MLFQ_QUEUES; i++) {
        if (mlfq->queues[i].head != NULL) {
            pcb_t *pcb = dequeue_pcb(&mlfq->queues[i]);
            DBG("MLFQ: Selected process %d from queue %d\n", pcb->pid, i);
            return pcb;
        }
    }
    return NULL; // Nenhuma tarefa encontrada
}

/**
 * @brief Realiza boost de prioridade
 */
void mlfq_priority_boost(mlfq_t *mlfq, uint32_t current_time_ms) {
    DBG("MLFQ: Performing priority boost at time %d ms\n", current_time_ms);

    // Move todas as tarefas das filas 1 e 2 para a fila 0
    for (int i = 1; i < MLFQ_QUEUES; i++) {
        while (mlfq->queues[i].head != NULL) {
            pcb_t *pcb = dequeue_pcb(&mlfq->queues[i]);
            enqueue_pcb(&mlfq->queues[0], pcb);
            DBG("MLFQ: Boosted process %d from queue %d to queue 0\n", pcb->pid, i);
        }
    }

    mlfq->last_boost_time = current_time_ms;
}

/**
 * @brief Obtém o time slice para uma fila específica
 */
uint32_t get_time_slice(uint8_t queue) {
    switch (queue) {
        case 0: return TIME_SLICE_0;
        case 1: return TIME_SLICE_1;
        case 2: return TIME_SLICE_2;
        default: return TIME_SLICE_2;
    }
}

/**
 * @brief Algoritmo de escalonamento MLFQ
 */
void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task, mlfq_t *mlfq) {
    static mlfq_pcb_data_t cpu_task_data = {0};

    // Verifica se é hora de fazer boost de prioridade
    if (current_time_ms - mlfq->last_boost_time >= BOOST_INTERVAL) {
        mlfq_priority_boost(mlfq, current_time_ms);
    }

    // Se há uma tarefa executando na CPU
    if (*cpu_task != NULL) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        cpu_task_data.time_used += TICKS_MS;

        uint32_t current_time_slice = get_time_slice(cpu_task_data.current_queue);

        // Verifica se a tarefa terminou
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Tarefa finalizada
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };

            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }

            DBG("MLFQ: Process %d finished execution at time %d ms\n",
                (*cpu_task)->pid, current_time_ms);

            free(*cpu_task);
            *cpu_task = NULL;
            cpu_task_data.time_used = 0;
        }
        // Verifica se a tarefa usou todo o time slice da fila atual
        else if (cpu_task_data.time_used >= current_time_slice) {
            DBG("MLFQ: Process %d used all time slice in queue %d\n",
                (*cpu_task)->pid, cpu_task_data.current_queue);

            // Rebaixa a tarefa para uma fila de prioridade mais baixa (se possível)
            if (cpu_task_data.current_queue < MLFQ_QUEUES - 1) {
                cpu_task_data.current_queue++;
                DBG("MLFQ: Process %d demoted to queue %d\n",
                    (*cpu_task)->pid, cpu_task_data.current_queue);
            }

            // Coloca a tarefa de volta na fila apropriada
            enqueue_pcb(&mlfq->queues[cpu_task_data.current_queue], *cpu_task);
            *cpu_task = NULL;
            cpu_task_data.time_used = 0;
        }
    }

    // Se a CPU está ociosa, busca próxima tarefa
    if (*cpu_task == NULL) {
        *cpu_task = mlfq_dequeue(mlfq);
        if (*cpu_task != NULL) {
            // Determina a fila baseada na prioridade (precisa ser mapeada)
            // Por simplicidade, assumimos que veio da fila 0 inicialmente
            // Em uma implementação real, precisaríamos rastrear a fila de cada tarefa
            cpu_task_data.current_queue = 0;
            cpu_task_data.time_used = 0;
            cpu_task_data.last_queue_time = current_time_ms;

            DBG("MLFQ: Started process %d in queue %d at time %d ms\n",
                (*cpu_task)->pid, cpu_task_data.current_queue, current_time_ms);
        }
    }
}

// Versão simplificada para compatibilidade com a interface existente
void mlfq_scheduler_simple(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    static mlfq_t mlfq;
    static int initialized = 0;

    if (!initialized) {
        mlfq_init(&mlfq);
        initialized = 1;
    }

    // Move tarefas da ready queue para o MLFQ
    while (rq->head != NULL) {
        pcb_t *pcb = dequeue_pcb(rq);
        mlfq_enqueue(&mlfq, pcb);
    }

    mlfq_scheduler(current_time_ms, rq, cpu_task, &mlfq);
}