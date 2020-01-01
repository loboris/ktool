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


#include "uart.h"
#include "isp.h"
#include "slip.h"
#include "syscalls.h"
#include "sysctl.h"
#include "flash.h"
#include "fpioa.h"
#include "encoding.h"
#include "sleep.h"

// Define isp_cb as uninitialized global to reduce code size
isp_cb cb;

static uint64_t app_start = USER_MEMORY_START_ADDRESS;
static uint32_t core0_sync = 0;
static uint32_t core1_sync = 0;

// Standard CRC32 function implementation
//----------------------------------------------------------
static uint32_t block_crc32(uint8_t *message, uint32_t size)
{
    int j;
    uint32_t byte, crc, mask;

    crc = 0xFFFFFFFF;
    while (size != 0) {
        byte = *message;
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }

        message++;
        size--;
    }
    return ~crc;
}

//----------------------------------------
static void isp_send_info(const char *str)
{
    isp_response_t response = {
            .op_ret = ISPMODE_DEBUG_MSG,
            .reason = 0
    };

    SLIP_send_frame_delimiter();
    SLIP_send_frame_data_buf(&response, sizeof(isp_response_t));
    SLIP_send_frame_data_buf(str, strlen(str));
    SLIP_send_frame_delimiter();
}

//---------------------
// Process SLIP command
//-----------------------------------------------
static void process_command(isp_response_t *resp)
{
    uint32_t flash_address;
    uint8_t *data_ptr;
    uint32_t flash_chip = 1;
    uint32_t sector_addr, sector_offset;

    switch (cb.command_ptr->op) {
        case ISPMODE_FLASH_INIT:
            // === Initialize SPI flash ===
            flash_chip = cb.command_ptr->address;
            if (flash_chip != 0 && flash_chip != 1) {
                resp->reason = ISPMODE_RET_INVALID_COMMAND;
                break;
            }

            // Flash should only be initialized once, mark as done
            if (!(cb.status & ISPMODE_STATUS_FLASH_SET)) {
                if (FLASH_OK == flash_init(flash_chip)) {
                    flash_enable_quad_mode();
                    cb.status |= ISPMODE_STATUS_FLASH_SET;
                    resp->reason = ISPMODE_RET_OK;
                }
                else {
                    resp->reason = ISPMODE_RET_BAD_INITIALIZATION;
                }
            }
            else {
                // already initialized
                resp->reason = ISPMODE_RET_OK;
            }
            break;

        case ISPMODE_FLASH_ERASE:
            // === Erase the whole SPI Flash ===
            if (!(cb.status & ISPMODE_STATUS_FLASH_SET)) {
                resp->reason = ISPMODE_RET_INVALID_COMMAND;
                break;
            }
            flash_chip_erase();
            // wait until erased
            while(flash_is_busy())
                ;
            resp->reason = ISPMODE_RET_OK;
            break;

        case ISPMODE_FLASH_ERASE_BLOCK:
        case ISPMODE_FLASH_WRITE:
            // === Flash block write ===
            if (!(cb.status & ISPMODE_STATUS_FLASH_SET)) {
                resp->reason = ISPMODE_RET_INVALID_COMMAND;
                break;
            }

            // check crc32 of the received data (address + data_len + data_buf)
            if (cb.command_ptr->checksum != block_crc32((uint8_t *)cb.command_ptr + 8, cb.command_ptr->data_len + 8)) {
                resp->reason = ISPMODE_RET_BAD_DATA_CHECKSUM;
                break;
            }

            flash_address = cb.command_ptr->address;
            data_ptr = (uint8_t *) cb.command_ptr->data_buf;

            // calculate sector address and offset
            sector_offset=1;
            if ((cb.command_ptr->data_len == 65536) || (cb.command_ptr->data_len == 4096)) {
                sector_addr = flash_address & (~(cb.command_ptr->data_len - 1));
                sector_offset = flash_address & (cb.command_ptr->data_len - 1);
            }

            if (sector_offset != 0) {
                isp_send_info("Aligne error!\n");
                return;
            }

            // erase the requested sector or 64K block
            if (cb.command_ptr->data_len == 65536) flash_64k_block_erase(sector_addr);
            else flash_sector_erase(sector_addr);
            while (flash_is_busy() == FLASH_BUSY) {
                ;
            }

            if (cb.command_ptr->op == ISPMODE_FLASH_WRITE) {
                // write received data to the erased sector or 64K block
                flash_write_data(flash_address, data_ptr, cb.command_ptr->data_len);
            }

            resp->reason = ISPMODE_RET_OK;

            cb.status |= ISPMODE_STATUS_IDLE;

            break;

        case ISPMODE_FLASH_READ:
            // === Flash block read ===
            if (!(cb.status & ISPMODE_STATUS_FLASH_SET)) {
                resp->reason = ISPMODE_RET_INVALID_COMMAND;
                break;
            }
            if (cb.command_ptr->data_len != 4) {
                resp->reason = ISPMODE_RET_BAD_DATA_LEN;
                return;
            }
            resp->reason = ISPMODE_RET_IGNORE;

            cb.command_ptr->data_len = *(uint32_t *)cb.command_ptr->data_buf;
            flash_address = cb.command_ptr->address;
            data_ptr = (uint8_t *)cb.command_ptr->data_buf;

            // calculate sector address and offset
            sector_offset=1;
            if ((cb.command_ptr->data_len == 65536) || (cb.command_ptr->data_len == 4096)) {
                sector_addr = flash_address & (~(cb.command_ptr->data_len - 1));
                sector_offset = flash_address & (cb.command_ptr->data_len - 1);
            }

            if (sector_offset != 0) {
                cb.command_ptr->data_len = 0;
            }
            else {
                // === read sector data ===
                flash_read_data(flash_address, data_ptr, cb.command_ptr->data_len, FLASH_QUAD);
            }
            cb.command_ptr->checksum = block_crc32((uint8_t *)cb.command_ptr + 8, cb.command_ptr->data_len + 8);
            SLIP_send((const void *)cb.command_ptr, cb.command_ptr->data_len + 16);

            cb.status |= ISPMODE_STATUS_IDLE;

            break;

        case ISPMODE_FLASH_ID:
            // === Flash block read ===
            if (!(cb.status & ISPMODE_STATUS_FLASH_SET)) {
                resp->reason = ISPMODE_RET_INVALID_COMMAND;
                break;
            }
            resp->reason = ISPMODE_RET_IGNORE;

            //flash_read_id(&cb.command_ptr->data_buf[0], &cb.command_ptr->data_buf[1]);
            flash_read_jedec_id(&cb.command_ptr->data_buf[0]);
            flash_read_unique(&cb.command_ptr->data_buf[3]);
            cb.command_ptr->checksum = block_crc32((uint8_t *)cb.command_ptr + 8, 19);
            SLIP_send((const void *)cb.command_ptr, 27);

            cb.status |= ISPMODE_STATUS_IDLE;

            break;

        case ISPMODE_SRAM_WRITE:
            // === Write block to SRAM ===

            // check crc32 of the received data (address + data_len + data_buf)
            if (cb.command_ptr->checksum != block_crc32((uint8_t *)cb.command_ptr + 8, cb.command_ptr->data_len + 8)) {
                resp->reason = ISPMODE_RET_BAD_DATA_CHECKSUM;
                break;
            }

            data_ptr = (uint8_t *)cb.command_ptr->data_buf;
            uint8_t *sram = (uint8_t *)cb.command_ptr->address;
            if (cb.command_ptr->address < 0x80000000) sram += 0x80000000;
            for (uint32_t n = 0; n < cb.command_ptr->data_len; n++) {
                sram[n] = data_ptr[n];
            }
            resp->reason = ISPMODE_RET_OK;

            cb.status |= ISPMODE_STATUS_IDLE;

            break;

        case ISPMODE_UART_BAUDRATE_SET:
            // === Set the UART baud rate ===
            if (cb.command_ptr->data_len != 4) {
                resp->reason = ISPMODE_RET_BAD_DATA_LEN;
                return;
            }
            uart_configure(UART_NUM, *(uint32_t *)cb.command_ptr->data_buf, 8, UART_STOP_1, UART_PARITY_NONE);
            resp->reason = ISPMODE_RET_OK;
            break;

        case ISPMODE_REBOOT:
            if (!(cb.status & ISPMODE_STATUS_IDLE)) {
                // check current status
                cb.status |= ISPMODE_STATUS_IDLE;
                resp->reason = ISPMODE_RET_INVALID_COMMAND;
                break;
            }
            isp_send_info("Reboot!!!");
            // Hardware reset the SoC
            sysctl_reset(SYSCTL_RESET_SOC);

            break;

        case ISPMODE_EXECUTE:
            //isp_send_info("Execute!!!");
            resp->reason = ISPMODE_RET_OK;
            SLIP_send((const void *) resp, 2);
            usleep(500);
            // Start running the loaded application
            sysctl_reset(SYSCTL_RESET_SPI3);
            sysctl_reset(SYSCTL_RESET_UART3);
            sysctl_reset(SYSCTL_RESET_FPIOA);
            asm ("fence.i");
            asm ("fence.i");
            asm ("jr %0" : : "r"(app_start));

            break;

        case ISPMODE_NOP:
            // ping-pong
            resp->reason = ISPMODE_RET_OK;
            break;
    }
}

