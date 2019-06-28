/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdmmc.h"
#include "mmc.h"
#include "sd.h"
#include "../utils/types.h"
#include "../utils/util.h"
#include "../utils/fatal.h"
#include "../emuMMC/emummc.h"

#define DPRINTF(...) //fprintf(stdout, __VA_ARGS__)

sdmmc_accessor_t *_current_accessor = NULL;
bool sdmmc_memcpy_buf = false;
extern bool custom_driver;

static inline u32 unstuff_bits(u32 *resp, u32 start, u32 size)
{
	const u32 mask = (size < 32 ? 1 << size : 0) - 1;
	const u32 off = 3 - ((start) / 32);
	const u32 shft = (start) & 31;
	u32 res = resp[off] >> shft;
	if (size + shft > 32)
		res |= resp[off - 1] << ((32 - shft) % 32);
	return res & mask;
}

/*
* Common functions for SD and MMC.
*/

// FS DMA calculations.
intptr_t sdmmc_calculate_dma_addr(sdmmc_accessor_t *_this, void *buf, unsigned int num_sectors)
{
    int dma_buf_idx = 0;
    char *_buf = (char *)buf;
    char *actual_buf_start = _buf;
    char *actual_buf_end = &_buf[512 * num_sectors];
    char *dma_buffer_start = _this->parent->dmaBuffers[0].device_addr_buffer;

    if (dma_buffer_start <= _buf && actual_buf_end <= &dma_buffer_start[_this->parent->dmaBuffers[0].device_addr_buffer_size])
    {
        dma_buf_idx = 0;
    }
    else
    {
        dma_buffer_start = _this->parent->dmaBuffers[1].device_addr_buffer;
        if (dma_buffer_start <= actual_buf_start && actual_buf_end <= &dma_buffer_start[_this->parent->dmaBuffers[1].device_addr_buffer_size])
        {
            dma_buf_idx = 1;
        }
        else
        {
            dma_buffer_start = _this->parent->dmaBuffers[2].device_addr_buffer;
            if (dma_buffer_start <= actual_buf_start && actual_buf_end <= &dma_buffer_start[_this->parent->dmaBuffers[2].device_addr_buffer_size])
            {
                dma_buf_idx = 2;
            }
            else
            {
                // If buffer is on a random heap
                return 0;
            }
        }
    }

	sdmmc_memcpy_buf = false;

    intptr_t admaaddr = (intptr_t)&_this->parent->dmaBuffers[dma_buf_idx].device_addr_buffer_masked[actual_buf_start - dma_buffer_start];
    return admaaddr;
}

int sdmmc_calculate_dma_index(sdmmc_accessor_t *_this, void *buf, unsigned int num_sectors)
{
    int dma_buf_idx = 0;
    char *_buf = (char *)buf;
    char *actual_buf_start = _buf;
    char *actual_buf_end = &_buf[512 * num_sectors];
    char *dma_buffer_start = _this->parent->dmaBuffers[0].device_addr_buffer;

    if (dma_buffer_start <= _buf && actual_buf_end <= &dma_buffer_start[_this->parent->dmaBuffers[0].device_addr_buffer_size])
    {
        dma_buf_idx = 0;
    }
    else
    {
        dma_buffer_start = _this->parent->dmaBuffers[1].device_addr_buffer;
        if (dma_buffer_start <= actual_buf_start && actual_buf_end <= &dma_buffer_start[_this->parent->dmaBuffers[1].device_addr_buffer_size])
        {
            dma_buf_idx = 1;
        }
        else
        {
            dma_buffer_start = _this->parent->dmaBuffers[2].device_addr_buffer;
            if (dma_buffer_start <= actual_buf_start && actual_buf_end <= &dma_buffer_start[_this->parent->dmaBuffers[2].device_addr_buffer_size])
            {
                dma_buf_idx = 2;
            }
            else
            {
                // If buffer is on a random heap
                return -1;
            }
        }
    }

	sdmmc_memcpy_buf = false;

    return dma_buf_idx;
}

int sdmmc_calculate_fitting_dma_index(sdmmc_accessor_t *_this, unsigned int num_sectors)
{
    int dma_buf_idx = 0;
	int blkSize = num_sectors * 512;

    if (_this->parent->dmaBuffers[0].device_addr_buffer_size >= blkSize)
    {
        dma_buf_idx = 0;
    }
    else
    {
        if (_this->parent->dmaBuffers[1].device_addr_buffer_size >= blkSize)
        {
            dma_buf_idx = 1;
        }
        else
        {
            if (_this->parent->dmaBuffers[2].device_addr_buffer_size >= blkSize)
            {
                dma_buf_idx = 2;
            }
            else
            {
                // Can't find a fitting buffer
                return 0;
            }
        }
    }
	
	sdmmc_memcpy_buf = true;
    return dma_buf_idx;
}

static int _sdmmc_storage_check_result(u32 res)
{
	//Error mask:
	//R1_OUT_OF_RANGE, R1_ADDRESS_ERROR, R1_BLOCK_LEN_ERROR,
	//R1_ERASE_SEQ_ERROR, R1_ERASE_PARAM, R1_WP_VIOLATION,
	//R1_LOCK_UNLOCK_FAILED, R1_COM_CRC_ERROR, R1_ILLEGAL_COMMAND,
	//R1_CARD_ECC_FAILED, R1_CC_ERROR, R1_ERROR, R1_CID_CSD_OVERWRITE,
	//R1_WP_ERASE_SKIP, R1_ERASE_RESET, R1_SWITCH_ERROR
	if (!(res & 0xFDF9A080))
		return 1;
	//TODO: R1_SWITCH_ERROR we can skip for certain card types.
	return 0;
}

static int _sdmmc_storage_execute_cmd_type1_ex(sdmmc_storage_t *storage, u32 *resp, u32 cmd, u32 arg, u32 check_busy, u32 expected_state, u32 mask)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, cmd, arg, SDMMC_RSP_TYPE_1, check_busy);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
		return 0;

	sdmmc_get_rsp(storage->sdmmc, resp, 4, SDMMC_RSP_TYPE_1);
	if (mask)
		*resp &= ~mask;

	if (_sdmmc_storage_check_result(*resp))
		if (expected_state == 0x10 || R1_CURRENT_STATE(*resp) == expected_state)
			return 1;
	return 0;
}

