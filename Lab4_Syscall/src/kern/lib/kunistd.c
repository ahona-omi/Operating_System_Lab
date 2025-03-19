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
#include <kunistd.h>
/* Add your functions here */

void __sys_start_task(uint32_t psp)
{
	asm volatile ("MOV R0, %0"
		:
		:"r" (psp)
	);
	asm volatile ("LDMIA R0!,{R4-R11}");
	asm volatile ("MSR PSP, R0");
	asm volatile ("ISB 0xf" ::: "memory");
	asm volatile ("MOV LR, 0xFFFFFFFD"); ///why this line is needed?
	asm volatile ("BX LR");
}


int __sys_fork(uint32_t* parents_psp){

	uint32_t psp_stack_for_child[1024];

	uint32_t* psp_for_child = psp_stack_for_child + 1024;

	// kprintf("next pc: %x\n", next_pc);

	// __sys_start_task((uint32_t)parents_psp);

	// asm volatile ("MOV R0, %0"
	// 	:
	// 	:"r" ((uint32_t)parents_psp)
	// );

	// asm volatile ("MSR PSP, R0");
	// asm volatile ("ISB 0xf" ::: "memory");
	// asm volatile ("MOV LR, 0xFFFFFFFD"); ///why this line is needed?
	// asm volatile ("BX LR");

	

	// kprintf("psp_child_stack: %x\n", READY_QUEUE[CURR_TASK_P + 1].psp);

	// kprintf("psp_parent_stack: %x\n", READY_QUEUE[CURR_TASK_P].psp);

	// kprintf("psp_for_parent svc: %x\n", parents_psp );

	parents_psp[0] = TASK_ID + 1 ; //parents return child id

	TCB_TypeDef* tcb = (TCB_TypeDef*)(READY_QUEUE + CURR_TASK_P + 1);
	tcb->magic_number = MAGIC_NUMBER;
	tcb->task_id = TASK_ID;
	TASK_ID++;
	kprintf("task_id: %d\n", tcb->task_id);

	tcb->status = READY;
	tcb->execution_time = 0;
	tcb->waiting_time = 0;
	tcb->digital_sinature = DIGITAL_SIGNATURE;

	tcb->psp = psp_for_child;

	*(--tcb->psp) = parents_psp[7];      // xPSR
	*(--tcb->psp) = (uint32_t)parents_psp[6]; // PC
	*(--tcb->psp) = parents_psp[5];      // LR
	*(--tcb->psp) = parents_psp[4];      // R12
	*(--tcb->psp) = parents_psp[3];      // R3
	*(--tcb->psp) = parents_psp[2];      // R2
	*(--tcb->psp) = parents_psp[1];      // R1
	*(--tcb->psp) = 0;      			 // R0 ... child returns 0

	for(uint32_t i = 0; i < 8; i++)
	{
		*(--tcb->psp) = 0;
	}

	// *(--tcb->psp) = parents_psp[15];      // r4
	// *(--tcb->psp) = parents_psp[14]; 	// r5
	// *(--tcb->psp) = parents_psp[13];      // r6
	// *(--tcb->psp) = parents_psp[12];      // R7
	// *(--tcb->psp) = parents_psp[11];      // R8
	// *(--tcb->psp) = parents_psp[10];      // R9
	// *(--tcb->psp) = parents_psp[9];      // R10
	// *(--tcb->psp) = parents_psp[8];      			 // R11

	// kprintf("psp_child_stack: %x\n", READY_QUEUE[CURR_TASK_P + 1].psp);

	// kprintf("psp_parent_stack: %x\n", parents_psp);

	QUEUE_SIZE_P++;

	__DSB();
	__ISB();

	
	// SCB->ICSR |= (1 << 27) ; // Clear all pending interrupts
	SCB->ICSR |= (1 << 28) ; // set PendSV bit

	kprintf("forked\n");

	return tcb->task_id;

}



