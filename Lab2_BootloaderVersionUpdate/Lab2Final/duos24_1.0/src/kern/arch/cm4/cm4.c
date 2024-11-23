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

static volatile uint32_t __mscount;
static volatile uint32_t __sec_count;
static volatile uint32_t __min_count;
static volatile uint32_t __hour_count;
/************************************************************************************
* __SysTick_init(uint32_t reload) 
* Function initialize the SysTick clock. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

void __SysTick_init(uint32_t reload)
{
    SYSTICK->CTRL &= ~(1<<0); //disable systick timer
    SYSTICK->CTRL|=SysTick_CTRL_ENABLE_Msk; //enable systick counter
    SYSTICK->CTRL |= SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_CLKSOURCE_Msk; //enable interrupt and internal clock source
    SYSTICK->LOAD = PLL_N*reload;
    SYSTICK->VAL =0; // initialize the counter
    NVIC_SetPriority(SysTick_IRQn,1);
    NVIC_EnableIRQ(SysTick_IRQn);
    __mscount=0;
    __sec_count=0;
    __min_count=0;
    __hour_count=0;
}
void SysTickIntDisable(void)
{
    SYSTICK->CTRL &= ~(SysTick_CTRL_TICKINT_Msk);
}

void SysTickIntEnable(void)
{
    SYSTICK->CTRL |= (SysTick_CTRL_TICKINT_Msk);
}
/************************************************************************************
* __sysTick_enable(void) 
* The function enables the SysTick clock if already not enabled. 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/
void __SysTick_enable(void)
{
    if(!(SYSTICK->CTRL & (1<<0))) SYSTICK->CTRL |= 1<<0;
}
void __sysTick_disable(void)
{
    if((SYSTICK->CTRL & (1<<0))) SYSTICK->CTRL &= ~(1<<0);
}
uint32_t __getSysTickCount(void)
{
    return SYSTICK->VAL;
}
/************************************************************************************
* __updateSysTick(uint32_t count) 
* Function reinitialize the SysTick clock. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

void __updateSysTick(uint32_t count)
{
    SYSTICK->CTRL &= ~(1<<0); //disable systick timer
    SYSTICK->VAL =0; // initialize the counter
    __mscount=0;
    SYSTICK->CTRL |= SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_CLKSOURCE_Msk;; //enable interrupt and internal clock source
    SYSTICK->LOAD = PLL_N*count;
    SYSTICK->CTRL|=SysTick_CTRL_ENABLE_Msk; //enable systick counter
}

/************************************************************************************
* __getTime(void) 
* Function return the SysTick elapsed time from the begining or reinitialing. The function with a weak attribute enables 
* redefining the function to change its characteristics whenever necessary.
**************************************************************************************/

uint32_t __getTime(void)
{
    //for(uint32_t i=0;i<10000000;i++);
    //kprintf("Time %d\n",__mscount);
    return (uint32_t)(__mscount+(SYSTICK->LOAD-SYSTICK->VAL)/(PLL_N*1000));
}
uint32_t __get__Second(void){
    return __sec_count;
}
uint32_t __get__Minute(void){
    return __min_count;
}
uint32_t __get__Hour(void){
    return __hour_count;
}
void SysTick_Handler(void)
{
    __mscount+=(SYSTICK->LOAD)/(PLL_N*1000);
}

void __enable_fpu()
{
    SCB->CPACR |= ((0xFUL<<20));
}

uint8_t ms_delay(uint32_t delay)
{
	uint32_t cms=__getTime();
	while((__getTime()-cms)<delay);
	return 1;
}

uint32_t getmsTick(void)
{
    return __getTime();
}

uint32_t wait_until(uint32_t delay)
{
    uint32_t st_time=__getTime();
    if(delay<=0) return 0;
     while(1)
     {
        if((__getTime()-st_time)>delay) return 0;
     }
    return 1;
}

void SYS_SLEEP_WFI(void)
{
    __WFI();
}