static int _sdmmc_storage_execute_cmd_type1(sdmmc_storage_t *storage, u32 cmd, u32 arg, u32 check_busy, u32 expected_state)
{
	u32 tmp;
	return _sdmmc_storage_execute_cmd_type1_ex(storage, &tmp, cmd, arg, check_busy, expected_state, 0);
}

static int _sdmmc_storage_go_idle_state(sdmmc_storage_t *storage)
{
	sdmmc_cmd_t cmd;
	sdmmc_init_cmd(&cmd, MMC_GO_IDLE_STATE, 0, SDMMC_RSP_TYPE_0, 0);
	return sdmmc_execute_cmd(storage->sdmmc, &cmd, 0, 0);
}

static int _sdmmc_storage_get_cid(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmd;
	sdmmc_init_cmd(&cmd, MMC_ALL_SEND_CID, 0, SDMMC_RSP_TYPE_2, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmd, 0, 0))
		return 0;
	sdmmc_get_rsp(storage->sdmmc, buf, 0x10, SDMMC_RSP_TYPE_2);
	return 1;
}

static int _sdmmc_storage_select_card(sdmmc_storage_t *storage)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SELECT_CARD, storage->rca << 16, 1, 0x10);
}

static int _sdmmc_storage_get_csd(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, MMC_SEND_CSD, storage->rca << 16, SDMMC_RSP_TYPE_2, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
		return 0;
	sdmmc_get_rsp(storage->sdmmc, buf, 0x10, SDMMC_RSP_TYPE_2);
	return 1;
}

static int _sdmmc_storage_set_blocklen(sdmmc_storage_t *storage, u32 blocklen)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SET_BLOCKLEN, blocklen, 0, R1_STATE_TRAN);
}

static int _sdmmc_storage_get_status(sdmmc_storage_t *storage, u32 *resp, u32 mask)
{
	return _sdmmc_storage_execute_cmd_type1_ex(storage, resp, MMC_SEND_STATUS, storage->rca << 16, 0, R1_STATE_TRAN, mask);
}

static int _sdmmc_storage_check_status(sdmmc_storage_t *storage)
{
	u32 tmp;
	return _sdmmc_storage_get_status(storage, &tmp, 0);
}

static int _sdmmc_storage_readwrite_ex(sdmmc_storage_t *storage, u32 *blkcnt_out, u32 sector, u32 num_sectors, void *buf, u32 is_write)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_req_t reqbuf;
	u32 tmp = 0;

	sdmmc_init_cmd(&cmdbuf, is_write ? MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK, sector, SDMMC_RSP_TYPE_1, 0);

	reqbuf.buf = buf;
	reqbuf.num_sectors = num_sectors;
	reqbuf.blksize = 512;
	reqbuf.is_write = is_write;
	reqbuf.is_multi_block = 1;
	reqbuf.is_auto_cmd12 = 1;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, blkcnt_out))
	{			
		sdmmc_stop_transmission(storage->sdmmc, &tmp);
		_sdmmc_storage_get_status(storage, &tmp, 0);
		return 0;
	}
	
	return 1;
}

int sdmmc_storage_end(sdmmc_storage_t *storage)
{
	if (!_sdmmc_storage_go_idle_state(storage))
		return 0;
	sdmmc_end(storage->sdmmc);
	return 1;
}

static int _sdmmc_storage_readwrite(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf, u32 is_write)
{
	u8 *bbuf = (u8 *)buf;

	while (num_sectors)
	{
		u32 blkcnt = 0;
		//Retry 9 times on error.
		u32 retries = 10;
		do
		{
			if (_sdmmc_storage_readwrite_ex(storage, &blkcnt, sector, MIN(num_sectors, 0xFFFF), bbuf, is_write))
				goto out;
			else
				retries--;

			msleep(100);
		} while (retries);
		return 0;

out:;
		DPRINTF("readwrite: %08X\n", blkcnt);
		sector += blkcnt;
		num_sectors -= blkcnt;
		bbuf += 512 * blkcnt;
	}
	return 1;
}

extern _sdmmc_accessor_sd sdmmc_accessor_sd;
extern _sdmmc_accessor_nand sdmmc_accessor_nand;
int sdmmc_storage_read(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf)
{
	if (!custom_driver)
	{
		sdmmc_accessor_t *accessor_sd = sdmmc_accessor_sd();
		sdmmc_accessor_t *accessor_nand = sdmmc_accessor_nand();

		if (sdmmc_calculate_dma_addr(accessor_sd, buf, num_sectors))
		{
			return !accessor_sd->vtab->read_write(accessor_sd, sector, num_sectors, buf, num_sectors * 512, 1);
		}
		else
		{
			if (sdmmc_calculate_dma_addr(accessor_nand, buf, num_sectors))
			{
				// buf is on the nand dma buffer
				int original_dma_idx = sdmmc_calculate_dma_index(accessor_nand, buf, num_sectors);
				sdmmc_dma_buffer_t *original_dma_buffer = &accessor_nand->parent->dmaBuffers[original_dma_idx];

				// Next entry
				int dma_idx = sdmmc_calculate_fitting_dma_index(accessor_sd, num_sectors) + 1;
				
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer = original_dma_buffer->device_addr_buffer;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_masked = original_dma_buffer->device_addr_buffer_masked;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_size = original_dma_buffer->device_addr_buffer_size;

				u64 res = accessor_sd->vtab->read_write(accessor_sd, sector, num_sectors, buf, num_sectors * 512, 1);

				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer = 0;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_masked = 0;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_size = 0;

				return !res;
			}
			else
			{
				// buf is on a heap
				int dma_idx = sdmmc_calculate_fitting_dma_index(accessor_sd, num_sectors);
				void *dma_buf = &accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer[0];

				u64 res = accessor_sd->vtab->read_write(accessor_sd, sector, num_sectors, dma_buf, num_sectors * 512, 1);
				memcpy(buf, dma_buf, num_sectors * 512);

				return !res;
			}
		}
	}
	else
	{
		return _sdmmc_storage_readwrite(storage, sector, num_sectors, buf, 0);
	}
}

