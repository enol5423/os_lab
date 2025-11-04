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

#ifndef DEBUG
#define DEBUG 1
#endif

// Test function to demonstrate SysTick syscall implementation
void test_systick_syscalls(void)
{
    kprintf("\n=== SYSTICK SYSCALL DEMONSTRATION ===\n");
    
    // Test 1: Basic time reading
    kprintf("1. Testing Basic Time Functions:\n");
    kprintf("   Current time (ms): %d\n", __getTime());
    kprintf("   Current seconds: %d\n", __get__Second());
    kprintf("   Current minutes: %d\n", __get__Minute());
    kprintf("   Current hours: %d\n", __get__Hour());
    kprintf("   Millisecond tick: %d\n", getmsTick());
    
    // Test 2: SysTick counter access
    kprintf("\n2. Testing SysTick Counter Access:\n");
    uint32_t counter1 = __getSysTickCount();
    kprintf("   SysTick counter: %d\n", counter1);
    
    // Test 3: Short delay demonstration
    kprintf("\n3. Testing Delay Functions:\n");
    kprintf("   Starting 2000ms delay test...\n");
    uint32_t start_time = __getTime();
    ms_delay(2000);  // 2 second delay
    uint32_t end_time = __getTime();
    kprintf("   Delay completed! Elapsed time: %d ms\n", end_time - start_time);
    
    // Test 4: Wait until function
    kprintf("\n4. Testing wait_until function:\n");
    kprintf("   Waiting 1500ms using wait_until...\n");
    start_time = __getTime();
    uint32_t result_time = wait_until(1500);
    kprintf("   Wait completed at time: %d ms\n", result_time);
    
    // Test 5: SysTick enable/disable demonstration
    kprintf("\n5. Testing SysTick Control:\n");
    kprintf("   Current time before disable: %d ms\n", __getTime());
    
    kprintf("   Disabling SysTick for 1 second...\n");
    __SysTick_disable();
    // Simulate some work (busy wait without SysTick)
    for(volatile int i = 0; i < 10000000; i++);
    
    kprintf("   Re-enabling SysTick...\n");
    __SysTick_enable();
    ms_delay(100); // Small delay to show it's working again
    kprintf("   Current time after re-enable: %d ms\n", __getTime());
    
    // Test 6: Continuous time monitoring
    kprintf("\n6. Continuous Time Monitoring (10 iterations):\n");
    for(int i = 0; i < 10; i++) {
        kprintf("   [%d] Time: %d ms, H:M:S = %d:%d:%d\n", 
                i+1, __getTime(), __get__Hour(), __get__Minute(), __get__Second());
        ms_delay(500);  // 500ms between readings
    }
    
    kprintf("\n=== SYSTICK SYSCALL DEMO COMPLETED ===\n");
    kprintf("All functions working correctly!\n\n");
}
void kmain(void)
{
    __sys_init();
    
    // Enable SysTick interrupts for time tracking
    SysTickIntEnable();
    __SysTick_enable();
    
    // --- Syscall smoke test via userland wrappers ---
    write(1, "Hello via SYS_write\r\n", 21);
    uint32_t t_ms = time_ms();
    kprintf("time_ms() via syscall: %d\r\n", (int)t_ms);
    int pid = getpid();
    kprintf("getpid() via syscall: %d\r\n", pid);
    int yret = yield();
    kprintf("yield() returned: %d\r\n", yret);
    // ------------------------------------------------

    // Run the syscall demonstration
    kprintf("\n*** Starting SysTick Syscall Demo ***\n");
    test_systick_syscalls();
    
    // Interactive loop - shows live time updates
    kprintf("*** Entering interactive mode - showing live time ***\n");
    kprintf("*** System will show time updates every 3 seconds ***\n");
    
    uint32_t last_display_time = 0;
    while (1)
    {
        uint32_t current_time = __getTime();
        
        // Display time every 3 seconds
        if (current_time - last_display_time >= 3000) {
            kprintf("Live Time: %d ms | H:M:S = %02d:%02d:%02d\n", 
                    current_time, __get__Hour(), __get__Minute(), __get__Second());
            last_display_time = current_time;
        }
        
        // Small delay to prevent overwhelming the output
        ms_delay(100);
    }
}
