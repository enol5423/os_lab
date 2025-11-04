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
 
#include <unistd.h>
#include <syscall_def.h>

/* Generic SVC invocation helper: r0=callno, r1..r3=args. Returns r0. */
static inline int svc_invoke(uint32_t callno, uint32_t a1, uint32_t a2, uint32_t a3)
{
	register uint32_t r0 __asm__("r0") = callno;
	register uint32_t r1 __asm__("r1") = a1;
	register uint32_t r2 __asm__("r2") = a2;
	register uint32_t r3 __asm__("r3") = a3;
	__asm volatile ("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "memory");
	return (int)r0;
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return (ssize_t)svc_invoke(SYS_write, (uint32_t)fd, (uint32_t)buf, (uint32_t)count);
}

ssize_t read(int fd, void *buf, size_t count)
{
	return (ssize_t)svc_invoke(SYS_read, (uint32_t)fd, (uint32_t)buf, (uint32_t)count);
}

void _exit(int status)
{
	(void)svc_invoke(SYS__exit, (uint32_t)status, 0U, 0U);
	while (1) { /* no return */ }
}

pid_t getpid(void)
{
	return (pid_t)svc_invoke(SYS_getpid, 0U, 0U, 0U);
}

uint32_t time_ms(void)
{
	return (uint32_t)svc_invoke(SYS___time, 0U, 0U, 0U);
}

int reboot(void)
{
	return svc_invoke(SYS_reboot, 0U, 0U, 0U);
}

int yield(void)
{
	return svc_invoke(SYS_yield, 0U, 0U, 0U);
}