int sdmmc_storage_write(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf)
{
	if (!custom_driver)
	{
		sdmmc_accessor_t *accessor_sd = sdmmc_accessor_sd();
		sdmmc_accessor_t *accessor_nand = sdmmc_accessor_nand();

		if (sdmmc_calculate_dma_addr(accessor_sd, buf, num_sectors))
		{
			return !accessor_sd->vtab->read_write(accessor_sd, sector, num_sectors, buf, num_sectors * 512, 0);
		}
		else
		{
			if (sdmmc_calculate_dma_addr(accessor_nand, buf, num_sectors))
			{
				// buf is on the nand dma buffer
				int original_dma_idx = sdmmc_calculate_dma_index(accessor_nand, buf, num_sectors);
				sdmmc_dma_buffer_t *original_dma_buffer = &accessor_nand->parent->dmaBuffers[original_dma_idx];

				// Next entry
				int dma_idx = sdmmc_calculate_fitting_dma_index(accessor_sd, num_sectors) + 1;

				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer = original_dma_buffer->device_addr_buffer;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_masked = original_dma_buffer->device_addr_buffer_masked;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_size = original_dma_buffer->device_addr_buffer_size;

				u64 res = accessor_sd->vtab->read_write(accessor_sd, sector, num_sectors, buf, num_sectors * 512, 0);
				
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer = 0;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_masked = 0;
				accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer_size = 0;

				return !res;
			}
			else
			{
				// buf is on a heap
				int dma_idx = sdmmc_calculate_fitting_dma_index(accessor_sd, num_sectors);
				void *dma_buf = &accessor_sd->parent->dmaBuffers[dma_idx].device_addr_buffer[0];

				memcpy(dma_buf, buf, num_sectors * 512);
				u64 res = accessor_sd->vtab->read_write(accessor_sd, sector, num_sectors, dma_buf, num_sectors * 512, 0);

				return !res;
			}
		}
	}
	else
	{
		return _sdmmc_storage_readwrite(storage, sector, num_sectors, buf, 1);
	}
}

/*
* MMC specific functions.
*/

static int _mmc_storage_get_op_cond_inner(sdmmc_storage_t *storage, u32 *pout, u32 power)
{
	sdmmc_cmd_t cmd;

	u32 arg = 0;
	switch (power)
	{
	case SDMMC_POWER_1_8:
		arg = 0x40000080; //Sector access, voltage.
		break;
	case SDMMC_POWER_3_3:
		arg = 0x403F8000; //Sector access, voltage.
		break;
	default:
		return 0;
	}

	sdmmc_init_cmd(&cmd, MMC_SEND_OP_COND, arg, SDMMC_RSP_TYPE_3, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmd, 0, 0))
		return 0;

	return sdmmc_get_rsp(storage->sdmmc, pout, 4, SDMMC_RSP_TYPE_3);
}

static int _mmc_storage_get_op_cond(sdmmc_storage_t *storage, u32 power)
{
	u32 timeout = get_tmr_ms() + 1500;

	while (1)
	{
		u32 cond = 0;
		if (!_mmc_storage_get_op_cond_inner(storage, &cond, power))
			break;
		if (cond & MMC_CARD_BUSY)
		{
			if (cond & 0x40000000)
				storage->has_sector_access = 1;
			return 1;
		}
		if (get_tmr_ms() > timeout)
			break;
		usleep(1000);
	}

	return 0;
}

static int _mmc_storage_set_relative_addr(sdmmc_storage_t *storage)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SET_RELATIVE_ADDR, storage->rca << 16, 0, 0x10);
}

static void _mmc_storage_parse_cid(sdmmc_storage_t *storage)
{
	u32 *raw_cid = (u32 *)&(storage->raw_cid);

	switch (storage->csd.mmca_vsn)
	{
	case 0: /* MMC v1.0 - v1.2 */
	case 1: /* MMC v1.4 */
		storage->cid.prod_name[6] = unstuff_bits(raw_cid, 48, 8);
		storage->cid.manfid = unstuff_bits(raw_cid, 104, 24);
		storage->cid.hwrev  = unstuff_bits(raw_cid, 44, 4);
		storage->cid.fwrev  = unstuff_bits(raw_cid, 40, 4);
		storage->cid.serial = unstuff_bits(raw_cid, 16, 24);
		break;
	case 2: /* MMC v2.0 - v2.2 */
	case 3: /* MMC v3.1 - v3.3 */
	case 4: /* MMC v4 */
		storage->cid.manfid   = unstuff_bits(raw_cid, 120, 8);
		storage->cid.card_bga = unstuff_bits(raw_cid, 112, 2);
		storage->cid.oemid    = unstuff_bits(raw_cid, 104, 8);
		storage->cid.prv      = unstuff_bits(raw_cid, 48, 8);
		storage->cid.serial   = unstuff_bits(raw_cid, 16, 32);
		break;
	default:
		break;
	}

	storage->cid.prod_name[0] = unstuff_bits(raw_cid, 96, 8);
	storage->cid.prod_name[1] = unstuff_bits(raw_cid, 88, 8);
	storage->cid.prod_name[2] = unstuff_bits(raw_cid, 80, 8);
	storage->cid.prod_name[3] = unstuff_bits(raw_cid, 72, 8);
	storage->cid.prod_name[4] = unstuff_bits(raw_cid, 64, 8);
	storage->cid.prod_name[5] = unstuff_bits(raw_cid, 56, 8);

	storage->cid.month = unstuff_bits(raw_cid, 12, 4);
	storage->cid.year  = unstuff_bits(raw_cid, 8, 4) + 1997;
	if (storage->ext_csd.rev >= 5)
	{
		if (storage->cid.year < 2010)
			storage->cid.year += 16;
	}
}

static void _mmc_storage_parse_csd(sdmmc_storage_t *storage)
{
	u32 *raw_csd = (u32 *)&(storage->raw_csd);

	storage->csd.mmca_vsn = unstuff_bits(raw_csd, 122, 4);
	storage->csd.structure = unstuff_bits(raw_csd, 126, 2);
	storage->csd.cmdclass = unstuff_bits(raw_csd, 84, 12);
	storage->csd.read_blkbits = unstuff_bits(raw_csd, 80, 4);
	storage->csd.capacity = (1 + unstuff_bits(raw_csd, 62, 12)) << (unstuff_bits(raw_csd, 47, 3) + 2);
}

