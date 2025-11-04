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
		putstr(buf, (size_t)len);
		return (int32_t)len;
	}
	return -ENOSYS;
}

static int32_t sys_time_impl(void)
{
	return (int32_t)__getTime();
}

static int32_t sys_reboot_impl(void)
{
	NVIC_SystemReset();
	return 0; /* not reached */
}

static int32_t sys_yield_impl(void)
{
	/* Pend a PendSV for cooperative yield; minimal stub scheduler */
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	return 0;
}

static int32_t sys_getpid_impl(void)
{
	/* No tasking yet; return a fixed pid */
	return 1;
}

static int32_t sys_exit_impl(int code)
{
	(void)code;
	/* In absence of a full scheduler, just loop low-power */
	while (1) {
		__WFI();
	}
	/* unreachable, but keep compiler happy */
	return 0;
}

int32_t syscall_dispatch(uint16_t callno, uint32_t a1, uint32_t a2, uint32_t a3)
{
	switch (callno)
	{
		case SYS_read:
			return -ENOSYS; /* not implemented */
		case SYS_write:
			return sys_write_impl((int)a1, (const uint8_t*)a2, a3);
		case SYS_reboot:
			return sys_reboot_impl();
		case SYS__exit:
			return sys_exit_impl((int)a1);
		case SYS_getpid:
			return sys_getpid_impl();
		case SYS___time:
			return sys_time_impl();
		case SYS_yield:
			return sys_yield_impl();
		default:
			return -ENOSYS; /* see errno.h and errmsg.h */
	}
}

