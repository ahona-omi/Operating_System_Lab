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
#include <stm32_startup.h>
#include <syscall_def.h>
#include <types.h>
#include <times.h>
#include <unixtd.h>
#ifndef DEBUG
#define DEBUG 1
#endif

/*
typedef struct task_tcb{
    uint32_t magic_number; //here it is 0xFECABAA0
    uint16_t task_id; //a unsigned 16 bit integer starting from 1000
    void *psp; //task stack pointer or stackframe address
    uint16_t status; //task status: running, waiting, ready, killed, or terminated
    uint32_t execution_time; //total execution time (in ms)
    uint32_t waiting_time; //total waiting time (in ms)
    uint32_t digital_sinature; //current value is 0x00000001
}TCB_TypeDef;

*/

/****************** ********************/

volatile uint32_t QUEUE_SIZE_P = 3;
volatile uint32_t CURR_TASK_P = 0;
volatile uint16_t TASK_ID = 1000;

volatile TCB_TypeDef READY_QUEUE[MAX_QUEUE_SIZE_P];

// void PUSH_tcb(TCB_TypeDef* tcb, void(*func_ptr)(void))
// {
//     for (uint32_t i = 0; i < QUEUE_SIZE_P; i++)
//     {

//         if (READY_QUEUE[i].task_id == tcb->task_id)
//         {
//             break;
//         }
//         else if (i == QUEUE_SIZE_P - 1)
//         {
//             kprintf("Queue is full\n");
//             return;
//         }
//         else if(READY_QUEUE[i].status == KILLED || READY_QUEUE[i].status == TERMINATED)
//         {
//             create_tcb(READY_QUEUE + i, (void*) func_ptr, tcb->psp);
//             break;
//         }
//     }
// }
// void POP_tcb(TCB_TypeDef* tcb)
// {
//     for (uint32_t i = 0; i < QUEUE_SIZE_P; i++)
//     {
//         if (READY_QUEUE[i] == tcb)
//         {
//             READY_QUEUE[i] = NULL;
//             break;
//         }
//     }
// }

void create_tcb(TCB_TypeDef *tcb, void (*func_ptr)(void), uint32_t *stack_start)
{
    tcb->magic_number = MAGIC_NUMBER;
    tcb->task_id = TASK_ID;
    TASK_ID++;
    tcb->status = READY;
    tcb->execution_time = 0;
    tcb->waiting_time = 0;
    tcb->digital_sinature = DIGITAL_SIGNATURE;

    tcb->psp = stack_start;
    *(--tcb->psp) = 0x01000000;         // xPSR
    *(--tcb->psp) = (uint32_t)func_ptr; // PC
    *(--tcb->psp) = 0xFFFFFFFD;         // LR

    for (uint32_t i = 0; i < 13; i++)
    {
        *(--tcb->psp) = 0;
    }

    __ISB();
}

/**************** *********************/

uint32_t *psp_stack_addresses[2];
uint32_t size = 2;
uint32_t CURR_TASK = 0;

void __set_user_mode(void)
{

    uint32_t psp_stack[1024];
    PSP_Init(psp_stack + 1024);
    asm volatile(
        ".global PSP_Init\n"
        "PSP_Init:\n"
        "msr psp, r0\n"
        "mov r0, #3\n"
        "msr control, r0\n"
        "isb 0xf\n"
        :
        :
        : "memory");
}

// attribute = naked -> active
// attribute = weak -> not active
// #ifndef RR
// #define RR
// void __attribute__((naked)) PendSV_Handler(void)
// {
//     // Clear all pending interrupts
//     SCB->ICSR |= (1 << 27);

//     // kprintf("PendSV_Handler\n");

//     // save current context
//     asm volatile(
//         "mrs r0, psp\n"
//         "isb 0xf\n"
//         "stmdb r0!, {r4-r11}\n");

//     asm volatile("mov %0, r0\n"
//                  : "=r"(psp_stack_addresses[CURR_TASK])
//                  :);

//     __DSB();
//     __ISB();
//     /*---------------------------------------------*/

//     CURR_TASK = (CURR_TASK + 1) % size;

//     asm volatile(
//         "mov r0, %0"
//         :
//         : "r"((uint32_t)psp_stack_addresses[CURR_TASK]));
//     asm volatile(
//         "ldmia r0!,{r4-r11}\n"
//         "msr psp, r0\n"
//         "isb 0xf\n"
//         "bx lr\n");
// }
// #endif

void __attribute__((noreturn)) sleep_state(void)
{
    // set_pending(0);

    __set_pending(0);

    while (1)
    {
        __WFI(); // Wait For Interrupt (Stops CPU Until An Interrupt Occurs)
    }
}

