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

#include <UsartRingBuffer.h>
#include <sys_usart.h>
#include <serial_lin.h>
#include <kstring.h>
#include <stm32f446xx.h>
#include <system_config.h>
#include <cm4.h>
#include <cmd_def.h>

/*  Define the device uart and pc uart below according to your setup  */

/* put the following in the ISR

extern void Uart_isr (UART_HandleTypeDef *huart);

*/

/**************** =====================================>>>>>>>>>>>> NO chnages after this **********************/

void static store_char(unsigned char c, UART_HandleTypeDef *huart);

void Ringbuf_init(UART_HandleTypeDef *huart)
{
	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
	__UART_ENABLE_IT(huart, UART_IT_ERR);
	/* Enable the UART Data Register not empty Interrupt */
	__UART_ENABLE_IT(huart, UART_IT_RXNE);
}

void static store_char(unsigned char c, UART_HandleTypeDef *huart)
{
	ring_buffer *buffer = huart->pRxBuffPtr;
	uint32_t buff_size = huart->RxXferSize;
	unsigned int i = 0;
	if ((buffer->head + 1) == buff_size)
	{
		i = 0;
	}
	else
	{
		i = buffer->head + 1;
	}
	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	if (i != buffer->tail)
	{
		buffer->buffer[buffer->head] = c;
		buffer->head = i;
	}
}

int Look_for(char *str, char *buffertolookinto)
{
	unsigned int stringlength = __strlen((uint8_t *)str);
	unsigned int bufferlength = __strlen((uint8_t *)buffertolookinto);
	unsigned int so_far = 0;
	unsigned int indx = 0;
repeat:
	while (str[so_far] != buffertolookinto[indx])
		indx++;
	if (str[so_far] == buffertolookinto[indx])
	{
		while (str[so_far] == buffertolookinto[indx])
		{
			so_far++;
			indx++;
		}
	}

	else
	{
		so_far = 0;
		if (indx >= bufferlength)
			return -1;
		goto repeat;
	}

	if (so_far == stringlength)
		return 1;
	else
		return -1;
}

void GetDataFromBuffer(char *startString, char *endString, char *buffertocopyfrom, char *buffertocopyinto)
{
	int startStringLength = (int)__strlen((uint8_t *)startString);
	int endStringLength = (int)__strlen((uint8_t *)endString);
	int so_far = 0;
	int indx = 0;
	int startposition = 0;
	int endposition = 0;

repeat1:
	while (startString[so_far] != buffertocopyfrom[indx])
		indx++;
	if (startString[so_far] == buffertocopyfrom[indx])
	{
		while (startString[so_far] == buffertocopyfrom[indx])
		{
			so_far++;
			indx++;
		}
	}

	if (so_far == startStringLength)
		startposition = indx;
	else
	{
		so_far = 0;
		goto repeat1;
	}

	so_far = 0;

repeat2:
	while (endString[so_far] != buffertocopyfrom[indx])
		indx++;
	if (endString[so_far] == buffertocopyfrom[indx])
	{
		while (endString[so_far] == buffertocopyfrom[indx])
		{
			so_far++;
			indx++;
		}
	}

	if (so_far == endStringLength)
		endposition = (indx - endStringLength);
	else
	{
		so_far = 0;
		goto repeat2;
	}

	so_far = 0;
	indx = 0;

	for (int i = startposition; i < endposition; i++)
	{
		buffertocopyinto[indx] = buffertocopyfrom[i];
		indx++;
	}
}

void Uart_flush(UART_HandleTypeDef *uart)
{
	kmemset(uart->pRxBuffPtr->buffer, '\0', uart->RxXferSize);
	uart->pRxBuffPtr->head = 0;
	uart->pRxBuffPtr->tail = 0;
}

int Uart_peek(UART_HandleTypeDef *uart)
{
	if (IS_USART_INSTANCE(uart->Instance))
	{
		if (uart->pRxBuffPtr->head == uart->pRxBuffPtr->tail)
		{
			return -1;
		}
		else
		{
			return uart->pRxBuffPtr->buffer[uart->pRxBuffPtr->tail];
		}
	}
	else
		return -1;
}

int Uart_read(UART_HandleTypeDef *uart)
{
	if (IS_USART_INSTANCE(uart->Instance))
	{
		// if the head isn't ahead of the tail, we don't have any characters
		if (uart->pRxBuffPtr->head == uart->pRxBuffPtr->tail)
		{
			return -1;
		}
		else
		{
			unsigned char c = uart->pRxBuffPtr->buffer[uart->pRxBuffPtr->tail];
			uart->pRxBuffPtr->tail = (unsigned int)(uart->pRxBuffPtr->tail + 1) % uart->RxXferSize;
			return c;
		}
	}

	else
		return -1;
}

