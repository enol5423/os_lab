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
 
#include <cm4.h>
#include <sys_clock.h>
#include <syscall.h>

// Global variables for time tracking
static volatile uint32_t systick_ms_counter = 0;
static volatile uint32_t systick_seconds = 0;
static volatile uint32_t systick_minutes = 0;
static volatile uint32_t systick_hours = 0;

/************************************************************************************
* __SysTick_init(uint32_t reload) 
* Function initialize the SysTick clock. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

void __SysTick_init(uint32_t reload)
{
    // Disable SysTick first
    SYSTICK->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    
    // Set reload value (24-bit value, must be > 0)
    if (reload == 0) reload = 1;
    SYSTICK->LOAD = (reload - 1) & SysTick_LOAD_RELOAD_Msk;
    
    // Clear current value
    SYSTICK->VAL = 0;
    
    // Configure SysTick to use processor clock source
    SYSTICK->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
    
    // Reset time counters
    systick_ms_counter = 0;
    systick_seconds = 0;
    systick_minutes = 0;
    systick_hours = 0;
}
void SysTickIntDisable(void)
{
    // Disable SysTick interrupt
    SYSTICK->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}

void SysTickIntEnable(void)
{
    // Enable SysTick interrupt
    SYSTICK->CTRL |= SysTick_CTRL_TICKINT_Msk;
}
/************************************************************************************
* __sysTick_enable(void) 
* The function enables the SysTick clock if already not enabled. 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/
void __SysTick_enable(void)
{
    // Enable SysTick counter
    SYSTICK->CTRL |= SysTick_CTRL_ENABLE_Msk;
}
void __SysTick_disable(void)
{
    // Disable SysTick counter
    SYSTICK->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}
uint32_t __getSysTickCount(void)
{
    // Read current SysTick counter value
    return SYSTICK->VAL;
}
/************************************************************************************
* __updateSysTick(uint32_t count) 
* Function reinitialize the SysTick clock. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

void __updateSysTick(uint32_t count)
{
    // Update SysTick reload value
    if (count == 0) count = 1;
    SYSTICK->LOAD = (count - 1) & SysTick_LOAD_RELOAD_Msk;
    // Clear current value to restart counting
    SYSTICK->VAL = 0;
}

/************************************************************************************
* __getTime(void) 
* Function return the SysTick elapsed time from the begining or reinitialing. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

uint32_t __getTime(void)
{
    // Return elapsed time in milliseconds since initialization
    return systick_ms_counter;
}

uint32_t __get__Second(void){
    // Return current seconds
    return systick_seconds;
}
uint32_t __get__Minute(void){
    // Return current minutes
    return systick_minutes;
}
uint32_t __get__Hour(void){
    // Return current hours
    return systick_hours;
}
void SysTick_Handler(void)
{
    // Increment millisecond counter
    systick_ms_counter++;
    
    // Handle seconds rollover (1000ms = 1s)
    if ((systick_ms_counter % 1000) == 0) {
        systick_seconds++;
        
        // Handle minutes rollover (60s = 1min)
        if (systick_seconds >= 60) {
            systick_seconds = 0;
            systick_minutes++;
            
            // Handle hours rollover (60min = 1hr)
            if (systick_minutes >= 60) {
                systick_minutes = 0;
                systick_hours++;
                
                // Handle day rollover (24hr = 1day) - reset to 0
                if (systick_hours >= 24) {
                    systick_hours = 0;
                }
            }
        }
    }
}

void __enable_fpu()
{
    SCB->CPACR |= ((0xFUL<<20));
}

uint8_t ms_delay(uint32_t delay)
{
    uint32_t start_time = systick_ms_counter;
    uint32_t target_time = start_time + delay;
    
    // Handle counter overflow case
    if (target_time < start_time) {
        // Wait for overflow to occur
        while (systick_ms_counter >= start_time) {
            __NOP(); // Prevent compiler optimization
        }
        // Now wait for target time
        while (systick_ms_counter < (target_time & 0xFFFFFFFF)) {
            __NOP();
        }
    } else {
        // Normal case - no overflow
        while (systick_ms_counter < target_time) {
            __NOP();
        }
    }
    
    return 1; // Success
}

uint32_t getmsTick(void)
{
    // Return millisecond tick counter
    return systick_ms_counter;
}

uint32_t wait_until(uint32_t delay)
{
    uint32_t start_time = systick_ms_counter;
    uint32_t target_time = start_time + delay;
    
    // Handle counter overflow case
    if (target_time < start_time) {
        // Wait for overflow to occur
        while (systick_ms_counter >= start_time) {
            __NOP(); // Prevent compiler optimization
        }
        // Now wait for target time
        while (systick_ms_counter < (target_time & 0xFFFFFFFF)) {
            __NOP();
        }
    } else {
        // Normal case - no overflow
        while (systick_ms_counter < target_time) {
            __NOP();
        }
    }
    
    return systick_ms_counter; // Return current time
}

void SYS_SLEEP_WFI(void)
{
    __WFI();
}

/* Minimal PendSV handler for cooperative yield demo.
 * A real context switcher would save/restore r4-r11 and swap PSPs.
 */
void PendSV_Handler(void)
{
    /* Simply clear the pending flag and return. */
    SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
}