#ifndef RR
#define RR
void __attribute__((naked)) PendSV_Handler(void)
{
    // Clear all pending interrupts
    SCB->ICSR |= (1 << 27);

    // kprintf("PendSV_Handler\n");

    // save current context
    if (READY_QUEUE[CURR_TASK_P].status == RUNNING)
    {
        READY_QUEUE[CURR_TASK_P].status = READY;
        asm volatile(
            "mrs r0, psp\n"
            "isb 0xf\n"
            "stmdb r0!, {r4-r11}\n");

        asm volatile("mov %0, r0\n"
                     : "=r"(READY_QUEUE[CURR_TASK_P].psp)
                     :);
    }
    __DSB();
    __ISB();
    /*---------------------------------------------*/

    // CURR_TASK_P = (CURR_TASK_P + 1) % QUEUE_SIZE_P;

    uint32_t chosen_task = MAX_QUEUE_SIZE_P;
    uint32_t count = 0;

    for (int i = (CURR_TASK_P + 1) % QUEUE_SIZE_P;; i = (i + 1) % QUEUE_SIZE_P)
    {

        if (READY_QUEUE[i].status == READY)
        {
            chosen_task = i;
            break;
        }

        count++;

        if (count >= MAX_QUEUE_SIZE_P)
        {
            break;
        }
    }

    if (chosen_task == 5)
    { // finished

        // while(1);
        // uint32_t new_psp_stack[1024];
        // uint32_t* new_psp = (uint32_t*) new_psp_stack + 1024;
        // *(--new_psp) = 0x01000000;      // xPSR
        // *(--new_psp) = (uint32_t)sleep_state; // PC
        // *(--new_psp) = 0xFFFFFFFD;      // LR

        // for (uint32_t i = 0; i < 13; i++)
        // {
        //     *(--new_psp) = 0;
        // }

        __set_pending(0);
        // __asm volatile(
        //     "mov r0, %0\n"
        //     "msr psp, r0\n"
        //     "isb 0xf\n"
        //     :
        //     : "r" (new_psp)
        // );

        __DSB();
        __ISB();

        // __asm volatile("bx lr\n");
        // sleep_state();

        asm volatile("bl sleep_state");
    }
    else
    {
        CURR_TASK_P = chosen_task;
    }

    __DSB();
    __ISB();

    asm volatile(
        "mov r0, %0"
        :
        : "r"((uint32_t)READY_QUEUE[CURR_TASK_P].psp));

    READY_QUEUE[CURR_TASK_P].status = RUNNING;

    asm volatile(
        "ldmia r0!,{r4-r11}\n"
        "msr psp, r0\n"
        "isb 0xf\n"
        "bx lr\n");
}
#endif

// target extended-remote localhost:3333

void yield(void)
{
    __ISB();

    asm volatile("PUSH {r4-r11}");
    asm volatile("svc %0" : : "i"(SYS_yield));
    asm volatile("POP {r4-r11}");
    __ISB();
}

void task_exit(void)
{
    // READY_QUEUE[CURR_TASK_P].status = KILLED;

    __ISB();

    TCB_TypeDef *tcb = READY_QUEUE + CURR_TASK_P;
    __asm volatile(
        "MOV R2, %0\n"
        :
        : "r"(tcb));
    asm volatile("PUSH {r4-r11}");
    asm volatile("svc %0" : : "i"(SYS__exit));
    asm volatile("POP {r4-r11}");

    kprintf("task exited : %d\n", READY_QUEUE[CURR_TASK_P].task_id);

    __DSB();
    __ISB();

    yield();
}

void task0(void)
{
    for (uint32_t i = 0; i < 100; i++)
    {
        kprintf("Task 0 call %d\n", i);
    }

    kprintf("task 0 finished\n");

    // while(1);
    task_exit();

    while (1)
        ;
}

void task1(void)
{
    for (uint32_t i = 0; i < 100; i++)
    {
        kprintf("Task 1 call %d\n", i);
    }

    kprintf("task 1 finished\n");
    task_exit();

    while (1)
        ;
}

void task2(void)
{
    for (uint32_t i = 0; i < 100; i++)
    {
        kprintf("Task 2 call %d\n", i);
    }

    kprintf("task 2 finished\n");
    task_exit();

    while (1)
        ;
}

void task_for_fork(void)
{

    kprintf("inside task for fork\n");

    int pid = fork(&pid); // pc
    if (pid == 0)
    {
        kprintf("child process\n");
        kprintf("pid returned: %d\n", pid);
        // while(1);
    }
    else
    {

        kprintf("parent process\n");
        kprintf("pid returned: %d\n", pid);

        // while(1);
    }

    task_exit();
}

