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

#include <schedule.h>
#include <cm4.h>
#include <kstdio.h>
#include <errno.h>

/* CMSIS Core includes for register access */
#include "stm32f446xx.h"

/* Inline helpers for PSP and CONTROL register access */
static inline void __set_PSP(uint32_t topOfProcStack)
{
    __asm volatile ("MSR PSP, %0" : : "r" (topOfProcStack) : );
}

static inline void __set_CONTROL(uint32_t control)
{
    __asm volatile ("MSR CONTROL, %0" : : "r" (control) : );
}

/* Global scheduler state */
static Scheduler_TypeDef g_scheduler;

/* Task storage - TCBs and stacks */
static TCB_TypeDef g_tcb_pool[MAX_TASKS];
static uint8_t g_task_stacks[MAX_TASKS][TASK_STACK_SIZE] __attribute__((aligned(8)));

/* Tick counter for scheduling */
static volatile uint32_t g_scheduler_ticks = 0;
static volatile uint8_t g_schedule_pending = 0;

/*
 * Initialize the task scheduler
 */
void scheduler_init(void)
{
    kprintf("[SCHEDULER] Initializing scheduler...\n");
    
    /* Clear scheduler state */
    g_scheduler.current_task = 0;
    g_scheduler.ready_queue = 0;
    g_scheduler.task_count = 0;
    g_scheduler.next_task_id = 1000;
    g_scheduler.context_switches = 0;
    g_scheduler.scheduler_started = 0;
    
    /* Clear TCB pool */
    for (int i = 0; i < MAX_TASKS; i++) {
        g_tcb_pool[i].status = TASK_STATUS_KILLED;
        g_tcb_pool[i].task_id = 0;
        g_tcb_pool[i].psp = 0;
        g_tcb_pool[i].next = 0;
    }
    
    g_scheduler_ticks = 0;
    g_schedule_pending = 0;
    
    kprintf("[SCHEDULER] Scheduler initialized successfully\n");
}

/*
 * Initialize task stack with a proper exception frame
 * Stack grows downward, so we start from the top
 */
void initialize_task_stack(TCB_TypeDef *task, void (*task_handler)(void))
{
    uint32_t *stack_ptr;
    
    /* Get pointer to top of stack (highest address) */
    uint32_t stack_index = (task - g_tcb_pool);
    stack_ptr = (uint32_t*)(&g_task_stacks[stack_index][TASK_STACK_SIZE]);
    
    /* Create initial exception stack frame (hardware auto-saves these on exception entry) */
    *(--stack_ptr) = 0x01000000;        /* xPSR - Thumb bit set */
    *(--stack_ptr) = (uint32_t)task_handler;  /* PC - task entry point */
    *(--stack_ptr) = 0xFFFFFFFD;        /* LR - EXC_RETURN (return to thread mode, use PSP) */
    *(--stack_ptr) = 0x12121212;        /* R12 */
    *(--stack_ptr) = 0x03030303;        /* R3 */
    *(--stack_ptr) = 0x02020202;        /* R2 */
    *(--stack_ptr) = 0x01010101;        /* R1 */
    *(--stack_ptr) = 0x00000000;        /* R0 */
    
    /* Save software context (R4-R11) - these must be manually saved/restored */
    *(--stack_ptr) = 0x11111111;        /* R11 */
    *(--stack_ptr) = 0x10101010;        /* R10 */
    *(--stack_ptr) = 0x09090909;        /* R9 */
    *(--stack_ptr) = 0x08080808;        /* R8 */
    *(--stack_ptr) = 0x07070707;        /* R7 */
    *(--stack_ptr) = 0x06060606;        /* R6 */
    *(--stack_ptr) = 0x05050505;        /* R5 */
    *(--stack_ptr) = 0x04040404;        /* R4 */
    
    /* Store final PSP value in TCB */
    task->psp = (void*)stack_ptr;
}

/*
 * Add task to ready queue (circular linked list)
 */
void add_task_to_ready_queue(TCB_TypeDef *task)
{
    task->status = TASK_STATUS_READY;
    
    if (g_scheduler.ready_queue == 0) {
        /* First task in queue */
        g_scheduler.ready_queue = task;
        task->next = task;  /* Point to itself (circular) */
    } else {
        /* Insert at end of circular queue */
        TCB_TypeDef *last = g_scheduler.ready_queue;
        while (last->next != g_scheduler.ready_queue) {
            last = last->next;
        }
        last->next = task;
        task->next = g_scheduler.ready_queue;  /* Complete the circle */
    }
}

/*
 * Get next ready task from queue (round-robin)
 */
TCB_TypeDef* get_next_ready_task(void)
{
    if (g_scheduler.ready_queue == 0) {
        return 0;
    }
    
    /* Move to next task in circular queue */
    TCB_TypeDef *next_task = g_scheduler.ready_queue;
    g_scheduler.ready_queue = g_scheduler.ready_queue->next;
    
    return next_task;
}

/*
 * Create a new task
 */
