/* Copyright 2020 LoBo
 * 
 * K210 2nd stage ISP
 * ver. 1.0.1, 01/2020
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

/*
 * Copyright (c) 2016 Cesanta Software Limited & Espressif Systems (Shanghai) PTE LTD
 * All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "uart.h"
#include "isp.h"
#include "slip.h"
#include "syscalls.h"
#include <stdio.h>

//-------------------------
void uart_send_char(char c)
{
    uart_send_data(UART_NUM, (char *)&c, 1);
}

//----------------------------------
void SLIP_send_frame_delimiter(void)
{
    uart_send_char((char)SLIP_SPECIAL_BYTE_END);
}

//--------------------------------
void SLIP_send_frame_data(char ch)
{
    if (ch == SLIP_SPECIAL_BYTE_END) {
        uart_send_char((char)SLIP_SPECIAL_BYTE_ESC);
        uart_send_char((char)SLIP_ESCAPED_BYTE_END);
    }
    else if (ch == SLIP_SPECIAL_BYTE_ESC) {
        uart_send_char((char)SLIP_SPECIAL_BYTE_ESC);
        uart_send_char((char)SLIP_ESCAPED_BYTE_ESC);
    }
    else {
        uart_send_char(ch);
    }
}

//-----------------------------------------------------------
void SLIP_send_frame_data_buf(const void *buf, uint32_t size)
{
    const uint8_t *buf_c = (const uint8_t *) buf;

    for (int i = 0; i < size; i++)
        SLIP_send_frame_data(buf_c[i]);
}

//--------------------------------------------
void SLIP_send(const void *pkt, uint32_t size)
{
    SLIP_send_frame_delimiter();
    SLIP_send_frame_data_buf(pkt, size);
    SLIP_send_frame_delimiter();
}

// Proces the received byte from data stream
//-------------------------------------------------------
int16_t SLIP_process_byte(char byte, slip_state_t *state)
{
    if (byte == SLIP_SPECIAL_BYTE_END) {
        if (*state == SLIP_NO_FRAME) {
            *state = SLIP_FRAME;
            return SLIP_NO_BYTE;
        }
        else {
            *state = SLIP_NO_FRAME;
            return SLIP_FINISHED_FRAME;
        }
    }

    switch (*state) {
        case SLIP_NO_FRAME:
            return SLIP_NO_BYTE;
        case SLIP_FRAME:
            if (byte == SLIP_SPECIAL_BYTE_ESC) {
                *state = SLIP_FRAME_ESCAPING;
                return SLIP_NO_BYTE;
            }
            return byte;
        case SLIP_FRAME_ESCAPING:
            if (byte == SLIP_ESCAPED_BYTE_END) {
                *state = SLIP_FRAME;
                return SLIP_SPECIAL_BYTE_END;
            }
            if (byte == SLIP_ESCAPED_BYTE_ESC) {
                *state = SLIP_FRAME;
                return SLIP_SPECIAL_BYTE_ESC;
            }
            // framing error
            return SLIP_NO_BYTE;
    }
    // framing error
    return SLIP_NO_BYTE;
}
