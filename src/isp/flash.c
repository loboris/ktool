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

#include "sysctl.h"
#include "flash.h"
#include "spi.h"
#include "fpioa.h"

#define SPI_SLAVE_SELECT                    (0x01)

#define flash_FLASH_PAGE_SIZE               256
#define flash_FLASH_SECTOR_SIZE             4096
#define flash_FLASH_PAGE_NUM_PER_SECTOR	    16
#define flash_FLASH_CHIP_SIZE               (16777216 UL)

#define WRITE_ENABLE                        0x06
#define WRITE_DISABLE                       0x04
#define READ_REG1                           0x05
#define READ_REG2                           0x35
#define WRITE_REG                           0x01
#define READ_DATA                           0x03
#define FAST_READ                           0x0B
#define FAST_READ_DUAL_OUTPUT               0x3B
#define FAST_READ_QUAL_OUTPUT               0x6B
#define FAST_READ_DUAL_IO                   0xBB
#define FAST_READ_QUAL_IO                   0xEB
#define DUAL_READ_RESET                     0xFFFF
#define QUAL_READ_RESET                     0xFF
#define PAGE_PROGRAM                        0x02
#define QUAD_PAGE_PROGRAM                   0x32
#define SECTOR_ERASE                        0x20
#define BLOCK_32K_ERASE                     0x52
#define BLOCK_64K_ERASE                     0xD8
#define CHIP_ERASE                          0x60
#define READ_ID                             0x90
#define READ_JEDEC_ID                       0x9F
#define READ_UNIQUE                         0x4B

#define REG1_BUSY_MASK                      0x01
#define REG2_QUAL_MASK                      0x02
#define CONTINUE_READ_MASK                  0x20


enum flash_status_t (*flash_page_program_fun)(uint32_t addr, uint8_t *data_buf, uint32_t length);
enum flash_status_t (*flash_read_fun)(uint32_t addr, uint8_t *data_buf, uint32_t length);

static enum flash_status_t flash_stand_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
static enum flash_status_t flash_quad_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
static enum flash_status_t flash_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length);
static enum flash_status_t flash_quad_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length);

static volatile spi_t *spi_handle = (volatile spi_t *)SPI3_BASE_ADDR;
static uint8_t dfs_offset, tmod_offset, frf_offset;

