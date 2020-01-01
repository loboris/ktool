/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdint.h>
#include <stdlib.h>
#include "sysctl.h"
#include "uart.h"

#define __UART_BRATE_CONST 16

volatile uart_t *const uart[3] =
    {
        (volatile uart_t *)UART1_BASE_ADDR,
        (volatile uart_t *)UART2_BASE_ADDR,
        (volatile uart_t *)UART3_BASE_ADDR};

#define UART_INTERRUPT_SEND 0x02U
#define UART_INTERRUPT_RECEIVE 0x04U
#define UART_INTERRUPT_CHARACTER_TIMEOUT 0x0CU

volatile int g_write_count = 0;

static int uart_channel_putc(char c, uart_device_number_t channel)
{
    while(uart[channel]->LSR & (1u << 5))
        continue;
    uart[channel]->THR = c;
    return c & 0xff;
}

int uart_receive_data(uart_device_number_t channel, char *buffer, size_t buf_len)
{
    size_t i = 0;
    for(i = 0; i < buf_len; i++)
    {
        if(uart[channel]->LSR & 1)
            buffer[i] = (char)(uart[channel]->RBR & 0xff);
        else
            break;
    }
    return i;
}

int uart_send_data(uart_device_number_t channel, const char *buffer, size_t buf_len)
{
    g_write_count = 0;
    while(g_write_count < buf_len)
    {
        uart_channel_putc(*buffer++, channel);
        g_write_count++;
    }
    return g_write_count;
}

void uart_configure(uart_device_number_t channel, uint32_t baud_rate, uart_bitwidth_t data_width, uart_stopbit_t stopbit, uart_parity_t parity)
{
    /*configASSERT(data_width >= 5 && data_width <= 8);
    if(data_width == 5)
    {
        configASSERT(stopbit != UART_STOP_2);
    } else
    {
        configASSERT(stopbit != UART_STOP_1_5);
    }*/

    uint32_t stopbit_val = stopbit == UART_STOP_1 ? 0 : 1;
    uint32_t parity_val = 0;
    /*switch(parity)
    {
        case UART_PARITY_NONE:
            parity_val = 0;
            break;
        case UART_PARITY_ODD:
            parity_val = 1;
            break;
        case UART_PARITY_EVEN:
            parity_val = 3;
            break;
        default:
            //configASSERT(!"Invalid parity");
            break;
    }*/

    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
    uint32_t divisor = freq / baud_rate;
    uint8_t dlh = divisor >> 12;
    uint8_t dll = (divisor - (dlh << 12)) / __UART_BRATE_CONST;
    uint8_t dlf = divisor - (dlh << 12) - dll * __UART_BRATE_CONST;

    /* Set UART registers */

    uart[channel]->LCR |= 1u << 7;
    uart[channel]->DLH = dlh;
    uart[channel]->DLL = dll;
    uart[channel]->DLF = dlf;
    uart[channel]->LCR = 0;
    uart[channel]->LCR = (data_width - 5) | (stopbit_val << 2) | (parity_val << 3);
    uart[channel]->LCR &= ~(1u << 7);
    uart[channel]->IER |= 0x80; /* THRE */
    uart[channel]->FCR = UART_RECEIVE_FIFO_1 << 6 | UART_SEND_FIFO_8 << 4 | 0x1 << 3 | 0x1;
}

void __attribute__((weak, alias("uart_configure")))
uart_config(uart_device_number_t channel, uint32_t baud_rate, uart_bitwidth_t data_width, uart_stopbit_t stopbit, uart_parity_t parity);

void uart_init(uart_device_number_t channel)
{
    sysctl_clock_enable(SYSCTL_CLOCK_UART1 + channel);
    sysctl_reset(SYSCTL_RESET_UART1 + channel);
}
