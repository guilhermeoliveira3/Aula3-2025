//
// Created by cardo on 03/10/2025.
//
#ifndef MLFQ_H
#define MLFQ_H

#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include <unistd.h>
#include "queue.h"

#define MLFQ_QUEUES 3           // Número de filas (0 = mais alta prioridade, 2 = mais baixa)
#define TIME_SLICE_0 100        // Time slice para fila 0 (100ms)
#define TIME_SLICE_1 200        // Time slice para fila 1 (200ms)
#define TIME_SLICE_2 400        // Time slice para fila 2 (400ms)
#define BOOST_INTERVAL 5000     // Boost de prioridade a cada 5 segundos

/**
 * @brief Estrutura para as múltiplas filas do MLFQ
 */
typedef struct {
    queue_t queues[MLFQ_QUEUES]; // Array de filas (0 = mais alta prioridade)
    uint32_t last_boost_time;    // Último tempo em que foi feito boost de prioridade
} mlfq_t;

/**
 * @brief Inicializa a estrutura MLFQ
 *
 * @param mlfq Ponteiro para a estrutura MLFQ
 */
void mlfq_init(mlfq_t *mlfq);

/**
 * @brief Algoritmo de escalonamento Multi-Level Feedback Queue (MLFQ)
 *
 * Este algoritmo implementa múltiplas filas com diferentes prioridades e time slices.
 * - Fila 0: Prioridade mais alta, time slice menor
 * - Fila 1: Prioridade média, time slice médio
 * - Fila 2: Prioridade mais baixa, time slice maior
 *
 * Tarefas novas entram na fila de prioridade mais alta.
 * Tarefas que usam todo seu time slice são rebaixadas.
 * Boost de prioridade periódico para evitar starvation.
 *
 * @param current_time_ms Tempo atual em milissegundos
 * @param rq Fila de prontos (não usada diretamente - usamos as filas internas do MLFQ)
 * @param cpu_task Tarefa atualmente em execução na CPU
 * @param mlfq Estrutura MLFQ com as múltiplas filas
 */
void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task, mlfq_t *mlfq);

/**
 * @brief Adiciona uma tarefa ao MLFQ (sempre na fila de prioridade mais alta)
 *
 * @param mlfq Estrutura MLFQ
 * @param pcb Tarefa a ser adicionada
 */
void mlfq_enqueue(mlfq_t *mlfq, pcb_t *pcb);

/**
 * @brief Encontra a próxima tarefa para executar (da fila de maior prioridade não vazia)
 *
 * @param mlfq Estrutura MLFQ
 * @return pcb_t* Próxima tarefa a executar, ou NULL se não houver tarefas
 */
pcb_t* mlfq_dequeue(mlfq_t *mlfq);

/**
 * @brief Realiza boost de prioridade - move todas as tarefas para a fila de prioridade mais alta
 *
 * @param mlfq Estrutura MLFQ
 * @param current_time_ms Tempo atual em milissegundos
 */
void mlfq_priority_boost(mlfq_t *mlfq, uint32_t current_time_ms);

/**
 * @brief Versão simplificada do MLFQ para compatibilidade com interface existente
 *
 * Esta função fornece uma interface compatível com os outros escalonadores
 * enquanto usa a implementação completa do MLFQ internamente.
 */
void mlfq_scheduler_simple(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);

#endif // MLFQ_H
