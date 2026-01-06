/*
 * Copyright (c) 2022 
 * Computer Science and Engineering, University of Dhaka
 * Credit: CSE Batch 25 (starter) and Prof. Mosaddek Tushar
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 
#ifndef __SCHEDULE_H
#define __SCHEDULE_H

#include <stdint.h>

/* Task status definitions */
#define TASK_STATUS_NEW         0
#define TASK_STATUS_READY       1
#define TASK_STATUS_RUNNING     2
#define TASK_STATUS_WAITING     3
#define TASK_STATUS_TERMINATED  4
#define TASK_STATUS_KILLED      5

/* Stack magic number for integrity checking */
#define STACK_MAGIC_NUMBER      0xDEADBEEF

/* Maximum number of tasks */
#define MAX_TASKS               8

/* Task stack size (in bytes) */
#define TASK_STACK_SIZE         1024

/* Task Control Block (TCB) */
typedef struct t_task_tcb {
    uint32_t magic_number;      /* Stack integrity check */
    uint16_t task_id;           /* Unique task ID (starting from 1000) */
    void *psp;                  /* Process Stack Pointer - CRITICAL for scheduling */
    uint16_t status;            /* Task status (new/ready/running/waiting/terminated/killed) */
    uint8_t priority;           /* Priority level (0 = highest) */
    uint16_t parent_id;         /* Parent task ID */
    uint32_t execution_time;    /* Total execution time in ms */
    uint32_t waiting_time;      /* Total waiting time in ms */
    uint32_t start_time;        /* Time when task was created */
    uint32_t last_run_time;     /* Last time task got CPU */
    void (*task_handler)(void); /* Task entry point function */
    struct t_task_tcb *next;    /* Next task in ready queue */
} TCB_TypeDef;

/* Scheduler state */
typedef struct {
    TCB_TypeDef *current_task;  /* Currently running task */
    TCB_TypeDef *ready_queue;   /* Head of ready queue (circular) */
    uint16_t task_count;        /* Number of active tasks */
    uint16_t next_task_id;      /* Next available task ID */
    uint32_t context_switches;  /* Total context switches performed */
    uint8_t scheduler_started;  /* Flag: scheduler running */
} Scheduler_TypeDef;

/* Scheduler API functions */
void scheduler_init(void);
int32_t task_create(void (*task_handler)(void), uint8_t priority);
void scheduler_start(void);
void schedule_next_task(void);
TCB_TypeDef* get_current_task(void);
void task_yield(void);

/* Internal helper functions */
void add_task_to_ready_queue(TCB_TypeDef *task);
TCB_TypeDef* get_next_ready_task(void);
void initialize_task_stack(TCB_TypeDef *task, void (*task_handler)(void));

#endif /* __SCHEDULE_H */



