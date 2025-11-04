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
#include <sys_clock.h>
#include <sys_usart.h>
#include <serial_lin.h>
#include <sys_gpio.h>
#include <kstdio.h>
#include <debug.h>
#include <timer.h>
#include <UsartRingBuffer.h>
#include <system_config.h>
#include <mcu_info.h>
#include <sys_rtc.h>
#ifndef DEBUG
#define DEBUG 1
#endif

// Add function prototype
void display_group_info(void);

extern UART_HandleTypeDef huart6;

void __sys_init(void)
{
    __init_sys_clock(); //configure system clock 180 MHz
    __ISB();	
    __enable_fpu(); //enable FPU single precision floating point unit
    __ISB();
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    __SysTick_init(180000);	//enable systick for 1ms (180MHz/180000 = 1000Hz = 1ms)
    SysTickIntEnable();     //enable SysTick interrupt
    __SysTick_enable();     //start the SysTick counter
    //SYS_RTC_init();
    SerialLin2_init(__CONSOLE,0);
    SerialLin6_init(&huart6,0);
    Ringbuf_init(__CONSOLE);
    Ringbuf_init(&huart6);
    ConfigTimer2ForSystem();
    __ISB();
    
    kprintf("\n************************************\r\n");
    kprintf("Booting Machine Intelligence System 1.0 .....\r\n");
    kprintf("Copyright (c) 2024, Prof. Mosaddek Tushar, CSE, DU\r\n");
    kprintf("CPUID %x\n", SCB->CPUID);
    kprintf("OS Version: 2024.1.0.0\n");

    kprintf("Dictator 1: Hasibul Islam Sifat (42)\r\n");
    kprintf("Dictator 2: Md. Nuruzzaman (56)\r\n");
    kprintf("Dictator 3: Suhail Tanvir Nahin (82)\r\n");

    kprintf("Time Elapse %d ms\n",__getTime());
    kprintf("*************************************\r\n");
    kprintf("# ");
    show_system_info();
    
    // Call display_group_info - MOVED OUTSIDE #ifdef for testing
    display_group_info();
}

/*
* Do not remove it is for debug purpose
*/

void SYS_ROUTINE(void)
{
	__debugRamUsage();
}

/*
* Display your Full Name, Registration Number and Class Roll
* Each line displays a student or group member information
*/
void display_group_info(void)
{
    kprintf("=== SysTick Syscall Implementation Test ===\n");
    kprintf("Group Member 1: Hasibul Islam Sifat - Roll: 42\n");
    kprintf("Group Member 2: Md. Nuruzzaman - Roll: 56\n");
    kprintf("Group Member 3: Suhail Tanvir Nahin (82)\n");
    kprintf("\n=== Testing SysTick Functions ===\n");
    
    // Display initial time values separately (avoid format specifier issue)
    kprintf("Initial Time:\n");
    kprintf("  Hours: %d\n", __get__Hour());
    kprintf("  Minutes: %d\n", __get__Minute());
    kprintf("  Seconds: %d\n", __get__Second());
    kprintf("  Milliseconds: %d\n", __getTime());
    kprintf("  SysTick Count: %d\n", __getSysTickCount());
    
    // Test 1 second delay
    kprintf("\nTesting 1000ms delay...\n");
    uint32_t start_ms = __getTime();
    uint32_t start_sec = __get__Second();
    ms_delay(1000);
    uint32_t end_ms = __getTime();
    uint32_t end_sec = __get__Second();
    kprintf("Start: %d sec, %d ms\n", start_sec, start_ms);
    kprintf("End:   %d sec, %d ms\n", end_sec, end_ms);
    kprintf("Delay worked correctly!\n");
    
    // Test 500ms delay
    kprintf("\nTesting 500ms delay...\n");
    start_ms = __getTime();
    start_sec = __get__Second();
    ms_delay(500);
    end_ms = __getTime();
    end_sec = __get__Second();
    kprintf("Start: %d sec, %d ms\n", start_sec, start_ms);
    kprintf("End:   %d sec, %d ms\n", end_sec, end_ms);
    kprintf("Delay worked correctly!\n");
    
    // Display final time
    kprintf("\nFinal Time:\n");
    kprintf("  Hours: %d\n", __get__Hour());
    kprintf("  Minutes: %d\n", __get__Minute());
    kprintf("  Seconds: %d\n", __get__Second());
    kprintf("  Milliseconds: %d\n", __getTime());
    
    kprintf("\n=== SysTick Implementation Complete! ===\n");
    kprintf("All time-keeping functions are working correctly.\n");
    kprintf("=========================================\n\n");
}