void Uart_write(int c, UART_HandleTypeDef *uart)
{
	if (!IS_USART_INSTANCE(uart->Instance))
		return;
	unsigned int i;
	if (c >= 0)
	{
		if ((uart->pTxBuffPtr->head + 1) == uart->TxXferSize)
		{
			i = 0;
		}
		else
		{
			i = uart->pTxBuffPtr->head + 1;
		}

		// If the output buffer is full, there's nothing for it other than to
		// wait for the interrupt handler to empty it a bit
		// ???: return 0 here instead?

		// if (i == uart->pTxBuffPtr->tail)
		// {
		// 	// kprintf("Uart buffer full, dropping character: %c\n", c);
		// 	return; // Drop the character instead of hanging
		// }

		while (i == uart->pTxBuffPtr->tail)
			;

		uart->pTxBuffPtr->buffer[uart->pTxBuffPtr->head] = (uint8_t)c;
		uart->pTxBuffPtr->head = i;

		__UART_ENABLE_IT(uart, UART_IT_TXE); // Enable UART transmission interrupt
	}
}

int IsDataAvailable(UART_HandleTypeDef *uart)
{
	if (IS_USART_INSTANCE(uart->Instance))
	{
		return (uint16_t)(uart->RxXferSize + uart->pRxBuffPtr->head - uart->pRxBuffPtr->tail) % uart->RxXferSize;
	}
	else
		return -1;
}

int Get_after(char *string, uint8_t numberofchars, char *buffertosave, UART_HandleTypeDef *uart)
{

	while (Wait_for(string, uart) != 1)
		;
	for (int indx = 0; indx < numberofchars; indx++)
	{
		while (!(IsDataAvailable(uart)))
			;
		buffertosave[indx] = (char)Uart_read(uart);
	}
	return 1;
}

void Uart_sendstring(const char *s, UART_HandleTypeDef *uart)
{
	while (*s != '\0')
		Uart_write(*s++, uart);
}

void Uart_printbase(long n, uint8_t base, UART_HandleTypeDef *uart)
{
	char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
	char *s = &buf[sizeof(buf) - 1];
	char c;
	*s = '\0';

	// prevent crash if called with base == 1
	if (base < 2)
		base = 10;

	do
	{
		long m = n;
		n /= base;
		c = (char)(m - (long)(base * n));
		*--s = c < 10 ? c + '0' : c + 'A' - 10;
	} while (n);

	while (*s)
		Uart_write(*s++, uart);
}

int Copy_upto(char *string, char *buffertocopyinto, UART_HandleTypeDef *uart)
{
	int so_far = 0;
	int len = (int)__strlen((uint8_t *)string);
	int indx = 0;

again:
	while (!IsDataAvailable(uart))
		;
	while (Uart_peek(uart) != string[so_far])
	{
		buffertocopyinto[indx] = uart->pRxBuffPtr->buffer[uart->pRxBuffPtr->tail];
		uart->pRxBuffPtr->tail = (unsigned int)(uart->pRxBuffPtr->tail + 1) % uart->RxXferSize;
		indx++;
		while (!IsDataAvailable(uart))
			;
	}
	while (Uart_peek(uart) == string[so_far])
	{
		so_far++;
		buffertocopyinto[indx++] = (char)Uart_read(uart);
		if (so_far == len)
			return 1;
		while (!IsDataAvailable(uart))
			;
	}

	if (so_far != len)
	{
		so_far = 0;
		goto again;
	}

	if (so_far == len)
		return 1;
	else
		return -1;
}

int Wait_for(char *string, UART_HandleTypeDef *uart)
{
	int so_far = 0;
	int len = (int)__strlen((uint8_t *)string);
	uint32_t c_time = __getTime();
again_device:
	while (!IsDataAvailable(uart))
	{
#ifdef MS_TIMEOUT
		if ((__getTime() - c_time) >= MS_TIMEOUT * 1000)
		{
			return SYS_TIMEOUT;
		}
#endif
	}
	if (Uart_peek(uart) != string[so_far])
	{
		uart->pRxBuffPtr->tail = (unsigned int)(uart->pRxBuffPtr->tail + 1) % uart->RxXferSize;
		goto again_device;
	}
	while (Uart_peek(uart) == string[so_far])
	{
		so_far++;
		Uart_read(uart);
		if (so_far == len)
			return 1;
		while (!IsDataAvailable(uart))
		{
#ifdef MS_TIMEOUT
			if ((__getTime() - c_time) >= MS_TIMEOUT * 1000)
			{
				return SYS_TIMEOUT;
			}
#endif
		}
	}

	if (so_far != len)
	{
		so_far = 0;
		goto again_device;
	}

	if (so_far == len)
		return 1;
	else
		return 0;
}