//------------------------------------------------------------------------------------------------------------------
static enum flash_status_t flash_receive_data(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
    uint32_t index, fifo_len;

    spi_handle->ctrlr0 = (0x07 << dfs_offset) | (0x03 << tmod_offset);
    spi_handle->ctrlr1 = rx_len - 1;
    spi_handle->ssienr = 0x01;
    while (cmd_len--) {
        spi_handle->dr[0] = *cmd_buff++;
    }
    spi_handle->ser = SPI_SLAVE_SELECT;
    while (rx_len) {
        fifo_len = spi_handle->rxflr;
        fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
        for (index = 0; index < fifo_len; index++) {
            *rx_buff++ = spi_handle->dr[0];
        }
        rx_len -= fifo_len;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
    return FLASH_OK;
}

//---------------------------------------------------------------------------------------------------------------
static enum flash_status_t flash_send_data(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
    uint32_t index, fifo_len;

    spi_handle->ctrlr0 = (0x07 << dfs_offset) | (0x01 << tmod_offset);
    spi_handle->ssienr = 0x01;
    while (cmd_len--) {
        spi_handle->dr[0] = *cmd_buff++;
    }
    fifo_len = 32 - spi_handle->txflr;
    fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
    for (index = 0; index < fifo_len; index++) {
        spi_handle->dr[0] = *tx_buff++;
    }
    tx_len -= fifo_len;
    spi_handle->ser = SPI_SLAVE_SELECT;
    while (tx_len) {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
        for (index = 0; index < fifo_len; index++) {
            spi_handle->dr[0] = *tx_buff++;
        }
        tx_len -= fifo_len;
    }
    while ((spi_handle->sr & 0x05) != 0x04) {
        ;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
    return FLASH_OK;
}

//----------------------------------------------------------------------------------------------------------------------------
static enum flash_status_t flash_receive_data_enhanced(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
    uint32_t index, fifo_len;

    spi_handle->ctrlr1 = rx_len - 1;
    spi_handle->ssienr = 0x01;
    while (cmd_len--) {
        spi_handle->dr[0] = *cmd_buff++;
    }
    spi_handle->ser = SPI_SLAVE_SELECT;
    while (rx_len) {
        fifo_len = spi_handle->rxflr;
        fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
        for (index = 0; index < fifo_len; index++) {
            *rx_buff++ = spi_handle->dr[0];
        }
        rx_len -= fifo_len;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
    return FLASH_OK;
}

//-------------------------------------------------------------------------------------------------------------------------
static enum flash_status_t flash_send_data_enhanced(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
    uint32_t index, fifo_len;

    spi_handle->ssienr = 0x01;
    while (cmd_len--) {
        spi_handle->dr[0] = *cmd_buff++;
    }
    fifo_len = 32 - spi_handle->txflr;
    fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
    for (index = 0; index < fifo_len; index++) {
        spi_handle->dr[0] = *tx_buff++;
    }
    tx_len -= fifo_len;
    spi_handle->ser = SPI_SLAVE_SELECT;
    while (tx_len) {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
        for (index = 0; index < fifo_len; index++) {
            spi_handle->dr[0] = *tx_buff++;
        }
        tx_len -= fifo_len;
    }
    while ((spi_handle->sr & 0x05) != 0x04) {
        ;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
    return FLASH_OK;
}

//-------------------------------------------
enum flash_status_t flash_init(uint8_t index)
{
    uint8_t cmd[2] = {0xFF, 0xFF};

    sysctl_reset(SYSCTL_RESET_SPI3);
    sysctl_clock_enable(SYSCTL_CLOCK_SPI3);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_SPI3, 0);
    dfs_offset = 0;
    tmod_offset = 10;
    frf_offset = 22;

    spi_handle->baudr = 0x02;
    spi_handle->imr = 0x00;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;

    flash_page_program_fun = flash_page_program;
    flash_read_fun = flash_stand_read_data;
    flash_send_data(cmd, 2, 0, 0);
    return FLASH_OK;
}

//----------------------------------------------------------------------
enum flash_status_t flash_read_id(uint8_t *manuf_id, uint8_t *device_id)
{
    uint8_t cmd[4] = {READ_ID, 0x00, 0x00, 0x00};
    uint8_t data[2];

    flash_receive_data(cmd, 4, data, 2);
    *manuf_id = data[0];
    *device_id = data[1];
    return FLASH_OK;
}

//--------------------------------------------------------
enum flash_status_t flash_read_jedec_id(uint8_t *jedec_id)
{
    uint8_t cmd[1] = {READ_JEDEC_ID};

    flash_receive_data(cmd, 1, jedec_id, 3);
    return FLASH_OK;
}

//--------------------------------------------------------
enum flash_status_t flash_read_unique(uint8_t *unique_id)
{
    uint8_t cmd[5] = {READ_UNIQUE, 0x00, 0x00, 0x00, 0x00};

    flash_receive_data(cmd, 5, unique_id, 8);
    return FLASH_OK;
}

//-------------------------------------------------
static enum flash_status_t flash_write_enable(void)
{
    uint8_t cmd[1] = {WRITE_ENABLE};

    flash_send_data(cmd, 1, 0, 0);
    return FLASH_OK;
}

//-------------------------------------------------------------------------------------
static enum flash_status_t flash_write_status_reg(uint8_t reg1_data, uint8_t reg2_data)
{
    uint8_t cmd[3] = {WRITE_REG, reg1_data, reg2_data};

    flash_write_enable();
    flash_send_data(cmd, 3, 0, 0);
    return FLASH_OK;
}

//-----------------------------------------------------------
static enum flash_status_t flash_read_status_reg1(uint8_t *reg_data)
{
    uint8_t cmd[1] = {READ_REG1};
    uint8_t data[1];

    flash_receive_data(cmd, 1, data, 1);
    *reg_data = data[0];
    return FLASH_OK;
}

//------------------------------------------------------------------
static enum flash_status_t flash_read_status_reg2(uint8_t *reg_data)
{
    uint8_t cmd[1] = {READ_REG2};
    uint8_t data[1];

    flash_receive_data(cmd, 1, data, 1);
    *reg_data = data[0];
    return FLASH_OK;
}

//-------------------------------------
enum flash_status_t flash_is_busy(void)
{
    uint8_t status;

    flash_read_status_reg1(&status);
    if (status & REG1_BUSY_MASK)
        return FLASH_BUSY;
    return FLASH_OK;
}

//---------------------------------------------------
enum flash_status_t flash_sector_erase(uint32_t addr)
{
    uint8_t cmd[4] = {SECTOR_ERASE};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    flash_write_enable();
    flash_send_data(cmd, 4, 0, 0);
    return FLASH_OK;
}

//------------------------------------------------------
enum flash_status_t flash_32k_block_erase(uint32_t addr)
{
    uint8_t cmd[4] = {BLOCK_32K_ERASE};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    flash_write_enable();
    flash_send_data(cmd, 4, 0, 0);
    return FLASH_OK;
}

//------------------------------------------------------
enum flash_status_t flash_64k_block_erase(uint32_t addr)
{
    uint8_t cmd[4] = {BLOCK_64K_ERASE};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    flash_write_enable();
    flash_send_data(cmd, 4, 0, 0);
    return FLASH_OK;
}

//----------------------------------------
enum flash_status_t flash_chip_erase(void)
{
    uint8_t cmd[1] = {CHIP_ERASE};

    flash_write_enable();
    flash_send_data(cmd, 1, 0, 0);
    return FLASH_OK;
}

//----------------------------------------------
enum flash_status_t flash_enable_quad_mode(void)
{
    uint8_t reg1_data, reg2_data;

    flash_read_status_reg2(&reg2_data);
    if (!(reg2_data & REG2_QUAL_MASK)) {
        reg2_data |= REG2_QUAL_MASK;
        flash_read_status_reg1(&reg1_data);
        flash_write_status_reg(reg1_data, reg2_data);
    }
    flash_page_program_fun = flash_quad_page_program;
    flash_read_fun = flash_quad_read_data;
    return FLASH_OK;
}

//-----------------------------------------------
enum flash_status_t flash_disable_quad_mode(void)
{
    uint8_t reg1_data, reg2_data;

    flash_read_status_reg2(&reg2_data);
    if (reg2_data & REG2_QUAL_MASK) {
        reg2_data &= (~REG2_QUAL_MASK);
        flash_read_status_reg1(&reg1_data);
        flash_write_status_reg(reg1_data, reg2_data);
    }
    flash_page_program_fun = flash_page_program;
    flash_read_fun = flash_stand_read_data;
    return FLASH_OK;
}

//----------------------------------------------------------------------------------------------
static enum flash_status_t flash_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint8_t cmd[4] = {PAGE_PROGRAM};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    flash_write_enable();
    flash_send_data(cmd, 4, data_buf, length);
    while (flash_is_busy() == FLASH_BUSY) {
        ;
    }
    return FLASH_OK;
}

//---------------------------------------------------------------------------------------------------
static enum flash_status_t flash_quad_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t cmd[2];

    cmd[0] = QUAD_PAGE_PROGRAM;
    cmd[1] = addr;
    flash_write_enable();
    spi_handle->ctrlr0 = (0x01 << tmod_offset) | (0x07 << dfs_offset) | (0x02 << frf_offset);
    spi_handle->spi_ctrlr0 = (0x06 << 2) | (0x02 << 8);
    flash_send_data_enhanced(cmd, 2, data_buf, length);
    while (flash_is_busy() == FLASH_BUSY)
        ;
    return FLASH_OK;
}

//-------------------------------------------------------------------------------------
enum flash_status_t flash_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t page_remain, write_len, len;

    while (length) {
        page_remain = flash_FLASH_PAGE_SIZE - (addr & (flash_FLASH_PAGE_SIZE - 1));
        write_len = length < page_remain ? length : page_remain;
        length -= write_len;
        while (write_len) {
            len = write_len > 32 ? 32 : write_len;
            flash_page_program_fun(addr, data_buf, len);
            addr += len;
            data_buf += len;
            write_len -= len;
        }
    }
    return FLASH_OK;
}