int32_t task_create(void (*task_handler)(void), uint8_t priority)
{
    if (g_scheduler.task_count >= MAX_TASKS) {
        kprintf("[SCHEDULER] ERROR: Maximum tasks reached\n");
        return -ENOMEM;
    }
    
    /* Find free TCB slot */
    TCB_TypeDef *task = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tcb_pool[i].status == TASK_STATUS_KILLED) {
            task = &g_tcb_pool[i];
            break;
        }
    }
    
    if (task == 0) {
        return -ENOMEM;
    }
    
    /* Initialize TCB */
    task->magic_number = STACK_MAGIC_NUMBER;
    task->task_id = g_scheduler.next_task_id++;
    task->status = TASK_STATUS_NEW;
    task->priority = priority;
    task->parent_id = 0;
    task->execution_time = 0;
    task->waiting_time = 0;
    task->start_time = __getTime();
    task->last_run_time = 0;
    task->task_handler = task_handler;
    task->next = 0;
    
    /* Initialize task stack */
    initialize_task_stack(task, task_handler);
    
    /* Add to ready queue */
    add_task_to_ready_queue(task);
    
    g_scheduler.task_count++;
    
    kprintf("[SCHEDULER] Task created: ID=%d, Priority=%d\n", task->task_id, priority);
    
    return task->task_id;
}

/*
 * Get current running task
 */
TCB_TypeDef* get_current_task(void)
{
    return g_scheduler.current_task;
}

/*
 * Schedule next task - called from PendSV handler
 * This function is called in PendSV context
 */
void schedule_next_task(void)
{
    if (g_scheduler.ready_queue == 0) {
        return;  /* No tasks to schedule */
    }
    
    /* Save timing info for current task */
    if (g_scheduler.current_task != 0) {
        uint32_t current_time = __getTime();
        g_scheduler.current_task->execution_time += (current_time - g_scheduler.current_task->last_run_time);
    }
    
    /* Get next task */
    TCB_TypeDef *next_task = get_next_ready_task();
    if (next_task == 0) {
        return;
    }
    
    /* Update scheduler state */
    g_scheduler.current_task = next_task;
    next_task->status = TASK_STATUS_RUNNING;
    next_task->last_run_time = __getTime();
    
    g_scheduler.context_switches++;
    g_schedule_pending = 0;
}

/*
 * Start the scheduler
 * This function switches to the first task and never returns
 */
void scheduler_start(void)
{
    if (g_scheduler.task_count == 0) {
        kprintf("[SCHEDULER] ERROR: No tasks to run\n");
        return;
    }
    
    kprintf("[SCHEDULER] Starting scheduler with %d tasks\n", g_scheduler.task_count);
    kprintf("[SCHEDULER] Time quantum: 10ms\n");
    kprintf("[SCHEDULER] Total context switches: %d\n", g_scheduler.context_switches);
    
    g_scheduler.scheduler_started = 1;
    
    /* Set PendSV to lowest priority */
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    
    /* Set SysTick to low priority (but higher than PendSV) */
    NVIC_SetPriority(SysTick_IRQn, 0xFE);
    
    /* Get first task */
    TCB_TypeDef *first_task = get_next_ready_task();
    if (first_task == 0) {
        kprintf("[SCHEDULER] ERROR: Failed to get first task\n");
        return;
    }
    
    g_scheduler.current_task = first_task;
    first_task->status = TASK_STATUS_RUNNING;
    first_task->last_run_time = __getTime();
    
    /* Set PSP to first task's stack */
    __set_PSP((uint32_t)first_task->psp);
    
    /* Switch to use PSP instead of MSP */
    __set_CONTROL(0x02);  /* Use PSP for thread mode */
    __ISB();  /* Instruction Synchronization Barrier */
    
    kprintf("[SCHEDULER] Starting first task (ID=%d)...\n", first_task->task_id);
    
    /* Extract the task entry point from the initialized stack */
    /* Stack layout: [R4-R11] [R0,R1,R2,R3,R12,LR,PC,xPSR] */
    /* PSP points to R4, PC is at offset 14 words (56 bytes) */
    uint32_t *stack = (uint32_t*)first_task->psp;
    void (*task_func)(void) = (void(*)(void))stack[14];  /* PC is at index 14 */
    
    kprintf("[SCHEDULER] Task entry: 0x%08lX\n", (unsigned long)task_func);
    kprintf("[SCHEDULER] Calling task directly...\n\n");
    
    /* For now, just call the task function directly as a test */
    /* This won't give us proper multitasking yet, but will verify tasks work */
    task_func();
    
    /* Should never reach here */
    while(1) {
        kprintf("[ERROR] Task returned to scheduler!\n");
        ms_delay(1000);
    }
}

/*
 * Task yield - voluntarily give up CPU
 */
void task_yield(void)
{
    /* Trigger PendSV to perform context switch */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*
 * Called from SysTick handler every 10ms
 */
void scheduler_tick(void)
{
    if (!g_scheduler.scheduler_started) {
        return;
    }
    
    g_scheduler_ticks++;
    
    /* Trigger context switch via PendSV */
    g_schedule_pending = 1;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
