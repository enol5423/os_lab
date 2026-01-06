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

/* Syscall dispatcher implementation */
#include <syscall.h>
#include <syscall_def.h>
#include <errno.h>
#include <errmsg.h>
#include <stdint.h>
#include <kstdio.h>
#include <kunistd.h>
#include <cm4.h>
#include <schedule.h>
#include <UsartRingBuffer.h>
#include <sys_usart.h>
#include <serial_lin.h> /* for noIntWrite() polling UART TX used inside sys_write_impl */

/*
 Contract (register-based):
  - r0: syscall id (uint16_t used)
  - r1: arg1, r2: arg2, r3: arg3
 Return value placed in r0 by SVC handler after calling this.
*/

static int32_t sys_write_impl(int fd, const uint8_t *buf, uint32_t len)
{
	if (buf == 0U) return -EINVAL;
	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		/*
		 * IMPORTANT: This runs in SVC handler context. The usual Uart_write()
		 * uses an interrupt-driven TX ring buffer and will block if the TXE
		 * interrupt cannot run (which is true while we're in an exception with
		 * lower-priority interrupts masked). To avoid deadlock, use the
		 * polling-based noIntWrite() that directly writes to DR when TXE is set.
		 */
		extern UART_HandleTypeDef huart2; /* console UART */
		for (uint32_t i = 0; i < len; i++) {
			noIntWrite(&huart2, (char)buf[i]);
		}
		return (int32_t)len;
	}
	return -ENOSYS;
}

static int32_t sys_read_impl(int fd, uint8_t *buf, uint32_t len)
{
	if (buf == 0U) return -EINVAL;
	
	/* Only support reading from stdin (fd=0) via UART */
	if (fd == STDIN_FILENO) {
		extern UART_HandleTypeDef huart2;  /* Console UART */
		uint32_t count = 0;
		
		if (len == 0) return 0;
		
		/* Non-blocking: attempt to fetch up to len bytes; return 0 if none available. */
		/* First byte */
		int c = Uart_read(&huart2);
		if (c == -1) {
			/* Fallback direct hardware poll */
			if (huart2.Instance->SR & USART_SR_RXNE) {
				c = (int)(huart2.Instance->DR & 0xFF);
			} else {
				return 0; /* no data */
			}
		}
		buf[count++] = (uint8_t)c;
		/* Additional immediately available bytes */
		while (count < len) {
			int d = Uart_read(&huart2);
			if (d == -1) {
				if (huart2.Instance->SR & USART_SR_RXNE) {
					d = (int)(huart2.Instance->DR & 0xFF);
				} else break;
			}
			buf[count++] = (uint8_t)d;
		}
		return (int32_t)count;
	}
	
	return -ENOSYS;
}

static int32_t sys_time_impl(void)
{
	/*
 * __getTime()
 *
 * What is this?
 * - A standard function provided by ARM for microcontrollers (Cortex-M cores).
 *
 * What does it do?
 * - It returns the current time, typically in milliseconds or microseconds.
 * - The exact type and range depend on the implementation.
 *
 * How is it used?
 * - The return value is cast to (int32_t) to fit into a 32-bit signed integer.
 * - This is useful for timekeeping and timing operations.
 *
 * Example:
 * - __getTime() returns 123456789 (milliseconds).
 * - Cast to (int32_t) gives 123456789.
 */
	return (int32_t)__getTime();
}

static int32_t sys_reboot_impl(void)
{
	/*
 * NVIC_SystemReset()
 *
 * What is this?
 * - A standard function provided by ARM for microcontrollers (Cortex-M cores).
 *
 * What does it do?
 * - It forces the entire microcontroller to perform a **Software Reset** (a reboot).
 * - Everything returns to its starting, power-on state (CPU, peripherals, etc.).
 * - This is useful for error recovery or after updating the device firmware.
 *
* HOW NVIC_SystemReset() WORKS (Detailed Mechanism)
 *
 * 1. TARGET REGISTER: The function targets the **Application Interrupt and Reset
 * Control Register (AIRCR)** located in the **System Control Block (SCB)**.
 *
 * 2. WRITE OPERATION: It writes a 32-bit value to the AIRCR, which contains two
 * crucial parts:
 * a) **VECTKEY (0x5FA):** A required "magic key" written to the upper 16 bits
 * to authorize the change (a safety mechanism).
 * b) **SYSRESETREQ Bit:** The bit in the lower section that commands the CPU
 * to perform the reset.
 *
 * 3. SYNCHRONIZATION: A **Data Synchronization Barrier (DSB)** is executed to
 * ensure the reset command is fully processed before the core attempts to
 * execute any more code.
 *
 * 4. EXECUTION: The system resets almost immediately, forcing a reboot and a
 * jump back to the startup code (Vector Table), which means the function
 * call never returns.
 */
	NVIC_SystemReset();
	return 0; /* not reached */
}

static int32_t sys_yield_impl(void)
{
	/* Call scheduler's task_yield function to trigger PendSV */
	task_yield();
	return 0;
}

static int32_t sys_getpid_impl(void)
{
	/* Return current task's ID from scheduler */
	extern TCB_TypeDef* get_current_task(void);
	TCB_TypeDef *current = get_current_task();
	if (current != 0) {
		return (int32_t)current->task_id;
	}
	/* Fallback if no task running (shouldn't happen) */
	return 1;
}

static int32_t sys_exit_impl(int code)
{
	(void)code;
	/*
	 * Per assignment: (1) change process state to 'terminated'
	 * and (2) call yield() to activate PendSV for next task
	 */
	extern TCB_TypeDef* get_current_task(void);
	TCB_TypeDef *current = get_current_task();
	if (current != 0) {
		current->status = TASK_STATUS_KILLED;
	}
	/* Yield CPU to scheduler to run next task */
	task_yield();
	/* Should not return, but if no other tasks, halt */
	while (1) {
		__WFI();
	}
	return 0;
}

/* Debug instrumentation to verify dispatcher is reached */
volatile uint32_t g_syscall_dispatch_count = 0;
volatile uint32_t g_last_syscall_id = 0;
volatile uint32_t g_last_arg1 = 0;
volatile int32_t  g_last_syscall_ret = 0;

int32_t syscall_dispatch(uint16_t callno, uint32_t a1, uint32_t a2, uint32_t a3)
{
	g_syscall_dispatch_count++;
	g_last_syscall_id = callno;
	g_last_arg1 = a1;

	int32_t rv;
	switch (callno)
	{
		case SYS_read:
			rv = sys_read_impl((int)a1, (uint8_t*)a2, a3); break;
		case SYS_write:
			rv = sys_write_impl((int)a1, (const uint8_t*)a2, a3); break;
		case SYS_reboot:
			rv = sys_reboot_impl(); break;
		case SYS__exit:
			rv = sys_exit_impl((int)a1); break;
		case SYS_getpid:
			rv = sys_getpid_impl(); break;
		case SYS___time:
			rv = sys_time_impl(); break;
		case SYS_yield:
			rv = sys_yield_impl(); break;
		default:
			rv = -ENOSYS; /* see errno.h and errmsg.h */
			break;
	}
	g_last_syscall_ret = rv;
	return rv;
}

