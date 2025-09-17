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

/************************************************************************************
* __SysTick_init(uint32_t reload) 
* Function initialize the SysTick clock. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

void __SysTick_init(uint32_t reload)
{
    
}
void SysTickIntDisable(void)
{

}

void SysTickIntEnable(void)
{

}
/************************************************************************************
* __sysTick_enable(void) 
* The function enables the SysTick clock if already not enabled. 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/
void __SysTick_enable(void)
{

}
void __sysTick_disable(void)
{

}
uint32_t __getSysTickCount(void)
{
    return 0; // TODO: Implement SysTick count retrieval
}
/************************************************************************************
* __updateSysTick(uint32_t count) 
* Function reinitialize the SysTick clock. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

void __updateSysTick(uint32_t count)
{
 
}

/************************************************************************************
* __getTime(void) 
* Function return the SysTick elapsed time from the begining or reinitialing. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

uint32_t __getTime(void)
{
    return 0; // TODO: Implement time retrieval
}

uint32_t __get__Second(void){
    return 0; // TODO: Implement seconds retrieval
}
uint32_t __get__Minute(void){
    return 0; // TODO: Implement minutes retrieval
}
uint32_t __get__Hour(void){
    return 0; // TODO: Implement hours retrieval
}
void SysTick_Handler(void)
{
  
}

void __enable_fpu()
{
    SCB->CPACR |= ((0xFUL<<20));
}

uint8_t ms_delay(uint32_t delay)
{
    return 0; // TODO: Implement millisecond delay
}

uint32_t getmsTick(void)
{
    return 0; // TODO: Implement millisecond tick retrieval
}

uint32_t wait_until(uint32_t delay)
{
    return 0; // TODO: Implement wait until delay
}

void SYS_SLEEP_WFI(void)
{
    __WFI();
}