static void _mmc_storage_parse_ext_csd(sdmmc_storage_t *storage, u8 *buf)
{
	storage->ext_csd.rev = buf[EXT_CSD_REV];
	storage->ext_csd.ext_struct = buf[EXT_CSD_STRUCTURE];
	storage->ext_csd.card_type = buf[EXT_CSD_CARD_TYPE];
	storage->ext_csd.dev_version = *(u16 *)&buf[EXT_CSD_DEVICE_VERSION];
	storage->ext_csd.boot_mult = buf[EXT_CSD_BOOT_MULT];
	storage->ext_csd.rpmb_mult = buf[EXT_CSD_RPMB_MULT];
	storage->ext_csd.sectors = *(u32 *)&buf[EXT_CSD_SEC_CNT];
	storage->ext_csd.bkops = buf[EXT_CSD_BKOPS_SUPPORT];
	storage->ext_csd.bkops_en = buf[EXT_CSD_BKOPS_EN];
	storage->ext_csd.bkops_status = buf[EXT_CSD_BKOPS_STATUS];

	storage->sec_cnt  = *(u32 *)&buf[EXT_CSD_SEC_CNT];
}

static int _mmc_storage_get_ext_csd(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, MMC_SEND_EXT_CSD, 0, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 512;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	_mmc_storage_parse_ext_csd(storage, buf);

	return _sdmmc_storage_check_result(tmp);
}

static int _mmc_storage_switch(sdmmc_storage_t *storage, u32 arg)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SWITCH, arg, 1, 0x10);
}

static int _mmc_storage_switch_buswidth(sdmmc_storage_t *storage, u32 bus_width)
{
	if (bus_width == SDMMC_BUS_WIDTH_1)
		return 1;

	u32 arg = 0;
	switch (bus_width)
	{
	case SDMMC_BUS_WIDTH_4:
		arg = SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_4);
		break;
	case SDMMC_BUS_WIDTH_8:
		arg = SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_8);
		break;
	}

	if (_mmc_storage_switch(storage, arg))
		if (_sdmmc_storage_check_status(storage))
		{
			sdmmc_set_bus_width(storage->sdmmc, bus_width);
			return 1;
		}

	return 0;
}

static int _mmc_storage_enable_HS(sdmmc_storage_t *storage, int check)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS)))
		return 0;
	if (check && !_sdmmc_storage_check_status(storage))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, 2))
		return 0;
	DPRINTF("[MMC] switched to HS\n");
	storage->csd.busspeed = 52;
	if (check || _sdmmc_storage_check_status(storage))
		return 1;
	return 0;
}

static int _mmc_storage_enable_HS200(sdmmc_storage_t *storage)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS200)))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, 3))
		return 0;
	if (!sdmmc_config_tuning(storage->sdmmc, 3, MMC_SEND_TUNING_BLOCK_HS200))
		return 0;
	DPRINTF("[MMC] switched to HS200\n");
	storage->csd.busspeed = 200;
	return _sdmmc_storage_check_status(storage);
}

static int _mmc_storage_enable_HS400(sdmmc_storage_t *storage)
{
	if (!_mmc_storage_enable_HS200(storage))
		return 0;
	sdmmc_get_venclkctl(storage->sdmmc);
	if (!_mmc_storage_enable_HS(storage, 0))
		return 0;
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_BUS_WIDTH, EXT_CSD_DDR_BUS_WIDTH_8)))
		return 0;
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS400)))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, 4))
		return 0;
	DPRINTF("[MMC] switched to HS400\n");
	storage->csd.busspeed = 400;
	return _sdmmc_storage_check_status(storage);
}

static int _mmc_storage_enable_highspeed(sdmmc_storage_t *storage, u32 card_type, u32 type)
{
	//TODO: this should be a config item.
	// --v
	if (!1 || sdmmc_get_voltage(storage->sdmmc) != SDMMC_POWER_1_8)
		goto out;

	if (sdmmc_get_bus_width(storage->sdmmc) == SDMMC_BUS_WIDTH_8 &&
		card_type & EXT_CSD_CARD_TYPE_HS400_1_8V &&
		type == 4)
		return _mmc_storage_enable_HS400(storage);

	if (sdmmc_get_bus_width(storage->sdmmc) == SDMMC_BUS_WIDTH_8 ||
		(sdmmc_get_bus_width(storage->sdmmc) == SDMMC_BUS_WIDTH_4
		&& card_type & EXT_CSD_CARD_TYPE_HS200_1_8V
		&& (type == 4 || type == 3)))
		return _mmc_storage_enable_HS200(storage);

out:;
	if (card_type & EXT_CSD_CARD_TYPE_HS_52)
		return _mmc_storage_enable_HS(storage, 1);
	return 1;
}

static int _mmc_storage_enable_bkops(sdmmc_storage_t *storage)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_SET_BITS, EXT_CSD_BKOPS_EN, EXT_CSD_BKOPS_LEVEL_2)))
		return 0;
	return _sdmmc_storage_check_status(storage);
}