void init_scheduler_for_fork(void)
{
    CURR_TASK_P = 0;
    QUEUE_SIZE_P = 1;

    uint32_t psp_parent_task[1024];

    create_tcb(READY_QUEUE + CURR_TASK_P, (void *)task_for_fork, (uint32_t *)(psp_parent_task + 1024));
    QUEUE_SIZE_P++;

    // __set_pending(1);

    READY_QUEUE[CURR_TASK_P].status = RUNNING;

    // kprintf("pid : %d\n", READY_QUEUE[CURR_TASK_P].task_id);

    start_task((uint32_t)(READY_QUEUE[CURR_TASK_P].psp));
}

void start_task(uint32_t psp)
{

    asm volatile("MOV R0, %0"
                 :
                 : "r"(psp));
    asm volatile("PUSH {r4-r11}");
    asm volatile("svc %0" : : "i"(SYS_start));
    asm volatile("POP {r4-r11}");
}

void set_pending(uint8_t value)
{
    asm volatile("MOV R0, %0"
                 :
                 : "r"(value));
    asm volatile("PUSH {r4-r11}");
    asm volatile("svc %0" : : "i"(SYS_set_pending));
    asm volatile("POP {r4-r11}");
}

int getPID(void)
{
    int pid = 0;
    asm volatile("PUSH {r4-r11}");
    asm volatile("svc %0" : : "i"(SYS_getpid));
    asm volatile("POP {r4-r11}");

    asm volatile("MOV %0, R0"
                 : "=r"(pid));

    return pid;
}

int fork(uint32_t *pid)
{

    *(pid) = (uint32_t)0;

    asm volatile("PUSH {r4-r11}");
    asm volatile("svc %0" : : "i"(SYS_fork));
    asm volatile("POP {r4-r11}");

    asm volatile("MOV R0, 0");

    // yield(); // i want to return to here after fork of child process

    __DSB();
    __ISB();

    asm volatile("MOV %0, R2"
                 : "=r"(*pid));

    if (CURR_TASK_P == 0)
    {
        return *pid;
    }
    else
    {
        *pid = 0;
        return 0;
    }

    return *pid;
}

void init_tasks(void)
{

    uint64_t psp0_stack[1024], psp1_stack[1024];

    psp_stack_addresses[0] = psp0_stack + 1024;

    kprintf("psp0_stack: %x\n", psp_stack_addresses[0]);
    *(--psp_stack_addresses[0]) = 0x01000000;      // xPSR
    *(--psp_stack_addresses[0]) = (uint32_t)task0; // PC
    *(--psp_stack_addresses[0]) = 0xFFFFFFFD;      // LR

    for (uint32_t i = 0; i < 13; i++)
    {
        *(--psp_stack_addresses[0]) = 0;
    }

    kprintf("psp0_stack: %x\n", psp_stack_addresses[0]);

    psp_stack_addresses[1] = psp1_stack + 1024;
    *(--psp_stack_addresses[1]) = 0x01000000;      // xPSR
    *(--psp_stack_addresses[1]) = (uint32_t)task1; // PC
    *(--psp_stack_addresses[1]) = 0xFFFFFFFD;      // LR

    for (uint32_t i = 0; i < 13; i++)
    {
        *(--psp_stack_addresses[1]) = 0;
    }

    set_pending(1);

    start_task(psp_stack_addresses[CURR_TASK]);
}

void init_scheduler(void)
{
    uint64_t psp0_stack[1024], psp1_stack[1024], psp2_stack[1024];

    kprintf("psp0_stack: %x\n", psp0_stack + 1024);
    kprintf("psp1_stack: %x\n", psp1_stack + 1024);
    kprintf("psp2_stack: %x\n", psp2_stack + 1024);

    create_tcb(READY_QUEUE + CURR_TASK_P, (void *)task0, psp0_stack + 1024);
    CURR_TASK_P = (CURR_TASK_P + 1) % QUEUE_SIZE_P;
    create_tcb(READY_QUEUE + CURR_TASK_P, (void *)task1, psp1_stack + 1024);
    CURR_TASK_P = (CURR_TASK_P + 1) % QUEUE_SIZE_P;
    create_tcb(READY_QUEUE + CURR_TASK_P, (void *)task2, psp2_stack + 1024);
    CURR_TASK_P = (CURR_TASK_P + 1) % QUEUE_SIZE_P;

    kprintf("psp0_stack: %x\n", READY_QUEUE[0].psp);
    kprintf("psp1_stack: %x\n", READY_QUEUE[1].psp);
    kprintf("psp2_stack: %x\n", READY_QUEUE[2].psp);

    set_pending(1);

    READY_QUEUE[CURR_TASK_P].status = RUNNING;

    start_task((uint32_t)(READY_QUEUE[CURR_TASK_P].psp));
}