int look_for_frame(char *string, UART_HandleTypeDef *uart, uint32_t timeout, uint8_t *target)
{
	int so_far = 0;
	uint8_t c;
	uint32_t j;
	int len = (int)__strlen((uint8_t *)string);
	uint32_t c_time = __getTime();
again_device:
	j = 0;
	while (!IsDataAvailable(uart))
	{
		if (timeout > 0)
		{
			if ((__getTime() - c_time) >= timeout)
			{
				return SYS_TIMEOUT;
			}
		}
	}
	if (Uart_peek(uart) != string[so_far])
	{
		uart->pRxBuffPtr->tail = (unsigned int)(uart->pRxBuffPtr->tail + 1) % uart->RxXferSize;
		goto again_device;
	}
	while (Uart_peek(uart) == string[so_far])
	{
		so_far++;
		c = Uart_read(uart);
		target[j] = c;
		j++;
		if (so_far == len)
		{
			while (!IsDataAvailable(uart))
				;
			while (Uart_peek(uart) != '\n')
			{
				c = Uart_read(uart);
				target[j] = c;
				j++;
				while (!IsDataAvailable(uart))
					;
			}
			target[j] = 0;
			return 1;
		}
		while (!IsDataAvailable(uart))
		{
			if (timeout > 0)
			{
				if ((__getTime() - c_time) >= timeout)
				{
					return SYS_TIMEOUT;
				}
			}
		}
	}

	if (so_far != len)
	{
		so_far = 0;
		j = 0;
		goto again_device;
	}

	if (so_far == len)
		return 1;
	else
	{
		target[0] = 0;
		return 0;
	}
}

void Uart_isr(UART_HandleTypeDef *huart)
{
	uint32_t isrflags = READ_REG(huart->Instance->SR);
	uint32_t cr1its = READ_REG(huart->Instance->CR1);
	uint32_t tmp;
	unsigned char c;
	if (((isrflags & USART_SR_NE) != RESET) || ((isrflags & USART_SR_ORE) != RESET) || ((isrflags & USART_SR_FE) != RESET))
	{
		tmp = huart->Instance->SR;
		tmp |= (tmp | huart->Instance->DR);
		return;
	}

	/* if DR is not empty and the Rx Int is enabled */
	if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	{
		/******************
					 *  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
					 *          error) and IDLE (Idle line detected) flags are cleared by software
					 *          sequence: a read operation to USART_SR register followed by a read
					 *          operation to USART_DR register.
					 * @note   RXNE flag can be also cleared by a read to the USART_DR register.
					 * @note   TC flag can be also cleared by software sequence: a read operation to
					 *          USART_SR register followed by a write operation to USART_DR register.
					 * @note   TXE flag is cleared only by a write to the USART_DR register.

		*********************/
		huart->Instance->SR; /* Read status register */

		c = (unsigned char)(huart->Instance->DR); /* Read data register */
		store_char(c, huart);					  // store data in buffer

		return;
	}

	/*If interrupt is caused due to Transmit Data Register Empty */
	if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
	{

		if (huart->pTxBuffPtr->head == huart->pTxBuffPtr->tail)
		{
			// Buffer empty, so disable interrupts
			__UART_DISABLE_IT(huart, UART_IT_TXE);
		}

		else
		{
			// There is more data in the output buffer. Send the next byte
			c = huart->pTxBuffPtr->buffer[huart->pTxBuffPtr->tail];
			if (huart->pTxBuffPtr->tail + 1 == huart->TxXferSize)
			{
				huart->pTxBuffPtr->tail = 0;
			}
			else
			{
				huart->pTxBuffPtr->tail = (huart->pTxBuffPtr->tail + 1);
			}

			/******************
			*  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
			*          error) and IDLE (Idle line detected) flags are cleared by software
			*          sequence: a read operation to USART_SR register followed by a read
			*          operation to USART_DR register.
			* @note   RXNE flag can be also cleared by a read to the USART_DR register.
			* @note   TC flag can be also cleared by software sequence: a read operation to
			*          USART_SR register followed by a write operation to USART_DR register.
			* @note   TXE flag is cleared only by a write to the USART_DR register.

			*********************/

			huart->Instance->SR;
			// if(c!=255)
			huart->Instance->DR = c;
		}

		return;
	}
}