int sdmmc_storage_init_mmc(sdmmc_storage_t *storage, sdmmc_t *sdmmc, u32 id, u32 bus_width, u32 type)
{
	memset(storage, 0, sizeof(sdmmc_storage_t));
	storage->sdmmc = sdmmc;
	storage->rca = 2; //TODO: this could be a config item.

	if (!sdmmc_init(sdmmc, id, SDMMC_POWER_1_8, SDMMC_BUS_WIDTH_1, 0, 0))
		return 0;
	DPRINTF("[MMC] after init\n");

	usleep(1000 + (74000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (!_sdmmc_storage_go_idle_state(storage))
		return 0;
	DPRINTF("[MMC] went to idle state\n");

	if (!_mmc_storage_get_op_cond(storage, SDMMC_POWER_1_8))
		return 0;
	DPRINTF("[MMC] got op cond\n");

	if (!_sdmmc_storage_get_cid(storage, storage->raw_cid))
		return 0;
	DPRINTF("[MMC] got cid\n");

	if (!_mmc_storage_set_relative_addr(storage))
		return 0;
	DPRINTF("[MMC] set relative addr\n");

	if (!_sdmmc_storage_get_csd(storage, storage->raw_csd))
		return 0;
	DPRINTF("[MMC] got csd\n");
	_mmc_storage_parse_csd(storage);

	if (!sdmmc_setup_clock(storage->sdmmc, 1))
		return 0;
	DPRINTF("[MMC] after setup clock\n");

	if (!_sdmmc_storage_select_card(storage))
		return 0;
	DPRINTF("[MMC] card selected\n");

	if (!_sdmmc_storage_set_blocklen(storage, 512))
		return 0;
	DPRINTF("[MMC] set blocklen to 512\n");

	u32 *csd = (u32 *)storage->raw_csd;
	//Check system specification version, only version 4.0 and later support below features.
	if (unstuff_bits(csd, 122, 4) < CSD_SPEC_VER_4)
	{
		storage->sec_cnt = (1 + unstuff_bits(csd, 62, 12)) << (unstuff_bits(csd, 47, 3) + 2);
		return 1;
	}

	if (!_mmc_storage_switch_buswidth(storage, bus_width))
		return 0;
	DPRINTF("[MMC] switched buswidth\n");

	u8 *ext_csd = (u8 *)malloc(512);
	if (!_mmc_storage_get_ext_csd(storage, ext_csd))
	{
		free(ext_csd);
		return 0;
	}
	free(ext_csd);
	DPRINTF("[MMC] got ext_csd\n");
	_mmc_storage_parse_cid(storage); //This needs to be after csd and ext_csd

	/* When auto BKOPS is enabled the mmc device should be powered all the time until we disable this and check status.
	   Disable it for now until BKOPS disable added to power down sequence at sdmmc_storage_end().
	   Additionally this works only when we put the device in idle mode which we don't after enabling it. */
	if (storage->ext_csd.bkops & 0x1 && !(storage->ext_csd.bkops_en & EXT_CSD_BKOPS_LEVEL_2) && 0)
	{
		_mmc_storage_enable_bkops(storage);
		DPRINTF("[MMC] BKOPS enabled\n");
	}
	else
	{
		DPRINTF("[MMC] BKOPS disabled\n");
	}

	if (!_mmc_storage_enable_highspeed(storage, storage->ext_csd.card_type, type))
		return 0;
	DPRINTF("[MMC] succesfully switched to highspeed mode\n");

	sdmmc_sd_clock_ctrl(storage->sdmmc, 1);

	return 1;
}

int sdmmc_storage_set_mmc_partition(sdmmc_storage_t *storage, u32 partition)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_PART_CONFIG, partition)))
		return 0;
	if (!_sdmmc_storage_check_status(storage))
		return 0;
	storage->partition = partition;
	return 1;
}

/*
* SD specific functions.
*/

static int _sd_storage_execute_app_cmd(sdmmc_storage_t *storage, u32 expected_state, u32 mask, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out)
{
	u32 tmp;
	if (!_sdmmc_storage_execute_cmd_type1_ex(storage, &tmp, MMC_APP_CMD, storage->rca << 16, 0, expected_state, mask))
		return 0;
	return sdmmc_execute_cmd(storage->sdmmc, cmd, req, blkcnt_out);
}

static int _sd_storage_execute_app_cmd_type1(sdmmc_storage_t *storage, u32 *resp, u32 cmd, u32 arg, u32 check_busy, u32 expected_state)
{
	if (!_sdmmc_storage_execute_cmd_type1(storage, MMC_APP_CMD, storage->rca << 16, 0, R1_STATE_TRAN))
		return 0;
	return _sdmmc_storage_execute_cmd_type1_ex(storage, resp, cmd, arg, check_busy, expected_state, 0);
}

static int _sd_storage_send_if_cond(sdmmc_storage_t *storage)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_SEND_IF_COND, 0x1AA, SDMMC_RSP_TYPE_5, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
		return 1; // The SD Card is version 1.X

	u32 resp = 0;
	if (!sdmmc_get_rsp(storage->sdmmc, &resp, 4, SDMMC_RSP_TYPE_5))
		return 2;

	return (resp & 0xFF) == 0xAA ? 0 : 2;
}

static int _sd_storage_get_op_cond_once(sdmmc_storage_t *storage, u32 *cond, int is_version_1, int supports_low_voltage)
{
	sdmmc_cmd_t cmdbuf;
	// Support for Current > 150mA
	u32 arg = (~is_version_1 & 1) ? SD_OCR_XPC : 0;
	// Support for handling block-addressed SDHC cards
	arg	|= (~is_version_1 & 1) ? SD_OCR_CCS : 0;
	// Support for 1.8V
	arg |= (supports_low_voltage & ~is_version_1 & 1) ? SD_OCR_S18R : 0;
	// This is needed for most cards. Do not set bit7 even if 1.8V is supported.
	arg |= SD_OCR_VDD_32_33;
	sdmmc_init_cmd(&cmdbuf, SD_APP_OP_COND, arg, SDMMC_RSP_TYPE_3, 0);

	DPRINTF("[SD] before _sd_storage_execute_app_cmd\n");
	if (!_sd_storage_execute_app_cmd(storage, 0x10, is_version_1 ? 0x400000 : 0, &cmdbuf, 0, 0))
		return 0;
	DPRINTF("[SD] before sdmmc_get_rsp\n");
	return sdmmc_get_rsp(storage->sdmmc, cond, 4, SDMMC_RSP_TYPE_3);
}