//--------------------------------------------------------------------------------------------------------------
static enum flash_status_t flash_read(uint32_t addr, uint8_t *data_buf, uint32_t length, enum flash_read_t mode)
{
    uint32_t cmd[2];

    switch (mode) {
        /*
        case FLASH_STANDARD:
            *(((uint8_t *)cmd) + 0) = READ_DATA;
            *(((uint8_t *)cmd) + 1) = (uint8_t)(addr >> 16);
            *(((uint8_t *)cmd) + 2) = (uint8_t)(addr >> 8);
            *(((uint8_t *)cmd) + 3) = (uint8_t)(addr >> 0);
            flash_receive_data((uint8_t *)cmd, 4, data_buf, length);
            break;
        case FLASH_STANDARD_FAST:
            *(((uint8_t *)cmd) + 0) = FAST_READ;
            *(((uint8_t *)cmd) + 1) = (uint8_t)(addr >> 16);
            *(((uint8_t *)cmd) + 2) = (uint8_t)(addr >> 8);
            *(((uint8_t *)cmd) + 3) = (uint8_t)(addr >> 0);
            *(((uint8_t *)cmd) + 4) = 0xFF;
            flash_receive_data((uint8_t *)cmd, 5, data_buf, length);
            break;
        case FLASH_DUAL:
            cmd[0] = FAST_READ_DUAL_OUTPUT;
            cmd[1] = addr;
            spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x01 << frf_offset);
            spi_handle->spi_ctrlr0 = (0x06 << 2) | (0x02 << 8) | (0x08 << 11);
            flash_receive_data_enhanced(cmd, 2, data_buf, length);
            break;
        case FLASH_DUAL_SINGLE:
            cmd[0] = FAST_READ_DUAL_IO;
            cmd[1] = addr << 8;
            spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x01 << frf_offset);
            spi_handle->spi_ctrlr0 = (0x08 << 2) | (0x02 << 8) | 0x01;
            flash_receive_data_enhanced(cmd, 2, data_buf, length);
            break;
        */
        case FLASH_QUAD:
            cmd[0] = FAST_READ_QUAL_OUTPUT;
            cmd[1] = addr;
            spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x02 << frf_offset);
            spi_handle->spi_ctrlr0 = (0x06 << 2) | (0x02 << 8) | (0x08 << 11);
            flash_receive_data_enhanced(cmd, 2, data_buf, length);
            break;
        case FLASH_QUAD_SINGLE:
            cmd[0] = FAST_READ_QUAL_IO;
            cmd[1] = addr << 8;
            spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x02 << frf_offset);
            spi_handle->spi_ctrlr0 = (0x08 << 2) | (0x02 << 8) | (0x04 << 11) | 0x01;
            flash_receive_data_enhanced(cmd, 2, data_buf, length);
            break;
        default:
            break;
    }
    return FLASH_OK;
}

//------------------------------------------------------------------------------------------------------------
enum flash_status_t flash_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, enum flash_read_t mode)
{
    uint32_t write_len;

    while (length) {
        write_len = length > 32 ? 32 : length;
        flash_read(addr, data_buf, write_len, mode);
        addr += write_len;
        data_buf += write_len;
        length -= write_len;
    }
    return FLASH_OK;
}

//-------------------------------------------------------------------------------------------------
static enum flash_status_t flash_stand_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    return flash_read_data(addr, data_buf, length, FLASH_STANDARD_FAST);
}

//------------------------------------------------------------------------------------------------
static enum flash_status_t flash_quad_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    return flash_read_data(addr, data_buf, length, FLASH_QUAD_SINGLE);
}
