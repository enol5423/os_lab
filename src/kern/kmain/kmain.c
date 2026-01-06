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

#include <sys_init.h>
#include <cm4.h>
#include <kmain.h>
#include <stdint.h>
#include <sys_usart.h>
#include <kstdio.h>
#include <sys_rtc.h>
#include <kstring.h>
#include <unistd.h>
#include <UsartRingBuffer.h>  /* For Uart_read, Uart_peek */
// #include <schedule.h>  // Disabled for now

/* Userland interactive syscall menu (implemented in userland/utils/interactive_syscall_menu.c) */
extern void interactive_syscall_menu(void);

#ifndef DEBUG
#define DEBUG 1
#endif

/* Local wrappers for CONTROL/PSP writes (since CMSIS helpers may not be exposed here) */
static inline void k_set_PSP(uint32_t topOfProcStack)
{
    __asm volatile ("MSR PSP, %0" : : "r" (topOfProcStack) : );
}

static inline void k_set_CONTROL(uint32_t control)
{
    __asm volatile ("MSR CONTROL, %0" : : "r" (control) : );
}

/* No comprehensive auto test; interactive menu only */

void kmain(void)
{
    __sys_init();
    
    // Enable SysTick interrupts for time tracking
    SysTickIntEnable();
    __SysTick_enable();
    
    // Small delay for serial monitor connection
    ms_delay(1000);
    
    /* UART handle not needed here anymore; input handled entirely in userland via syscalls */
    
    kprintf("\n");
    kprintf("****************************************************\n");
    kprintf("   DUOS SYSCALL TESTING SYSTEM\n");
    kprintf("****************************************************\n");
    kprintf("\n");
    kprintf("Testing system calls through proper syscall interface\n");
    kprintf("All tests use syscall() wrapper functions (SVC)\n");
    kprintf("\n");
    kprintf("Starting INTERACTIVE mode\n");
    /*
     * Optionally run the interactive menu as an unprivileged user thread.
     * This demonstrates syscalls being invoked from user mode (PSP + unprivileged).
     * Define RUN_USER_UNPRIV to enable.
     */
#ifdef RUN_USER_UNPRIV
    /* allocate a small stack for the user thread */
    static uint32_t user_stack[256]; /* 1KB stack */
    uint32_t user_sp = (uint32_t)&user_stack[256]; /* top-of-stack (full descending) */

    kprintf("Switching to user (unprivileged) Thread mode using PSP...\n");
    /* Set PSP to user stack and switch Thread mode to use PSP & unprivileged */
    k_set_PSP(user_sp);
    /* CONTROL: bit0=SPSEL(1=PSP), bit1=nPRIV(1=unprivileged) -> value = 0b11 = 3 */
    k_set_CONTROL( (1<<0) | (1<<1) );
    __ISB(); /* ensure the change takes effect immediately */

    /* Run interactive menu unprivileged */
    interactive_syscall_menu();
#else
    /* Run interactive menu privileged (development mode) */
    interactive_syscall_menu();
#endif
    
    kprintf("\n");
    kprintf("================================================\n");
    kprintf("   EXITED INTERACTIVE MODE\n");
    kprintf("================================================\n");
    kprintf("\nSystem time: %d ms\n", (int)__getTime());
    
    /* Idle loop */
    while (1) {
        __WFI();  // Wait for interrupt - low power mode
    }
}