static int _sd_storage_get_op_cond(sdmmc_storage_t *storage, int is_version_1, int supports_low_voltage)
{
	u32 timeout = get_tmr_ms() + 1500;

	while (1)
	{
		u32 cond = 0;
		if (!_sd_storage_get_op_cond_once(storage, &cond, is_version_1, supports_low_voltage))
		{
			DPRINTF("[SD] _sd_storage_get_op_cond_once failed\r\n");
			break;
		}

		if (cond & MMC_CARD_BUSY)
		{
			if (cond & SD_OCR_CCS)
				storage->has_sector_access = 1;

			if (cond & SD_ROCR_S18A && supports_low_voltage)
			{
				//The low voltage regulator configuration is valid for SDMMC1 only.
				if (storage->sdmmc->id == SDMMC_1 &&
					_sdmmc_storage_execute_cmd_type1(storage, SD_SWITCH_VOLTAGE, 0, 0, R1_STATE_READY))
				{
					if (!sdmmc_enable_low_voltage(storage->sdmmc))
						return 0;
					storage->is_low_voltage = 1;

					DPRINTF("-> switched to low voltage\n");
				}
			}

			return 1;
		}
		if (get_tmr_ms() > timeout)
			break;
		msleep(10); // Needs to be at least 10ms for some SD Cards
	}

	DPRINTF("[SD] _sd_storage_get_op_cond Timeout\r\n");

	return 0;
}

static int _sd_storage_get_rca(sdmmc_storage_t *storage)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_SEND_RELATIVE_ADDR, 0, SDMMC_RSP_TYPE_4, 0);

	u32 timeout = get_tmr_ms() + 1500;

	while (1)
	{
		if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
			break;

		u32 resp = 0;
		if (!sdmmc_get_rsp(storage->sdmmc, &resp, 4, SDMMC_RSP_TYPE_4))
			break;

		if (resp >> 16)
		{
			storage->rca = resp >> 16;
			return 1;
		}

		if (get_tmr_ms() > timeout)
			break;
		usleep(1000);
	}

	return 0;
}

static void _sd_storage_parse_scr(sdmmc_storage_t *storage)
{
	// unstuff_bits can parse only 4 u32
	u32 resp[4];

	resp[3] = *(u32 *)&storage->raw_scr[4];
	resp[2] = *(u32 *)&storage->raw_scr[0];

	storage->scr.sda_vsn = unstuff_bits(resp, 56, 4);
	storage->scr.bus_widths = unstuff_bits(resp, 48, 4);
	if (storage->scr.sda_vsn == SCR_SPEC_VER_2)
		/* Check if Physical Layer Spec v3.0 is supported */
		storage->scr.sda_spec3 = unstuff_bits(resp, 47, 1);
	if (storage->scr.sda_spec3)
		storage->scr.cmds = unstuff_bits(resp, 32, 2);
}

int _sd_storage_get_scr(sdmmc_storage_t *storage, u8 *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_APP_SEND_SCR, 0, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 8;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!_sd_storage_execute_app_cmd(storage, R1_STATE_TRAN, 0, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	//Prepare buffer for unstuff_bits
	for (int i = 0; i < 8; i+=4)
	{
		storage->raw_scr[i + 3] = buf[i];
		storage->raw_scr[i + 2] = buf[i + 1];
		storage->raw_scr[i + 1] = buf[i + 2];
		storage->raw_scr[i]     = buf[i + 3];
	}
	_sd_storage_parse_scr(storage);

	return _sdmmc_storage_check_result(tmp);
}

int _sd_storage_switch_get(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_SWITCH, 0xFFFFFF, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 64;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	return _sdmmc_storage_check_result(tmp);
}

int _sd_storage_switch(sdmmc_storage_t *storage, void *buf, int mode, int group, u32 arg)
{
	sdmmc_cmd_t cmdbuf;
	u32 switchcmd = mode << 31 | 0x00FFFFFF;
	switchcmd &= ~(0xF << (group * 4));
	switchcmd |= arg << (group * 4);
	sdmmc_init_cmd(&cmdbuf, SD_SWITCH, switchcmd, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 64;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	return _sdmmc_storage_check_result(tmp);
}

void _sd_storage_set_current_limit(sdmmc_storage_t *storage, u8 *buf)
{
	u32 pwr = SD_SET_CURRENT_LIMIT_800;
	_sd_storage_switch(storage, buf, SD_SWITCH_SET, 3, pwr);

	while (pwr > 0)
	{
		pwr--;
		_sd_storage_switch(storage, buf, SD_SWITCH_SET, 3, pwr);
		if (((buf[15] >> 4) & 0x0F) == pwr)
			break;
	}

	switch (pwr)
	{
	case SD_SET_CURRENT_LIMIT_800:
		DPRINTF("[SD] Power limit raised to 800mA\n");
		break;
	case SD_SET_CURRENT_LIMIT_600:
		DPRINTF("[SD] Power limit raised to 600mA\n");
		break;
	case SD_SET_CURRENT_LIMIT_400:
		DPRINTF("[SD] Power limit raised to 800mA\n");
		break;
	default:
	case SD_SET_CURRENT_LIMIT_200:
		DPRINTF("[SD] Power limit defaulted to 200mA\n");
		break;
	}
}

int _sd_storage_enable_highspeed(sdmmc_storage_t *storage, u32 hs_type, u8 *buf)
{
	if (!_sd_storage_switch(storage, buf, SD_SWITCH_CHECK, 0, hs_type))
		return 0;

	u32 type_out = buf[16] & 0xF;
	if (type_out != hs_type)
		return 0;

	if ((((u16)buf[0] << 8) | buf[1]) < 0x320)
	{
		if (!_sd_storage_switch(storage, buf, SD_SWITCH_SET, 0, hs_type))
			return 0;

		if (type_out != (buf[16] & 0xF))
			return 0;
	}

	return 1;
}

int _sd_storage_enable_highspeed_low_volt(sdmmc_storage_t *storage, u32 type, u8 *buf)
{
	// Try to raise the current limit to let the card perform better.
	_sd_storage_set_current_limit(storage, buf);

	if (sdmmc_get_bus_width(storage->sdmmc) != SDMMC_BUS_WIDTH_4)
		return 0;

	if (!_sd_storage_switch_get(storage, buf))
		return 0;

	u32 hs_type = 0;
	switch (type)
	{
	case 11:
		// Fall through if not supported.
		if (buf[13] & SD_MODE_UHS_SDR104)
		{
			type = 11;
			hs_type = UHS_SDR104_BUS_SPEED;
			DPRINTF("[SD] Bus speed set to SDR104\n");
			storage->csd.busspeed = 104;
			break;
		}
	case 10:
		if (buf[13] & SD_MODE_UHS_SDR50)
		{
			type = 10;
			hs_type = UHS_SDR50_BUS_SPEED;
			DPRINTF("[SD] Bus speed set to SDR50\n");
			storage->csd.busspeed = 50;
			break;
		}
	case 8:
		if (!(buf[13] & SD_MODE_UHS_SDR12))
			return 0;
		type = 8;
		hs_type = UHS_SDR12_BUS_SPEED;
		DPRINTF("[SD] Bus speed set to SDR12\n");
		storage->csd.busspeed = 12;
		break;
	default:
		return 0;
		break;
	}

	if (!_sd_storage_enable_highspeed(storage, hs_type, buf))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, type))
		return 0;
	if (!sdmmc_config_tuning(storage->sdmmc, type, MMC_SEND_TUNING_BLOCK))
		return 0;
	return _sdmmc_storage_check_status(storage);
}