int32_t update_tail(UART_HandleTypeDef *huart, uint32_t len)
{
	if (((huart->pRxBuffPtr->tail + len) % huart->RxXferSize) <= huart->pRxBuffPtr->head)
	{
		huart->pRxBuffPtr->tail = ((huart->pRxBuffPtr->tail + len) % huart->RxXferSize);
		return 0;
	}
	return -1;
}

void debug_buffer(UART_HandleTypeDef *huart)
{
	uint32_t i = huart->pRxBuffPtr->tail;
	uint8_t *buffer = huart->pRxBuffPtr->buffer;
	uint32_t flag = 0;
	while (((huart->RxXferSize + huart->pRxBuffPtr->head - i) % huart->RxXferSize) > 0)
	{
		flag = 1;
		Uart_write(buffer[i], __CONSOLE);
		i = ((i + 1) % huart->RxXferSize);
	}
	if (flag == 1)
	{
		Uart_write('\n', __CONSOLE);
	}
}


/******** later added ********/



void DRV_USART_INIT(USART_TypeDef* usart)
{	
	/*****Modify according to your need *****/

	RCC->APB1ENR |= (1<<17); //enable UART 2
	RCC->AHB1ENR |= (1<<0); //enable GPIOA clock
	
		
	//2. Configure UART pin for Alternate function
	GPIOA->MODER |= (2<<4); //bits [5:4] -> 1:0 -->Alternate function for pin PA2
	GPIOA->MODER |= (2<<6); //bits [7:6] -> 1:0 -->Alternate function for PA3
	
	GPIOA->OSPEEDR |= (3<<4) | (3<<6); //bits [5:4] -> 1:1 -> high speed PA2; bits [7:6] -> 1:1 -> high speed PA3 
	
	GPIOA->AFR[0] |= (7<<8);//Bytes (11:10:09:08) = 0:1:1:1 --> AF7 Alternate function for USART2 at pin PA2
	GPIOA->AFR[0] |= (7<<12); //Bytes (15:14:13:12) = 0:1:1:1 --> AF7 Alternate function for USART2 at pin PA3
	
	//3. Enable UART on USART_CR1 rgister
	USART2->CR1 = 0x00; //clear USART
	USART2->CR1 |= (1<<13);  // UE-bit enable USART
	
	//4. Program M bit in USART CR1 to define the word length
	USART2->CR1 &= ~(1U<<12); // set M bit  = 0 for 8-bit word length
	
	//5. Select the baud rate using the USART_BRR register.
	USART2->BRR |= (7<<0) | (24<<4); //115200
	
	//  6. Enable transmission TE and recieption bits in USART_CR1 register
	USART2->CR1 |= (1<<2); // enable RE for receiver 
	USART2->CR1 |= (1<<3); //enable TE for transmitter
}
/*****Modify according to your need *****/
void UART_SendChar(USART_TypeDef *usart,uint8_t c){
	usart->DR = c;
	while(!(usart->SR & (1<<7)));
}
/*****Modify according to your need *****/
void _USART_WRITE(USART_TypeDef *usart,uint8_t *s)
{
	while (*s) UART_SendChar(usart,*s++);
}
/*****Modify according to your need *****/
uint8_t _USART_READ(USART_TypeDef* usart,uint8_t *buff,uint16_t size)
{
	uint8_t n=0;
	for(uint8_t i=0;i<size;i++){
		buff[i]=UART_GetChar(usart);
		n=i;
	}
	buff[n+1] = '\0';
	return n;
}

/*****Modify according to your need *****/

uint8_t UART_GetChar(USART_TypeDef *usart){
	uint8_t tmp;
	while(!(usart->SR & (1<<5)));
	tmp=(uint8_t)usart->DR;
	return tmp;
}

uint8_t _USART_READ_STR(USART_TypeDef* usart,uint8_t *buff,uint16_t size)
{
	uint8_t n=0;
	for(uint8_t i=0;i<size;i++){
		buff[i]=UART_GetChar(usart);
		n=i;
		if(buff[i]=='\0' || buff[i] == '\n' || buff[i] == ' ')
		{ 	
			buff[i]='\0';
			break;
		}
	}
	return n;
}







/****************************/