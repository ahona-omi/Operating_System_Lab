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

#include <unixtd.h>
/* Write your highlevel I/O details */

void du_scanf(char *format, ...)
{
    va_list args;
    va_start(args, format);

    asm volatile(
        "mov r0, %0\n"
        "mov r1, %1\n"
        :
        : "r"(format), "r"(&args));

    asm volatile("PUSH {r4-r11}");
    asm volatile(
        "svc %0\n"
        "POP {r4-r11}\n"
        :
        : "i"(SYS_read));

    va_end(args);
}

void du_printf(char *format, ...)
{
    uint8_t str[500];
    int index = 0;
    va_list(place_holders);

    va_start(place_holders, format);

    int i = 0;
    while (format[i] != '\0')
    {
        if (format[i] == '%')
        {
            i++;
            if (format[i] == 'd')
            {
                int num = va_arg(place_holders, int);
                if (num < 0)
                {
                    str[index] = '-';
                    index++;
                    num = -num;
                }
                uint8_t *result = convert(num, 10);

                for (result; *result != '\0'; result++)
                {
                    str[index] = *result;
                    index++;
                }
            }
            else if (format[i] == 'c')
            {
                uint8_t ch = va_arg(place_holders, int);

                str[index] = ch;
                index++;
            }
            else if (format[i] == 's')
            {
                uint8_t *s = va_arg(place_holders, uint8_t *);
                for (s; *s != '\0'; s++)
                {
                    str[index] = *s;
                    index++;
                }
            }
            else if (format[i] == 'f')
            {
                double num = va_arg(place_holders, double);

                // uint8_t* result = float_2_str(num);
                uint8_t *result = float2str(num);
                for (result; *result != '\0'; result++)
                {
                    str[index] = *result;
                    index++;
                }
            }
            else if (format[i] == 'x')
            {
                int num = va_arg(place_holders, int);
                if (num < 0)
                {
                    str[index] = '-';
                    index++;
                    num = -num;
                }

                str[index] = '0';
                index++;
                str[index] = 'x';
                index++;

                uint8_t *result = convert(num, 16);
                for (result; *result != '\0'; result++)
                {
                    str[index] = *result;
                    index++;
                }
            }
        }
        else
        {

            str[index] = (uint8_t)format[i];
            index++;
        }
        i++;
    }

    str[index] = '\0';

    va_end(place_holders);

    volatile uint8_t fd = 1;
    volatile char *buffer = (uint8_t *)str;
    volatile uint32_t size_x = index;

    // kprintf("format: %s\n", format);

    // kprintf("fd: %d\n", fd);
    // kprintf("buffer: %s\n", (char *)buffer);
    // kprintf("size: %d\n", size_x);

    asm volatile(
        "mov r0, %0\n" : : "r"(fd));
    asm volatile(
        "mov r1, %0\n" : : "r"(buffer));
    asm volatile(
        "mov r2, %0\n" : : "r"(size_x));

        __ISB();

    // int x = 0;
    // asm volatile("mov %0, r3\n"
    //              : "=r"(x));
    // kprintf("x: %d\n", x);

    __DSB();
    __ISB();

    asm volatile("PUSH {r4-r11}");
    asm volatile(
        "svc %0\n"
        "POP {r4-r11}\n"
        :
        : "i"(SYS_write));
}
