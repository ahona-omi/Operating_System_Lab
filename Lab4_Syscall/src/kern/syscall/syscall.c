

#include <sys_call.h>


void syscall(uint32_t *svc_args)
{
	/* The SVC_Handler calls this function to evaluate and execute the actual function */
	/* Take care of return value or code */

	uint32_t callno = ((uint8_t *)svc_args[6])[-2];
	// kprintf("callno : %d is being called\n", callno);

	switch (callno)
	{
	/* Write your code to call actual function (kunistd.h/c or times.h/c and handle the return value(s) */
	case SYS_read:
		char *format = (char *)svc_args[0];
		va_list *args_ptr = (va_list*)svc_args[1];  // Cast to va_list pointer
		va_list args;
		va_copy(args, *args_ptr);  // Copy contents properly
		kscanf(format, args);
		va_end(args);
		break;
	case SYS_write:
		
		uint8_t fd_w = svc_args[0];
		char* buffer_w = (char *)svc_args[1];
		uint32_t size_w = svc_args[3];

		__ISB();
		// kprintf("fd: %d\n", fd_w);
		// kprintf("buffer: %s\n", (char *)buffer_w);
		// kprintf("size: %d\n", size_w);

		Uart_sendstring(buffer_w,&huart2);
		// _USART_WRITE(USART2, (uint8_t*)buffer);
		
		break;
	case SYS_reboot:
		__NVIC_SystemReset();
		break;
	case SYS_start:
		uint32_t psp = (uint32_t)svc_args[0];
		__sys_start_task(psp);
		break;
	case SYS__exit:
		TCB_TypeDef *tcb_to_kill = (TCB_TypeDef *)svc_args[2];
		tcb_to_kill->status = KILLED;
		break;
	case SYS_getpid:
		int pid = READY_QUEUE[CURR_TASK_P].task_id;
		svc_args[0] = pid;
		break;
	case SYS___time:
		uint32_t time = __getTime();
		svc_args[0] = time;
		kprintf("Time in syscall: %d\n", time);
		break;
	case SYS_yield:
		// SCB->ICSR |= (1 << 27); // Clear all pending interrupts
		SCB->ICSR |= (1 << 28); // set PendSV bit
		break;
	case SYS_set_pending:
		uint8_t value = (uint8_t)svc_args[0];
		__set_pending(value);
		break;
	case SYS_fork:

		int pid = __sys_fork(svc_args);
		svc_args[2] = pid;
		break;
	case SYS_malloc:
		uint32_t size_m = svc_args[2];
		void *ptr = heap_malloc(size_m);
		kprintf("Malloc ptr: %x\n", ptr);
		svc_args[2] = (uint32_t)ptr;
		break;
	case SYS_free:
		void *ptr1 = (void *)svc_args[2];

		kprintf("memory to free --syscall : %x\n", ptr1);
		heap_free(ptr1);

		*(&ptr1) = NULL;
		break;
	case SYS_execv:
		char *filename = (char *)svc_args[0];
		char **argv = (char **)svc_args[1];
		char **envp = (char **)svc_args[2];
		int val = __sys_execv(filename, argv, envp);
		svc_args[0] = val;
		break;

	/* return error code see error.h and errmsg.h ENOSYS sys_errlist[ENOSYS]*/
	default:
		kprintf("Error: Invalid syscall number %d\n", callno);
		break;
	}
	/* Handle SVC return here */
}
