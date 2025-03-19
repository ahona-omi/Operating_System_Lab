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
 
#include <times.h>
/* Define you function details here */

uint32_t du_getSystickTime(void)
{
    uint32_t time;
    
    asm volatile(
        "PUSH {r4-r11, ip, lr}\n"
        "svc %0\n"
        "POP {r4-r11, ip, lr}\n"
        :
        : "i" (SYS___time)
    );

    asm volatile(
        "mov %0, r0\n"
        : "=r"(time)
    );

    return time;
}


void du_reboot(void){
    asm volatile(
        "PUSH {r4-r11, ip, lr}\n"
        "svc %0\n"
        "POP {r4-r11, ip, lr}\n"
        :
        : "i" (SYS_reboot)
    );
}


void* du_malloc(uint32_t size){
    uint8_t sys_malloc = 69;
    void *ptr;
    __asm volatile(
        "mov r2, %0\n"
        "PUSH {r4-r11, ip, lr}\n"
        "svc %1\n"
        "POP {r4-r11, ip, lr}\n"
        :
        : "r" (size), "i" (SYS_malloc)
    );

    __asm volatile(
        "mov %0, r2\n"
        : "=r"(ptr)
    );

    return ptr;
}


void du_free(void* ptr){

    kprintf("memory to free --times : %x\n", ptr);
    asm volatile(
        "mov r2, %0\n"
        "PUSH {r4-r11, ip, lr}\n"
        "svc %1\n"
        "POP {r4-r11, ip, lr}\n"
        :
        : "r" (ptr) , "i" (SYS_free)
    );

    ptr  = NULL;
    kprintf("memory to free --times : %x\n", ptr);
}
