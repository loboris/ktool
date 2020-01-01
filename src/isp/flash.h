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

/**
 * @file
 * @brief      SPI flash driver
 */
#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>

/**
 * @brief      flash operating status enumerate
 */
enum flash_status_t {
    FLASH_OK = 0,
    FLASH_BUSY,
};

/**
 * @brief      flash read operating enumerate
 */
enum flash_read_t {
    FLASH_STANDARD = 0,
    FLASH_STANDARD_FAST,
    FLASH_DUAL,
    FLASH_DUAL_SINGLE,
    FLASH_QUAD,
    FLASH_QUAD_SINGLE,
};

enum flash_status_t flash_init(uint8_t index);
enum flash_status_t flash_is_busy(void);
enum flash_status_t flash_chip_erase(void);
enum flash_status_t flash_enable_quad_mode(void);
enum flash_status_t flash_disable_quad_mode(void);
enum flash_status_t flash_sector_erase(uint32_t addr);
enum flash_status_t flash_32k_block_erase(uint32_t addr);
enum flash_status_t flash_64k_block_erase(uint32_t addr);
enum flash_status_t flash_read_id(uint8_t *manuf_id, uint8_t *device_id);
enum flash_status_t flash_read_jedec_id(uint8_t *jedec_id);
enum flash_status_t flash_read_unique(uint8_t *unique_id);
enum flash_status_t flash_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
enum flash_status_t flash_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, enum flash_read_t mode);

#endif