int _sd_storage_enable_highspeed_high_volt(sdmmc_storage_t *storage, u8 *buf)
{
	if (!_sd_storage_switch_get(storage, buf))
		return 0;
	if (!(buf[13] & SD_MODE_HIGH_SPEED))
		return 1;

	if (!_sd_storage_enable_highspeed(storage, 1, buf))
		return 0;
	if (!_sdmmc_storage_check_status(storage))
		return 0;
	return sdmmc_setup_clock(storage->sdmmc, 7);
}

static void _sd_storage_parse_ssr(sdmmc_storage_t *storage)
{
	// unstuff_bits supports only 4 u32 so break into 2 x 16byte groups
	u32 raw_ssr1[4];
	u32 raw_ssr2[4];

	raw_ssr1[3] = *(u32 *)&storage->raw_ssr[12];
	raw_ssr1[2] = *(u32 *)&storage->raw_ssr[8];
	raw_ssr1[1] = *(u32 *)&storage->raw_ssr[4];
	raw_ssr1[0] = *(u32 *)&storage->raw_ssr[0];

	raw_ssr2[3] = *(u32 *)&storage->raw_ssr[28];
	raw_ssr2[2] = *(u32 *)&storage->raw_ssr[24];
	raw_ssr2[1] = *(u32 *)&storage->raw_ssr[20];
	raw_ssr2[0] = *(u32 *)&storage->raw_ssr[16];

	storage->ssr.bus_width = (unstuff_bits(raw_ssr1, 510 - 384, 2) & SD_BUS_WIDTH_4) ? 4 : 1;
	switch(unstuff_bits(raw_ssr1, 440 - 384, 8))
	{
	case 0:
		storage->ssr.speed_class = 0;
		break;
	case 1:
		storage->ssr.speed_class = 2;
		break;
	case 2:
		storage->ssr.speed_class = 4;
		break;
	case 3:
		storage->ssr.speed_class = 6;
		break;
	case 4:
		storage->ssr.speed_class = 10;
		break;
	default:
		storage->ssr.speed_class = unstuff_bits(raw_ssr1, 440 - 384, 8);
		break;
	}
	storage->ssr.uhs_grade = unstuff_bits(raw_ssr1, 396 - 384, 4);
	storage->ssr.video_class = unstuff_bits(raw_ssr1, 384 - 384, 8);

	storage->ssr.app_class = unstuff_bits(raw_ssr2, 336 - 256, 4);
}

