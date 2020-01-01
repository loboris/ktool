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

#ifndef _FLAHSER_FLASHER_H
#define _FLAHSER_FLASHER_H

#include <stdint.h>
#include "slip.h"
#include "uart.h"

//#define FLASHER_DATAFRAME_SIZE 16

#define USER_MEMORY_START_ADDRESS (0x80000000UL)
#define ISP_DATABUF_SIZE          65536
#define UARTHS_BUFFER_SIZE        (ISP_DATABUF_SIZE + 256)
#define UART_NUM                  (UART_DEVICE_3)

typedef enum {
    ISPMODE_DEBUG_MSG         = 0xD1, // debug
    ISPMODE_NOP               = 0xD2, // no operation
    ISPMODE_FLASH_ERASE       = 0xD3, // erase the whole SPI Flash
    ISPMODE_FLASH_WRITE       = 0xD4, // write data to SPI Flash at specified address
    ISPMODE_REBOOT            = 0xD5, // reset the K210 to load and start the flashed application
    ISPMODE_UART_BAUDRATE_SET = 0xD6, // change uarths baudrate
    ISPMODE_FLASH_INIT        = 0xD7, // initialize the SPI Flash
    ISPMODE_SRAM_WRITE        = 0xD8, // write data to SRAM at specified address
    ISPMODE_EXECUTE           = 0xD9, // start the application at specified address
    ISPMODE_FLASH_READ        = 0xDA, // read data from SPI Flash at specified address
    ISPMODE_FLASH_ID          = 0xDE, // get SPI Flash ID
    ISPMODE_FLASH_ERASE_BLOCK = 0xDF, // erase flash sector or 64K block
} ISPMODE_COMMAND;


typedef enum {
    ISPMODE_RET_OK                 = 0xE0,    // command finished
    ISPMODE_RET_BAD_DATA_LEN       = 0xE1,    // unsupported or wrong data length
    ISPMODE_RET_BAD_DATA_CHECKSUM  = 0xE2,    // wrong data crc32 checksum
    ISPMODE_RET_INVALID_COMMAND    = 0xE3,    // invalid command
    ISPMODE_RET_BAD_INITIALIZATION = 0xE4,    // initialization failed
    ISPMODE_RET_IGNORE             = 0xEE,    // do nor send response
} ISPMODE_ERROR_CODE;

typedef enum {
    ISPMODE_STATUS_IDLE,
    ISPMODE_STATUS_FLASH_SET = 1 << 1,
    ISPMODE_STATUS_FLASH_WRITING = 1 << 2,
    ISPMODE_STATUS_SRAM_WRITING = 1 << 3,
} ISPMODE_STATUS;

typedef struct __attribute__((packed)) {
    uint16_t op;
    uint16_t reserved;
    uint32_t checksum;
    // All the following fields must participate in checksum calculation
    uint32_t address;
    uint32_t data_len;
    uint8_t data_buf[ISP_DATABUF_SIZE]; // data buffer for SLIP frame data (max 64 KB)
} isp_request_t;

typedef struct __attribute__((packed)) {
    uint8_t op_ret;
    uint8_t reason;
} isp_response_t;

typedef struct __attribute__((aligned(4))) {
    uint32_t status;
    uint8_t buffer[UARTHS_BUFFER_SIZE];
    isp_request_t *command_ptr;
    uint32_t recv_count;    // counts received characters
    uint32_t slip_state;    // current SLIP state (slip_state_t)
    uint32_t error;         // error code
} isp_cb;

void isp_main(void);

#endif