void* heap_malloc(uint32_t size)
{
    if (size == 0)
    {
        return NULL; // Cannot allocate zero size
    }

    // Ensure size is aligned to 8 bytes
    size = (size + 7) & ~7;

    if (size > heap->available)
    {
        kprintf("Heap is full or requested size too large\n");
        return NULL;
    }

	if(heap->free_list != NULL){
		heap_chunk* temp = heap->free_list;
		heap_chunk* prev_temp = NULL;

		while(temp != NULL){
			if((temp->size - sizeof(heap_chunk)) >= size && temp->isUse == 0){

				temp->isUse = 1;
				heap->available -= size;
				

				if(prev_temp == NULL){
					heap->free_list = temp->next;

					if(heap->curr == NULL){
						heap->curr = temp;
						
					}
					else{
						heap->curr->next = temp;
						temp->next = NULL;
						heap->curr = temp;
					}
				}
				else{
					prev_temp->next = temp->next;

					if(heap->curr == NULL){
						heap->curr = temp;
						
					}
					else{
						heap->curr->next = temp;
						temp->next = NULL;
						heap->curr = temp;
					}

				}

				uint32_t address = (uint32_t)((uint8_t *)heap->curr + sizeof(heap_chunk));

				kprintf("Allocated %d bytes at address %x from freelist\n", heap->curr->size - sizeof(heap_chunk), address);
				kprintf("Available heap size: %d bytes\n", heap->available);
				
				return (void *)(address);
			}
			prev_temp = temp;
			temp = temp->next;
		}
	}

	if(((uint8_t*)heap->curr + size + sizeof(heap_chunk)) >= (uint8_t*)&_eheap){
		kprintf("Heap is full or requested size too large\n");
		return NULL;
	}

    if (heap->curr == NULL)
    {

        // heap_chunk *newChunk = (heap_chunk *)((uint8_t *)heap->start + sizeof(heap_chunk) + size);
        heap_chunk *newChunk = (heap_chunk *)((uint8_t *)heap->start);
        newChunk->size = size + sizeof(heap_chunk); 
        newChunk->isUse = 1;
        newChunk->next = NULL;

        heap->curr = newChunk;
    }
    else
    {

        heap_chunk *newChunk = (heap_chunk *)((uint8_t *)heap->curr + heap->curr->size);
        newChunk->size = size + sizeof(heap_chunk);
        newChunk->isUse = 1;
        newChunk->next = NULL;

        heap->curr->next = newChunk;
        heap->curr = newChunk;
    }

    heap->available -= size + sizeof(heap_chunk);

    uint32_t address = (uint32_t)((uint8_t *)heap->curr + sizeof(heap_chunk));

    kprintf("Allocated %d bytes at address %x\n", size, address);
    kprintf("Available heap size: %d bytes\n", heap->available);

    return (void *)(address);
}



void heap_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    heap_chunk *chunk = (heap_chunk *)((uint8_t *)ptr - sizeof(heap_chunk)); // chunk who allocated the size

    kprintf("memory to free --heap_free : %x\n", chunk);

    if (chunk->isUse)
    {
        chunk->isUse = 0;
    }

    uint8_t* start = (uint8_t*)chunk + sizeof(heap_chunk);
	uint32_t size = chunk->size - sizeof(heap_chunk);

	int i=0;

	while(i < size){
		*start = 0;
		start++;
		i++;
	}

	chunk->next = heap->free_list;
	heap->free_list = chunk;

	kprintf("Freed %d bytes at address %x\n", size, (uint32_t)ptr);
	kprintf("Available heap size: %d bytes\n", heap->available);
}



int __sys_execv(char *filename, char *argv[], char *envp[]){
	kprintf("inside execv\n");

	//doing nothing with argv and envp

	file_entry_t file = file_list[find_file(filename)];

	if(file.address == NULL){
		kprintf("file not found\n");
		return -1;
	}

	void (*func_ptr)(void) = (void (*)(void)) file.address;

	TCB_TypeDef* tcb = (TCB_TypeDef*)(READY_QUEUE + CURR_TASK_P);

	uint64_t psp_stack[1024];
    tcb->psp = psp_stack + 1024;
    *(--tcb->psp) = 0x01000000;      // xPSR
    *(--tcb->psp) = (uint32_t)func_ptr; // PC
    *(--tcb->psp) = 0xFFFFFFFD;      // LR

    for (uint32_t i = 0; i < 13; i++)
    {
        *(--tcb->psp) = 0;
    }

	__sys_start_task((uint32_t)tcb->psp);


    __ISB();

	return 0;
}