//============
int main(void)
{
    // Initialize bss data to 0
    extern unsigned int _bss;
    extern unsigned int _ebss;
    unsigned int *dst;
    dst = &_bss;
    while(dst < &_ebss)
        *dst++ = 0;

    fpioa_init();

    if (read_csr(mhartid) != 0) {
        core0_sync = 1;
        while (core1_sync == 0) {
            asm("nop");
            //asm volatile("wfi");
        }
        
        asm("fence");
        asm("fence.i");
        asm ("jr %0" : : "r"(app_start));
        // This should never be reached!
        while (1) {
            ;
        }
    }

    int count = 0;
    uint8_t c;
    uint8_t *sram_start = (uint8_t *)0x80000000; 

    fpioa_set_function(4, FUNC_UART1_RX + UART_NUM * 2);
    fpioa_set_function(5, FUNC_UART1_TX + UART_NUM * 2);

    // Initialize and configure UART
    uart_init(UART_NUM);
    uart_configure(UART_NUM, 115200, 8, UART_STOP_1, UART_PARITY_NONE);

    // clear SRAM
    memset(sram_start, 0, 0x5E0000);
    // clear isp_cb structure
    memset(&cb, 0, sizeof(cb));

    // ==========================
    // Wait and process UART data
    // ==========================
    while (1) {
        // wait for character on UART
        count = uart_receive_data(UART_NUM, (char *)&c, 1);
        if (count == 0) continue;

        // === process the received character ============================
        // read next token
        int16_t r = SLIP_process_byte(c, (slip_state_t *) &cb.slip_state);

        if (r >= 0) {
            // put byte into receive buffer
            cb.buffer[cb.recv_count++] = (uint8_t)r;

            if (cb.recv_count > sizeof(isp_request_t)) {
                // shouldn't happen unless there are data errors
                r = SLIP_NO_FRAME;
                cb.error = ISPMODE_RET_BAD_DATA_LEN;
            }
        }

        if (r == SLIP_FINISHED_FRAME) {
            // end of frame, set 'command' to be processed
            cb.command_ptr = (isp_request_t *)cb.buffer;
            cb.recv_count = 0;
        }
        // ===============================================================
        
        // wait for a valid SLIP command or an error
        if ((cb.command_ptr == NULL) && (cb.error == 0)) continue;

        // command received, process it
        if (cb.command_ptr != NULL) {
            // valid command, prepare response
            isp_response_t response = {
                    .op_ret = cb.command_ptr->op,
                    .reason = 0
            };
            // process the received command
            process_command(&response);
            // prepare for the next SLIP frame
            cb.command_ptr = NULL;
            // send the command response
            if (response.reason != ISPMODE_RET_IGNORE) SLIP_send((const void *)&response, 2);
        }

        if (cb.error != 0) {
            // SLIP command parse error, prepare response
            isp_response_t response = {
                    .op_ret = ISPMODE_NOP,
                    .reason = cb.error
            };
            // reset error
            cb.error = 0;
            // send the command response
            if (response.reason != ISPMODE_RET_IGNORE) SLIP_send((const void *) &response, 2);
        }
    }
}