static int _sd_storage_get_ssr(sdmmc_storage_t *storage, u8 *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_APP_SD_STATUS, 0, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 64;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!(storage->csd.cmdclass & CCC_APP_SPEC))
	{
		DPRINTF("[SD] ssr: Card lacks mandatory SD Status function\n");
		return 0;
	}

	if (!_sd_storage_execute_app_cmd(storage, R1_STATE_TRAN, 0, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
    //Prepare buffer for unstuff_bits
	for (int i = 0; i < 64; i+=4)
	{
		storage->raw_ssr[i + 3] = buf[i];
		storage->raw_ssr[i + 2] = buf[i + 1];
		storage->raw_ssr[i + 1] = buf[i + 2];
		storage->raw_ssr[i]     = buf[i + 3];
	}
	_sd_storage_parse_ssr(storage);

	return _sdmmc_storage_check_result(tmp);
}

static void _sd_storage_parse_cid(sdmmc_storage_t *storage)
{
	u32 *raw_cid = (u32 *)&(storage->raw_cid);

	storage->cid.manfid = unstuff_bits(raw_cid, 120, 8);
	storage->cid.oemid  = unstuff_bits(raw_cid, 104, 16);
	storage->cid.prod_name[0] = unstuff_bits(raw_cid, 96, 8);
	storage->cid.prod_name[1] = unstuff_bits(raw_cid, 88, 8);
	storage->cid.prod_name[2] = unstuff_bits(raw_cid, 80, 8);
	storage->cid.prod_name[3] = unstuff_bits(raw_cid, 72, 8);
	storage->cid.prod_name[4] = unstuff_bits(raw_cid, 64, 8);
	storage->cid.hwrev  = unstuff_bits(raw_cid, 60, 4);
	storage->cid.fwrev  = unstuff_bits(raw_cid, 56, 4);
	storage->cid.serial = unstuff_bits(raw_cid, 24, 32);
	storage->cid.month  = unstuff_bits(raw_cid, 8, 4);
	storage->cid.year   = unstuff_bits(raw_cid, 12, 8) + 2000;
}

static void _sd_storage_parse_csd(sdmmc_storage_t *storage)
{
	u32 *raw_csd = (u32 *)&(storage->raw_csd);

	storage->csd.structure = unstuff_bits(raw_csd, 126, 2);
	storage->csd.cmdclass  = unstuff_bits(raw_csd, 84, 12);
	storage->csd.read_blkbits  = unstuff_bits(raw_csd, 80, 4);
	storage->csd.write_protect = unstuff_bits(raw_csd, 12, 2);
	switch(storage->csd.structure)
	{
	case 0:
		storage->csd.capacity = (1 + unstuff_bits(raw_csd, 62, 12)) << (unstuff_bits(raw_csd, 47, 3) + 2);
		break;
	case 1:
		storage->csd.c_size = (1 + unstuff_bits(raw_csd, 48, 22));
		storage->csd.capacity = storage->csd.c_size << 10;
		storage->csd.read_blkbits = 9;
		break;
	}
}

int sdmmc_storage_init_sd(sdmmc_storage_t *storage, sdmmc_t *sdmmc, u32 id, u32 bus_width, u32 type)
{
	int is_version_1 = 0;

	memset(storage, 0, sizeof(sdmmc_storage_t));
	storage->sdmmc = sdmmc;

	DPRINTF("[SD] before init\n");
	if (!sdmmc_init(sdmmc, id, SDMMC_POWER_3_3, SDMMC_BUS_WIDTH_1, 5, 0))
		return 0;
	DPRINTF("[SD] after init\n");

	usleep(1000 + (74000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (!_sdmmc_storage_go_idle_state(storage))
		return 0;
	DPRINTF("[SD] went to idle state\n");

	is_version_1 = _sd_storage_send_if_cond(storage);
	if (is_version_1 == 2)
		return 0;
	DPRINTF("[SD] after send if cond\n");

	if (!_sd_storage_get_op_cond(storage, is_version_1, bus_width == SDMMC_BUS_WIDTH_4 && type == 11))
		return 0;
	DPRINTF("[SD] got op cond\n");

	if (!_sdmmc_storage_get_cid(storage, storage->raw_cid))
		return 0;
	DPRINTF("[SD] got cid\n");
	_sd_storage_parse_cid(storage);

	if (!_sd_storage_get_rca(storage))
		return 0;
	DPRINTF("[SD] got rca (= %04X)\n", storage->rca);

	if (!_sdmmc_storage_get_csd(storage, storage->raw_csd))
		return 0;
	DPRINTF("[SD] got csd\n");

	//Parse CSD.
	_sd_storage_parse_csd(storage);
	switch (storage->csd.structure)
	{
	case 0:
		storage->sec_cnt = storage->csd.capacity;
		break;
	case 1:
		storage->sec_cnt = storage->csd.c_size << 10;
		break;
	default:
		DPRINTF("[SD] Unknown CSD structure %d\n", storage->csd.structure);
		break;
	}

	if (!storage->is_low_voltage)
	{
		if (!sdmmc_setup_clock(storage->sdmmc, 6))
			return 0;
		DPRINTF("[SD] after setup clock\n");
	}

	if (!_sdmmc_storage_select_card(storage))
		return 0;
	DPRINTF("[SD] card selected\n");

	if (!_sdmmc_storage_set_blocklen(storage, 512))
		return 0;
	DPRINTF("[SD] set blocklen to 512\n");

	u32 tmp = 0;
	if (!_sd_storage_execute_app_cmd_type1(storage, &tmp, SD_APP_SET_CLR_CARD_DETECT, 0, 0, R1_STATE_TRAN))
		return 0;
	DPRINTF("[SD] cleared card detect\n");

	u8 *buf = (u8 *)malloc(512);
	if (!_sd_storage_get_scr(storage, buf))
	{
		free(buf);
		return 0;
	}
		
	DPRINTF("[SD] got scr\n");

	// Check if card supports a wider bus and if it's not SD Version 1.X
	if (bus_width == SDMMC_BUS_WIDTH_4 && (storage->scr.bus_widths & 4) && (storage->scr.sda_vsn & 0xF))
	{
		if (!_sd_storage_execute_app_cmd_type1(storage, &tmp, SD_APP_SET_BUS_WIDTH, SD_BUS_WIDTH_4, 0, R1_STATE_TRAN))
		{
			free(buf);
			return 0;
		}
		sdmmc_set_bus_width(storage->sdmmc, SDMMC_BUS_WIDTH_4);
		DPRINTF("[SD] switched to wide bus width\n");
	}
	else
	{
		DPRINTF("[SD] SD does not support wide bus width\n");
	}

	if (storage->is_low_voltage)
	{
		if (!_sd_storage_enable_highspeed_low_volt(storage, type, buf))
		{
			free(buf);
			return 0;
		}
		DPRINTF("[SD] enabled highspeed (low voltage)\n");
	}
	else if (type != 6 && (storage->scr.sda_vsn & 0xF) != 0)
	{
		if (!_sd_storage_enable_highspeed_high_volt(storage, buf))
		{
			free(buf);
			return 0;
		}
		DPRINTF("[SD] enabled highspeed (high voltage)\n");
		storage->csd.busspeed = 25;
	}

	sdmmc_sd_clock_ctrl(sdmmc, 1);

	// Parse additional card info from sd status.
	if (_sd_storage_get_ssr(storage, buf))
	{
		DPRINTF("[SD] got sd status\n");
	}

	free(buf);
	return 1;
}

/*
* Gamecard specific functions.
*/

int _gc_storage_custom_cmd(sdmmc_storage_t *storage, void *buf)
{
	u32 resp;
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, 60, 0, SDMMC_RSP_TYPE_1, 1);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 64;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 1;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
	{
		sdmmc_stop_transmission(storage->sdmmc, &resp);
		return 0;
	}

	if (!sdmmc_get_rsp(storage->sdmmc, &resp, 4, SDMMC_RSP_TYPE_1))
		return 0;
	if (!_sdmmc_storage_check_result(resp))
		return 0;
	return _sdmmc_storage_check_status(storage);
}

int sdmmc_storage_init_gc(sdmmc_storage_t *storage, sdmmc_t *sdmmc)
{
	memset(storage, 0, sizeof(sdmmc_storage_t));
	storage->sdmmc = sdmmc;

	if (!sdmmc_init(sdmmc, SDMMC_2, SDMMC_POWER_1_8, SDMMC_BUS_WIDTH_8, 14, 0))
		return 0;
	DPRINTF("[gc] after init\n");

	usleep(1000 + (10000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (!sdmmc_config_tuning(storage->sdmmc, 14, MMC_SEND_TUNING_BLOCK_HS200))
		return 0;
	DPRINTF("[gc] after tuning\n");

	sdmmc_sd_clock_ctrl(sdmmc, 1);

	return 1;
}