///// for execve /////

volatile file_entry_t file_list[MAX_FILES];
volatile uint32_t file_count = 0;

int find_file(char *filename)
{
    for (uint32_t i = 0; i < file_count; i++)
    {
        if (strcomp((uint8_t *)file_list[i].name, (uint8_t *)filename) == 0)
        {
            return i;
        }
    }
    return -1;
}

int execve(char *filename, char *argv[], char *envp[])
{
    int return_val = 0;
    __asm volatile(
        "mov r0, %0\n"
        "mov r1, %1\n"
        "mov r2, %2\n"
        "push {r4,r11}\n"
        "svc %4\n"
        "pop {r4,r11}\n"
        "mov %3, r0\n"
        : "=r"(filename), "=r"(argv), "=r"(envp), "=r"(return_val)
        : "i"(SYS_execv));

    return return_val;
}

void file_A(void)
{
    kprintf("A started\n");

    char *argv[] = {"PRINT_B", "Hello World", NULL};
    char *envp[] = {NULL};

    int ret = execve(argv[0], argv, envp);

    if (ret == -1)
    {
        kprintf("execve failed\n");
    }

    kprintf("A finished -- this should not be printed\n"); // should not be printed
    task_exit();
}

void file_B(void)
{
    kprintf("B started\n");

    kprintf("B finished\n"); // should not be printed
    task_exit();
}

void file_C(void)
{
    kprintf("C started\n");

    kprintf("C finished\n"); // should not be printed
    task_exit();
}

void init_file_system(void)
{
    file_entry_t file1;
    file1.address = (uint32_t *)file_B;
    file1.size = 1024;
    file1.mode = O_RDONLY;
    strcopy((uint8_t *)file1.name, (const uint8_t *)"PRINT_B");
    file_list[file_count++] = file1;

    file_entry_t file2;
    file2.address = (uint32_t *)file_C;
    file2.size = 1024;
    file2.mode = O_RDONLY;
    strcopy((uint8_t *)file2.name, (const uint8_t *)"PRINT_C");
    file_list[file_count++] = file2;

    __ISB();
}

void init_task_for_execv(void)
{
    QUEUE_SIZE_P = 1;
    CURR_TASK_P = 0;

    uint32_t psp_stack[1024];

    create_tcb(READY_QUEUE + CURR_TASK_P, (void *)file_A, psp_stack + 1024);

    READY_QUEUE[CURR_TASK_P].status = RUNNING;

    start_task((uint32_t)(READY_QUEUE[CURR_TASK_P].psp));
}

void kmain(void)
{
    __sys_init();

    init_file_system();

    __NVIC_SetPriority(SVCall_IRQn, 0x1);
    NVIC_EnableIRQ(SVCall_IRQn);
    __NVIC_SetPriority(PendSV_IRQn, 0xFF);
    NVIC_EnableIRQ(PendSV_IRQn);
    __NVIC_SetPriority(SysTick_IRQn, 0xFF);
    NVIC_EnableIRQ(SysTick_IRQn);

    __ISB();

    kprintf("Hello World\n");

    __set_user_mode();

    // char c;
    // kscanf("%c", &c);

    // kprintf("You entered: %c\n", c);

    // init_tasks();

    // init_scheduler(); // round robin scheduler /// for a round robin example

    // init_scheduler_for_fork(); // for a forking example

    // init_task_for_execv(); // for execv example

    // typedef struct student{
    //     char name[20];
    //     int id;
    //     float cgpa;
    // } student;

    // student* s1 = (struct student *)du_malloc(sizeof(student));
    // s1->id = 1;
    // s1->cgpa = 3.5;

    // kprintf("Student ID: %d\n", s1->id);
    // kprintf("Student CGPA: %f\n", s1->cgpa);

    // du_free(s1);

    // kprintf("Student ID: %d\n", s1->id);
    // kprintf("Student CGPA: %f\n", s1->cgpa);

    // uint8_t *ptr = du_malloc(20);

    // kprintf("ptr: %x\n", ptr);

    // char c;
    // du_scanf("%c", &c);

    // du_printf("You entered: %c\n", c);

    du_printf("Hello from userland\n");

    while (1)
    {
    }
}
