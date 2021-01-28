/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#include "sdmmc.h"
#include "mmc.h"
#include "sd.h"

#if defined(FUSEE_STAGE1_SRC)
#include "../../../fusee/fusee-primary/fusee-primary-main/src/timers.h"
#elif defined(FUSEE_STAGE2_SRC)
#include "../../../fusee/fusee-secondary/src/timers.h"
#elif defined(SEPT_STAGE2_SRC)
#include "../../../sept/sept-secondary/src/timers.h"
#endif

#define UNSTUFF_BITS(resp,start,size)                               \
({                                                                  \
    const int __size = size;                                        \
    const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1;    \
    const int __off = 3 - ((start) / 32);                           \
    const int __shft = (start) & 31;                                \
    uint32_t __res;                                                 \
                                                                    \
    __res = resp[__off] >> __shft;                                  \
    if (__size + __shft > 32)                                       \
        __res |= resp[__off-1] << ((32 - __shft) % 32);             \
    __res & __mask;                                                 \
})

static const unsigned int tran_exp[] = {
    10000,      100000,     1000000,    10000000,
    0,      0,      0,      0
};

static const unsigned char tran_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

static const unsigned int taac_exp[] = {
    1,  10, 100,    1000,   10000,  100000, 1000000, 10000000,
};

static const unsigned int taac_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

/*
    Common SDMMC device functions.
*/

static bool is_sdmmc_device_r1_error(uint32_t status) {
    return (status & (R1_OUT_OF_RANGE | R1_ADDRESS_ERROR | R1_BLOCK_LEN_ERROR
                    | R1_ERASE_SEQ_ERROR | R1_ERASE_PARAM | R1_WP_VIOLATION
                    | R1_LOCK_UNLOCK_FAILED | R1_COM_CRC_ERROR | R1_ILLEGAL_COMMAND
                    | R1_CARD_ECC_FAILED | R1_CC_ERROR | R1_ERROR | R1_CID_CSD_OVERWRITE
                    | R1_WP_ERASE_SKIP | R1_ERASE_RESET | R1_SWITCH_ERROR));
}

static int sdmmc_device_send_r1_cmd(sdmmc_device_t *device, uint32_t opcode, uint32_t arg, bool is_busy, uint32_t resp_mask, uint32_t resp_state) {
    sdmmc_command_t cmd = {};

    cmd.opcode = opcode;
    cmd.arg = arg;
    cmd.flags = (is_busy ? SDMMC_RSP_R1B : SDMMC_RSP_R1);

    /* Try to send the command. */
    if (!sdmmc_send_cmd(device->sdmmc, &cmd, 0, 0)) {
        return 0;
    }

    uint32_t resp = 0;

    /* Try to load back the response. */
    if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R1, &resp)) {
        return 0;
    }

    /* Mask the response, if necessary. */
    if (resp_mask) {
        resp &= ~(resp_mask);
    }
    
    /* We got an error state. */
    if (is_sdmmc_device_r1_error(resp)) {
        return 0;
    }

    /* We need to check for the desired state. */
    if (resp_state != 0xFFFFFFFF) {
        /* We didn't get the expected state. */
        if (R1_CURRENT_STATE(resp) != resp_state) {
            return 0;
        }
    }

    return 1;
}

static int sdmmc_device_go_idle(sdmmc_device_t *device) {
    sdmmc_command_t cmd = {};

    cmd.opcode = MMC_GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = SDMMC_RSP_NONE;

    return sdmmc_send_cmd(device->sdmmc, &cmd, 0, 0);
}

static int sdmmc_device_send_cid(sdmmc_device_t *device, uint32_t *cid) {
    sdmmc_command_t cmd = {};

    cmd.opcode = MMC_ALL_SEND_CID;
    cmd.arg = 0;
    cmd.flags = SDMMC_RSP_R2;

    /* Try to send the command. */
    if (!sdmmc_send_cmd(device->sdmmc, &cmd, 0, 0)) {
        return 0;
    }

    /* Try to load back the response. */
    return sdmmc_load_response(device->sdmmc, SDMMC_RSP_R2, cid);
}

static int sdmmc_device_send_csd(sdmmc_device_t *device, uint32_t *csd) {
    sdmmc_command_t cmd = {};

    cmd.opcode = MMC_SEND_CSD;
    cmd.arg = (device->rca << 16);
    cmd.flags = SDMMC_RSP_R2;

    /* Try to send the command. */
    if (!sdmmc_send_cmd(device->sdmmc, &cmd, 0, 0)) {
        return 0;
    }

    /* Try to load back the response. */
    return sdmmc_load_response(device->sdmmc, SDMMC_RSP_R2, csd);
}

static int sdmmc_device_select_card(sdmmc_device_t *device) {
    /* Try to send the command. */
    return sdmmc_device_send_r1_cmd(device, MMC_SELECT_CARD, (device->rca << 16), true, 0, 0xFFFFFFFF);
}

static int sdmmc_device_set_blocklen(sdmmc_device_t *device, uint32_t blocklen) {
    /* Try to send the command. */
    return sdmmc_device_send_r1_cmd(device, MMC_SET_BLOCKLEN, blocklen, false, 0, R1_STATE_TRAN);
}

static int sdmmc_device_send_status(sdmmc_device_t *device) {
    /* Try to send the command. */
    return sdmmc_device_send_r1_cmd(device, MMC_SEND_STATUS, (device->rca << 16), false, 0, R1_STATE_TRAN);
}

static int sdmmc_device_rw(sdmmc_device_t *device, uint32_t sector, uint32_t num_sectors, void *data, bool is_read) {
    uint8_t *buf = (uint8_t *)data;

    sdmmc_command_t cmd = {};
    sdmmc_request_t req = {};

    while (num_sectors) {
        uint32_t num_blocks_out = 0;
        uint32_t num_retries = 10;

        for (; num_retries > 0; num_retries--) {
            cmd.opcode = is_read ? MMC_READ_MULTIPLE_BLOCK : MMC_WRITE_MULTIPLE_BLOCK;
            cmd.arg = sector;
            cmd.flags = SDMMC_RSP_R1;

            req.data = buf;
            req.blksz = 512;
            req.num_blocks = num_sectors;
            req.is_read = is_read;
            req.is_multi_block = true;
            req.is_auto_cmd12 = true;

            /* Try to send the command. */
            if (!sdmmc_send_cmd(device->sdmmc, &cmd, &req, &num_blocks_out)) {
                /* Abort the transmission. */
                sdmmc_abort(device->sdmmc, MMC_STOP_TRANSMISSION);

                /* Peek the SD card's status. */
                sdmmc_device_send_status(device);

                /* Wait for a while. */
                mdelay(100);
            } else {
                break;
            }
        }

        /* Failed to read/write on all attempts. */
        if (!num_retries) {
            return 0;
        }

        /* Advance to next sector. */
        sector += num_blocks_out;
        num_sectors -= num_blocks_out;
        buf += (512 * num_blocks_out);
    }

    return 1;
}

int sdmmc_device_read(sdmmc_device_t *device, uint32_t sector, uint32_t num_sectors, void *data) {
    return sdmmc_device_rw(device, sector, num_sectors, data, true);
}

int sdmmc_device_write(sdmmc_device_t *device, uint32_t sector, uint32_t num_sectors, void *data) {
    return sdmmc_device_rw(device, sector, num_sectors, data, false);
}

int sdmmc_device_finish(sdmmc_device_t *device) {
    /* Place the device in idle state. */
    if (!sdmmc_device_go_idle(device)) {
        return 0;
    }

    /* Terminate the device. */
    sdmmc_finish(device->sdmmc);
    return 1;
}

/*
    SD device functions.
*/

static void sdmmc_sd_decode_cid(sdmmc_device_t *device, uint32_t *cid) {
    device->cid.manfid          = UNSTUFF_BITS(cid, 120, 8);
    device->cid.oemid           = UNSTUFF_BITS(cid, 104, 16);
    device->cid.prod_name[0]    = UNSTUFF_BITS(cid, 96, 8);
    device->cid.prod_name[1]    = UNSTUFF_BITS(cid, 88, 8);
    device->cid.prod_name[2]    = UNSTUFF_BITS(cid, 80, 8);
    device->cid.prod_name[3]    = UNSTUFF_BITS(cid, 72, 8);
    device->cid.prod_name[4]    = UNSTUFF_BITS(cid, 64, 8);
    device->cid.hwrev           = UNSTUFF_BITS(cid, 60, 4);
    device->cid.fwrev           = UNSTUFF_BITS(cid, 56, 4);
    device->cid.serial          = UNSTUFF_BITS(cid, 24, 32);
    device->cid.year            = UNSTUFF_BITS(cid, 12, 8) + 2000;     /* SD cards year offset */
    device->cid.month           = UNSTUFF_BITS(cid, 8, 4);
}

static int sdmmc_sd_decode_csd(sdmmc_device_t *device, uint32_t *csd) {
    unsigned int e, m;
    device->csd.structure = UNSTUFF_BITS(csd, 126, 2);

    switch (device->csd.structure) {
        case 0:
            m = UNSTUFF_BITS(csd, 115, 4);
            e = UNSTUFF_BITS(csd, 112, 3);
            device->csd.taac_ns = (taac_exp[e] * taac_mant[m] + 9) / 10;
            device->csd.taac_clks = UNSTUFF_BITS(csd, 104, 8) * 100;

            m = UNSTUFF_BITS(csd, 99, 4);
            e = UNSTUFF_BITS(csd, 96, 3);
            device->csd.max_dtr = tran_exp[e] * tran_mant[m];
            device->csd.cmdclass = UNSTUFF_BITS(csd, 84, 12);

            e = UNSTUFF_BITS(csd, 47, 3);
            m = UNSTUFF_BITS(csd, 62, 12);
            device->csd.capacity = ((1 + m) << (e + 2));

            device->csd.read_blkbits = UNSTUFF_BITS(csd, 80, 4);
            device->csd.read_partial = UNSTUFF_BITS(csd, 79, 1);
            device->csd.write_misalign = UNSTUFF_BITS(csd, 78, 1);
            device->csd.read_misalign = UNSTUFF_BITS(csd, 77, 1);
            device->csd.dsr_imp = UNSTUFF_BITS(csd, 76, 1);
            device->csd.r2w_factor = UNSTUFF_BITS(csd, 26, 3);
            device->csd.write_blkbits = UNSTUFF_BITS(csd, 22, 4);
            device->csd.write_partial = UNSTUFF_BITS(csd, 21, 1);

            if (UNSTUFF_BITS(csd, 46, 1)) {
                device->csd.erase_size = 1;
            } else if (device->csd.write_blkbits >= 9) {
                device->csd.erase_size = UNSTUFF_BITS(csd, 39, 7) + 1;
                device->csd.erase_size <<= (device->csd.write_blkbits - 9);
            }
            break;
        case 1:
            device->csd.taac_ns = 0; /* Unused */
            device->csd.taac_clks = 0; /* Unused */

            m = UNSTUFF_BITS(csd, 99, 4);
            e = UNSTUFF_BITS(csd, 96, 3);
            device->csd.max_dtr = tran_exp[e] * tran_mant[m];
            device->csd.cmdclass = UNSTUFF_BITS(csd, 84, 12);
            device->csd.c_size = UNSTUFF_BITS(csd, 48, 22);

            m = UNSTUFF_BITS(csd, 48, 22);
            device->csd.capacity = ((1 + m) << 10);

            device->csd.read_blkbits = 9;
            device->csd.read_partial = 0;
            device->csd.write_misalign = 0;
            device->csd.read_misalign = 0;
            device->csd.r2w_factor = 4; /* Unused */
            device->csd.write_blkbits = 9;
            device->csd.write_partial = 0;
            device->csd.erase_size = 1;
            break;
        default:
            return 0;
    }

    return 1;
}

static int sdmmc_sd_decode_scr(sdmmc_device_t *device, uint8_t *scr) {
    uint8_t tmp[8];
    uint32_t resp[4];

    /* This must be swapped. */
    for (int i = 0; i < 8; i += 4) {
        tmp[i + 3] = scr[i];
        tmp[i + 2] = scr[i + 1];
        tmp[i + 1] = scr[i + 2];
        tmp[i]     = scr[i + 3];
    }

    resp[3] = *(uint32_t *)&tmp[4];
    resp[2] = *(uint32_t *)&tmp[0];

    device->scr.sda_vsn = UNSTUFF_BITS(resp, 56, 4);
    device->scr.bus_widths = UNSTUFF_BITS(resp, 48, 4);

    /* Check if Physical Layer Spec v3.0 is supported. */
    if (device->scr.sda_vsn == SD_SCR_SPEC_VER_2) {
        device->scr.sda_spec3 = UNSTUFF_BITS(resp, 47, 1);
    }
    if (device->scr.sda_spec3) {
        device->scr.cmds = UNSTUFF_BITS(resp, 32, 2);
    }
    
    /* Unknown SCR structure version. */
    if (UNSTUFF_BITS(resp, 60, 4)) {
        return 0;
    } else {
        return 1;
    }
}

static void sdmmc_sd_decode_ssr(sdmmc_device_t *device, uint8_t *ssr) {
    uint8_t tmp[64];
    uint32_t resp1[4];
    uint32_t resp2[4];

    /* This must be swapped. */
    for (int i = 0; i < 64; i += 4) {
        tmp[i + 3] = ssr[i];
        tmp[i + 2] = ssr[i + 1];
        tmp[i + 1] = ssr[i + 2];
        tmp[i]     = ssr[i + 3];
    }

    resp1[3] = *(uint32_t *)&tmp[12];
    resp1[2] = *(uint32_t *)&tmp[8];
    resp1[1] = *(uint32_t *)&tmp[4];
    resp1[0] = *(uint32_t *)&tmp[0];
    resp2[3] = *(uint32_t *)&tmp[28];
    resp2[2] = *(uint32_t *)&tmp[24];
    resp2[1] = *(uint32_t *)&tmp[20];
    resp2[0] = *(uint32_t *)&tmp[16];

    device->ssr.dat_bus_width = ((UNSTUFF_BITS(resp1, 126, 2) & SD_BUS_WIDTH_4) ? 4 : 1);
    device->ssr.speed_class = UNSTUFF_BITS(resp1, 56, 8);

    if (device->ssr.speed_class < 4) {
        device->ssr.speed_class <<= 1;
    } else if (device->ssr.speed_class == 4) {
        device->ssr.speed_class = 10;
    }

    device->ssr.uhs_speed_grade = UNSTUFF_BITS(resp1, 12, 4);
    device->ssr.video_speed_class = UNSTUFF_BITS(resp1, 0, 8);
    device->ssr.app_perf_class = UNSTUFF_BITS(resp2, 80, 4);
}

static int sdmmc_sd_send_app_cmd(sdmmc_device_t *device, sdmmc_command_t *cmd, sdmmc_request_t *req, uint32_t resp_mask, uint32_t resp_state) {
    /* Try to send the APP command. */
    if (!sdmmc_device_send_r1_cmd(device, MMC_APP_CMD, (device->rca << 16), false, resp_mask, resp_state)) {
        return 0;
    }

    /* Send the actual command. */
    if (!sdmmc_send_cmd(device->sdmmc, cmd, req, 0)) {
        return 0;
    }

    return 1;
}

static int sdmmc_sd_send_if_cond(sdmmc_device_t *device, bool *is_sd_ver2) {
    sdmmc_command_t cmd = {};

    cmd.opcode = SD_SEND_IF_COND;
    /* We set the bit if the host supports voltages between 2.7 and 3.6 V */
    cmd.arg = 0x1AA;
    cmd.flags = SDMMC_RSP_R7;

    /* Command failed, this means SD Card is not version 2. */
    if (!sdmmc_send_cmd(device->sdmmc, &cmd, 0, 0)) {
        *is_sd_ver2 = false;
        return 1;
    }

    uint32_t resp = 0;

    /* Try to load back the response. */
    if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R7, &resp)) {
        return 0;
    }

    /* Check if we got a valid response. */
    if ((resp & 0xFF) == 0xAA) {
        *is_sd_ver2 = true;
        return 1;
    }

    return 0;
}

static int sdmmc_sd_send_op_cond(sdmmc_device_t *device, bool is_sd_ver2, bool is_uhs_en) {
    sdmmc_command_t cmd = {};

    /* Program a large timeout. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    while (!is_timeout) {
        /* Set this since most cards do not answer if some reserved bits in the OCR are set. */
        uint32_t arg = SD_OCR_VDD_32_33;

        /* Request support for SDXC power control and SDHC block mode cards. */
        if (is_sd_ver2) {
            arg |= SD_OCR_XPC;
            arg |= SD_OCR_CCS;
        }

        /* Request support 1.8V switching. */
        if (is_uhs_en) {
            arg |= SD_OCR_S18R;
        }
        
        cmd.opcode = SD_APP_OP_COND;
        cmd.arg = arg;
        cmd.flags = SDMMC_RSP_R3;

        /* Try to send the command. */
        if (!sdmmc_sd_send_app_cmd(device, &cmd, 0, is_sd_ver2 ? 0 : 0x400000, 0xFFFFFFFF)) {
            return 0;
        }

        uint32_t resp = 0;

        /* Try to load back the response. */
        if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R3, &resp)) {
            return 0;
        }

        /* Card Power up bit is set. */
        if (resp & MMC_CARD_BUSY) {
            /* We have a SDHC block mode card. */
            if (resp & SD_OCR_CCS) {
                device->is_block_sdhc = true;
            }
            /* We asked for low voltage support and the card accepted. */
            if (is_uhs_en && (resp & SD_ROCR_S18A)) {
                /* Voltage switching is only valid for SDMMC1. */
                if (device->sdmmc->controller == SDMMC_1) {
                    /* Failed to issue voltage switching command. */
                    if (!sdmmc_device_send_r1_cmd(device, SD_SWITCH_VOLTAGE, 0, false, 0, R1_STATE_READY)) {
                        return 0;
                    }

                    /* Delay a bit before asking for the voltage switch. */
                    mdelay(100);

                    /* Tell the driver to switch the voltage. */
                    if (!sdmmc_switch_voltage(device->sdmmc)) {
                        return 0;
                    }

                    /* We are now running at 1.8V. */
                    device->is_180v = true;
                }
            }

            return 1;
        }

        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 2000000);

        /* Delay for a minimum of 10 milliseconds. */
        mdelay(10);
    }

    return 0;
}

static int sdmmc_sd_send_relative_addr(sdmmc_device_t *device) {
    sdmmc_command_t cmd = {};

    cmd.opcode = SD_SEND_RELATIVE_ADDR;
    cmd.arg = 0;
    cmd.flags = SDMMC_RSP_R6;

    /* Program a large timeout. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    while (!is_timeout) {
        /* Try to send the command. */
        if (!sdmmc_send_cmd(device->sdmmc, &cmd, 0, 0)) {
            return 0;
        }

        uint32_t resp = 0;

        /* Try to load back the response. */
        if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R6, &resp)) {
            return 0;
        }

        /* Save the RCA. */
        if (resp >> 16) {
            device->rca = (resp >> 16);
            return 1;
        }

        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 2000000);

        /* Delay for an appropriate period. */
        udelay(1000);
    }

    return 0;
}

static int sdmmc_sd_send_scr(sdmmc_device_t *device, uint8_t *scr) {
    sdmmc_command_t cmd = {};
    sdmmc_request_t req = {};

    cmd.opcode = SD_APP_SEND_SCR;
    cmd.arg = 0;
    cmd.flags = SDMMC_RSP_R1;

    req.data = scr;
    req.blksz = 8;
    req.num_blocks = 1;
    req.is_read = true;
    req.is_multi_block = false;
    req.is_auto_cmd12 = false;

    /* Try to send the APP command. */
    if (!sdmmc_sd_send_app_cmd(device, &cmd, &req, 0, R1_STATE_TRAN)) {
        return 0;
    }

    uint32_t resp = 0;

    /* Try to load back the response. */
    if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R1, &resp)) {
        return 0;
    }

    /* Evaluate the response. */
    if (is_sdmmc_device_r1_error(resp)) {
        return 0;
    } else {
        return 1;
    }
}

static int sdmmc_sd_set_clr_card_detect(sdmmc_device_t *device) {
    /* Try to send the APP command. */
    if (!sdmmc_device_send_r1_cmd(device, MMC_APP_CMD, (device->rca << 16), false, 0, R1_STATE_TRAN)) {
        return 0;
    }

    /* Try to send the command. */
    return sdmmc_device_send_r1_cmd(device, SD_APP_SET_CLR_CARD_DETECT, 0, false, 0, R1_STATE_TRAN);
}

static int sdmmc_sd_set_bus_width(sdmmc_device_t *device) {
    /* Try to send the APP command. */
    if (!sdmmc_device_send_r1_cmd(device, MMC_APP_CMD, (device->rca << 16), false, 0, R1_STATE_TRAN)) {
        return 0;
    }

    /* Try to send the command. */
    return sdmmc_device_send_r1_cmd(device, SD_APP_SET_BUS_WIDTH, SD_BUS_WIDTH_4, false, 0, R1_STATE_TRAN);
}

static int sdmmc_sd_switch(sdmmc_device_t *device, uint32_t mode, uint32_t group, uint8_t value, uint8_t *data) {
    sdmmc_command_t cmd = {};
    sdmmc_request_t req = {};

    cmd.opcode = SD_SWITCH;
    cmd.arg = ((mode << 31) | 0x00FFFFFF);
    cmd.arg &= ~(0xF << (group * 4));
    cmd.arg |= (value << (group * 4));
    cmd.flags = SDMMC_RSP_R1;

    req.data = data;
    req.blksz = 64;
    req.num_blocks = 1;
    req.is_read = true;
    req.is_multi_block = false;
    req.is_auto_cmd12 = false;

    /* Try to send the command. */
    if (!sdmmc_send_cmd(device->sdmmc, &cmd, &req, 0)) {
        return 0;
    }

    uint32_t resp = 0;

    /* Try to load back the response. */
    if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R1, &resp)) {
        return 0;
    }

    /* Evaluate the response. */
    if (is_sdmmc_device_r1_error(resp)) {
        return 0;
    } else {
        return 1;
    }
}

static int sdmmc_sd_set_current_limit(sdmmc_device_t *device, uint8_t *status) {
    /* Start with the highest possible limit. */
    int32_t current_limit = SD_SET_CURRENT_LIMIT_800;

    /* Try each limit. */
    while (current_limit > SD_SET_CURRENT_NO_CHANGE) {
        /* Switch the current limit. */
        if (!sdmmc_sd_switch(device, SD_SWITCH_SET, 3, current_limit, status)) {
            return 0;
        }

        /* Current limit was set successfully. */
        if (((status[15] >> 4) & 0x0F) == current_limit) {
            break;
        }

        current_limit--;
    }

    return 1;
}

static int sdmmc_sd_switch_hs(sdmmc_device_t *device, uint32_t type, uint8_t *status) {
    /* Test if the card supports high-speed mode. */
    if (!sdmmc_sd_switch(device, 0, 0, type, status)) {
        return 0;
    }

    uint32_t res_type = (status[16] & 0xF);

    /* This high-speed mode type is not supported. */
    if (res_type != type) {
        return 0;
    }

    if ((((uint16_t)status[0] << 8) | status[1]) < 0x320) {
        /* Try to switch to high-speed mode. */
        if (!sdmmc_sd_switch(device, 1, 0, type, status)) {
            return 0;
        }

        /* Something failed when switching to high-speed mode. */
        if ((status[16] & 0xF) != res_type) {
            return 0;
        }
    }

    return 1;
}

static int sdmmc_sd_switch_hs_low(sdmmc_device_t *device, uint8_t *status) {
    /* Adjust the current limit. */
    if (!sdmmc_sd_set_current_limit(device, status)) {
        return 0;
    }

    /* Invalid bus width. */
    if (device->sdmmc->bus_width != SDMMC_BUS_WIDTH_4BIT) {
        return 0;
    }

    /* Get the supported high-speed type. */
    if (!sdmmc_sd_switch(device, 0, 0, 0xF, status)) {
        return 0;
    }

    /* High-speed SDR104 is supported. */
    if (status[13] & SD_MODE_UHS_SDR104) {
        /* Switch to high-speed. */
        if (!sdmmc_sd_switch_hs(device, UHS_SDR104_BUS_SPEED, status)) {
            return 0;
        }

        /* Reconfigure the internal clock. */
        if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_SD_SDR104)) {
            return 0;
        }

        /* Run tuning. */
        if (!sdmmc_execute_tuning(device->sdmmc, SDMMC_SPEED_SD_SDR104, MMC_SEND_TUNING_BLOCK)) {
            return 0;
        }
    } else if (status[13] & SD_MODE_UHS_SDR50) {    /* High-speed SDR50 is supported. */
        /* Switch to high-speed. */
        if (!sdmmc_sd_switch_hs(device, UHS_SDR50_BUS_SPEED, status)) {
            return 0;
        }

        /* Reconfigure the internal clock. */
        if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_SD_SDR50)) {
            return 0;
        }

        /* Run tuning. */
        if (!sdmmc_execute_tuning(device->sdmmc, SDMMC_SPEED_SD_SDR50, MMC_SEND_TUNING_BLOCK)) {
            return 0;
        }
    } else if (status[13] & SD_MODE_UHS_SDR12) {    /* High-speed SDR12 is supported. */
        /* Switch to high-speed. */
        if (!sdmmc_sd_switch_hs(device, UHS_SDR12_BUS_SPEED, status)) {
            return 0;
        }

        /* Reconfigure the internal clock. */
        if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_SD_SDR12)) {
            return 0;
        }

        /* Run tuning. */
        if (!sdmmc_execute_tuning(device->sdmmc, SDMMC_SPEED_SD_SDR12, MMC_SEND_TUNING_BLOCK)) {
            return 0;
        }
    } else {
        return 0;
    }

    /* Peek the SD card's status. */
    return sdmmc_device_send_status(device);
}

static int sdmmc_sd_switch_hs_high(sdmmc_device_t *device, uint8_t *status) {
    /* Get the supported high-speed type. */
    if (!sdmmc_sd_switch(device, 0, 0, 0xF, status)) {
        return 0;
    }

    /* High-speed is supported. */
    if (status[13] & 2) {
        /* Switch to high-speed. */
        if (!sdmmc_sd_switch_hs(device, SDHCI_CTRL_UHS_SDR25, status)) {
            return 0;
        }

        /* Reconfigure the internal clock. */
        if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_SD_SDR25)) {
            return 0;
        }

        /* Peek the SD card's status. */
        return sdmmc_device_send_status(device);
    }

    /* Nothing to do. */
    return 1;
}

static int sdmmc_sd_status(sdmmc_device_t *device, uint8_t *ssr) {
    sdmmc_command_t cmd = {};
    sdmmc_request_t req = {};

    cmd.opcode = SD_APP_SD_STATUS;
    cmd.arg = 0;
    cmd.flags = SDMMC_RSP_R1;

    req.data = ssr;
    req.blksz = 64;
    req.num_blocks = 1;
    req.is_read = true;
    req.is_multi_block = false;
    req.is_auto_cmd12 = false;

    /* Try to send the APP command. */
    if (!sdmmc_sd_send_app_cmd(device, &cmd, &req, 0, R1_STATE_TRAN)) {
        return 0;
    }

    uint32_t resp = 0;

    /* Try to load back the response. */
    if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R1, &resp)) {
        return 0;
    }

    /* Evaluate the response. */
    if (is_sdmmc_device_r1_error(resp)) {
        return 0;
    } else {
        return 1;
    }
}

int sdmmc_device_sd_init(sdmmc_device_t *device, sdmmc_t *sdmmc, SdmmcBusWidth bus_width, SdmmcBusSpeed bus_speed) {
    bool is_sd_ver2 = false;
    uint32_t cid[4] = {0};
    uint32_t csd[4] = {0};
    uint8_t scr[8] = {0};
    uint8_t ssr[64] = {0};
    uint8_t switch_status[512] = {0};

    /* Initialize our device's struct. */
    memset(device, 0, sizeof(sdmmc_device_t));

    /* Try to initialize the driver. */
    if (!sdmmc_init(sdmmc, SDMMC_1, SDMMC_VOLTAGE_3V3, SDMMC_BUS_WIDTH_1BIT, SDMMC_SPEED_SD_IDENT)) {
        sdmmc_error(sdmmc, "Failed to initialize the SDMMC driver!");
        return 0;
    }

    /* Bind the underlying driver. */
    device->sdmmc = sdmmc;

    sdmmc_info(sdmmc, "SDMMC driver was successfully initialized for SD!");

    /* Apply at least 74 clock cycles. The card should be ready afterwards. */
    udelay((74000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);

    /* Instruct the SD card to go idle. */
    if (!sdmmc_device_go_idle(device)) {
        sdmmc_error(sdmmc, "Failed to go idle!");
        return 0;
    }

    sdmmc_info(sdmmc, "SD card went idle!");

    /* Get the SD card's interface operating condition. */
    if (!sdmmc_sd_send_if_cond(device, &is_sd_ver2)) {
        sdmmc_error(sdmmc, "Failed to send if cond!");
        return 0;
    }

    sdmmc_info(sdmmc, "Sent if cond to SD card!");

    /* Get the SD card's operating conditions. */
    if (!sdmmc_sd_send_op_cond(device, is_sd_ver2, (bus_width == SDMMC_BUS_WIDTH_4BIT) && (bus_speed == SDMMC_SPEED_SD_SDR104))) {
        sdmmc_error(sdmmc, "Failed to send op cond!");
        return 0;
    }

    sdmmc_info(sdmmc, "Sent op cond to SD card!");

    /* Get the SD card's CID. */
    if (!sdmmc_device_send_cid(device, cid)) {
        sdmmc_error(sdmmc, "Failed to get CID!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got CID from SD card!");

    /* Decode and save the CID. */
    sdmmc_sd_decode_cid(device, cid);

    /* Get the SD card's RCA. */
    if (!sdmmc_sd_send_relative_addr(device)) {
        sdmmc_error(sdmmc, "Failed to get RCA!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got RCA (0x%08x) from SD card!", device->rca);

    /* Get the SD card's CSD. */
    if (!sdmmc_device_send_csd(device, csd)) {
        sdmmc_error(sdmmc, "Failed to get CSD!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got CSD from SD card!");

    /* Decode and save the CSD. */
    if (!sdmmc_sd_decode_csd(device, csd)) {
        sdmmc_warn(sdmmc, "Got unknown CSD structure (0x%08x)!", device->csd.structure);
    }
    
    /* If we never switched to 1.8V, change the bus speed mode. */
    if (!device->is_180v) {
        /* Reconfigure the internal clock. */
        if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_SD_DS)) {
            sdmmc_error(sdmmc, "Failed to apply the correct bus speed!");
            return 0;
        }

        sdmmc_info(sdmmc, "Speed mode has been adjusted!");
    }

    /* Select the SD card. */
    if (!sdmmc_device_select_card(device)) {
        sdmmc_error(sdmmc, "Failed to select SD card!");
        return 0;
    }

    sdmmc_info(sdmmc, "SD card is now selected!");

    /* Change the SD card's block length. */
    if (!sdmmc_device_set_blocklen(device, 512)) {
        sdmmc_error(sdmmc, "Failed to set SD card's block length!");
        return 0;
    }

    sdmmc_info(sdmmc, "SD card's block length is now 512!");

    /* It's a good practice to disconnect the pull-up resistor with ACMD42. */
    if (!sdmmc_sd_set_clr_card_detect(device)) {
        sdmmc_error(sdmmc, "Failed to disconnect the pull-up resistor!");
        return 0;
    }

    sdmmc_info(sdmmc, "Pull-up resistor is now disconnected!");

    /* Get the SD card's SCR. */
    if (!sdmmc_sd_send_scr(device, scr)) {
        sdmmc_error(sdmmc, "Failed to get SCR!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got SCR from SD card!");

    /* Decode and save the SCR. */
    if (!sdmmc_sd_decode_scr(device, scr)) {
        sdmmc_error(sdmmc, "Got unknown SCR structure!");
        return 0;
    }

    /* Switch to wider bus (if supported). */
    if ((bus_width == SDMMC_BUS_WIDTH_4BIT)
        && (device->scr.bus_widths & SD_SCR_BUS_WIDTH_4)
        && (device->scr.sda_vsn & (SD_SCR_SPEC_VER_1 | SD_SCR_SPEC_VER_2))) {
        if (!sdmmc_sd_set_bus_width(device)) {
            sdmmc_error(sdmmc, "Failed to switch to wider bus!");
            return 0;
        }

        sdmmc_select_bus_width(device->sdmmc, SDMMC_BUS_WIDTH_4BIT);
        sdmmc_info(sdmmc, "Switched to wider bus!");
    }

    if (device->is_180v) {
        /* Switch to high-speed from low voltage (if possible). */
        if (!sdmmc_sd_switch_hs_low(device, switch_status)) {
            sdmmc_error(sdmmc, "Failed to switch to high-speed from low voltage!");
            return 0;
        }

        sdmmc_info(sdmmc, "Switched to high-speed from low voltage!");
    } else if ((device->scr.sda_vsn & (SD_SCR_SPEC_VER_1 | SD_SCR_SPEC_VER_2)) && ((bus_speed != SDMMC_SPEED_SD_DS))) {
        /* Switch to high-speed from high voltage (if possible). */
        if (!sdmmc_sd_switch_hs_high(device, switch_status)) {
            sdmmc_error(sdmmc, "Failed to switch to high-speed from high voltage!");
            return 0;
        }

        sdmmc_info(sdmmc, "Switched to high-speed from high voltage!");
    }

    /* Correct any inconsistent states. */
    sdmmc_adjust_sd_clock(sdmmc);

    /* Get the SD card's SSR. */
    if (!sdmmc_sd_status(device, ssr)) {
        sdmmc_error(sdmmc, "Failed to get SSR!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got SSR from SD card!");

    /* Decode and save the SSR. */
    sdmmc_sd_decode_ssr(device, scr);

    return 1;
}

/*
    MMC device functions.
*/

static void sdmmc_mmc_decode_cid(sdmmc_device_t *device, uint32_t *cid) {
    switch (device->csd.mmca_vsn) {
        case 0: /* MMC v1.0 - v1.2 */
        case 1: /* MMC v1.4 */
            device->cid.prod_name[6] = UNSTUFF_BITS(cid, 48, 8);
            device->cid.manfid = UNSTUFF_BITS(cid, 104, 24);
            device->cid.hwrev  = UNSTUFF_BITS(cid, 44, 4);
            device->cid.fwrev  = UNSTUFF_BITS(cid, 40, 4);
            device->cid.serial = UNSTUFF_BITS(cid, 16, 24);
            break;
        case 2: /* MMC v2.0 - v2.2 */
        case 3: /* MMC v3.1 - v3.3 */
        case 4: /* MMC v4 */
            device->cid.manfid   = UNSTUFF_BITS(cid, 120, 8);
            device->cid.oemid    = UNSTUFF_BITS(cid, 104, 8);
            device->cid.prv      = UNSTUFF_BITS(cid, 48, 8);
            device->cid.serial   = UNSTUFF_BITS(cid, 16, 32);
            break;
        default:
            break;
    }

    device->cid.prod_name[0] = UNSTUFF_BITS(cid, 96, 8);
    device->cid.prod_name[1] = UNSTUFF_BITS(cid, 88, 8);
    device->cid.prod_name[2] = UNSTUFF_BITS(cid, 80, 8);
    device->cid.prod_name[3] = UNSTUFF_BITS(cid, 72, 8);
    device->cid.prod_name[4] = UNSTUFF_BITS(cid, 64, 8);
    device->cid.prod_name[5] = UNSTUFF_BITS(cid, 56, 8);

    device->cid.month = UNSTUFF_BITS(cid, 12, 4);
    device->cid.year  = (UNSTUFF_BITS(cid, 8, 4) + 1997);

    if ((device->ext_csd.rev >= 5) && (device->cid.year < 2010)) {
        device->cid.year += 16;
    }
}

static int sdmmc_mmc_decode_csd(sdmmc_device_t *device, uint32_t *csd) {
    unsigned int e, m, a, b;

    device->csd.structure = UNSTUFF_BITS(csd, 126, 2);

    if (!device->csd.structure) {
        return 0;
    }

    device->csd.mmca_vsn = UNSTUFF_BITS(csd, 122, 4);

    m = UNSTUFF_BITS(csd, 115, 4);
    e = UNSTUFF_BITS(csd, 112, 3);
    device->csd.taac_ns = ((taac_exp[e] * taac_mant[m] + 9) / 10);
    device->csd.taac_clks = (UNSTUFF_BITS(csd, 104, 8) * 100);

    m = UNSTUFF_BITS(csd, 99, 4);
    e = UNSTUFF_BITS(csd, 96, 3);
    device->csd.max_dtr = (tran_exp[e] * tran_mant[m]);
    device->csd.cmdclass = UNSTUFF_BITS(csd, 84, 12);

    e = UNSTUFF_BITS(csd, 47, 3);
    m = UNSTUFF_BITS(csd, 62, 12);
    device->csd.capacity = ((1 + m) << (e + 2));

    device->csd.read_blkbits = UNSTUFF_BITS(csd, 80, 4);
    device->csd.read_partial = UNSTUFF_BITS(csd, 79, 1);
    device->csd.write_misalign = UNSTUFF_BITS(csd, 78, 1);
    device->csd.read_misalign = UNSTUFF_BITS(csd, 77, 1);
    device->csd.dsr_imp = UNSTUFF_BITS(csd, 76, 1);
    device->csd.r2w_factor = UNSTUFF_BITS(csd, 26, 3);
    device->csd.write_blkbits = UNSTUFF_BITS(csd, 22, 4);
    device->csd.write_partial = UNSTUFF_BITS(csd, 21, 1);

    if (device->csd.write_blkbits >= 9) {
        a = UNSTUFF_BITS(csd, 42, 5);
        b = UNSTUFF_BITS(csd, 37, 5);
        device->csd.erase_size = ((a + 1) * (b + 1));
        device->csd.erase_size <<= (device->csd.write_blkbits - 9);
    }

    return 1;
}

static void sdmmc_mmc_decode_ext_csd(sdmmc_device_t *device, uint8_t *ext_csd) {
    device->ext_csd.rev = ext_csd[EXT_CSD_REV];
    device->ext_csd.raw_ext_csd_structure = ext_csd[EXT_CSD_STRUCTURE];
    device->ext_csd.raw_card_type = ext_csd[EXT_CSD_CARD_TYPE];
    device->ext_csd.raw_rpmb_size_mult = ext_csd[EXT_CSD_RPMB_MULT];
    device->ext_csd.raw_sectors[0] = ext_csd[EXT_CSD_SEC_CNT + 0];
    device->ext_csd.raw_sectors[1] = ext_csd[EXT_CSD_SEC_CNT + 1];
    device->ext_csd.raw_sectors[2] = ext_csd[EXT_CSD_SEC_CNT + 2];
    device->ext_csd.raw_sectors[3] = ext_csd[EXT_CSD_SEC_CNT + 3];
    device->ext_csd.bkops = ext_csd[EXT_CSD_BKOPS_SUPPORT];
    device->ext_csd.man_bkops_en = ext_csd[EXT_CSD_BKOPS_EN];
    device->ext_csd.raw_bkops_status = ext_csd[EXT_CSD_BKOPS_STATUS];
}

static int sdmmc_mmc_send_op_cond(sdmmc_device_t *device, SdmmcBusVoltage bus_voltage) {
    sdmmc_command_t cmd = {};

    /* Program a large timeout. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    while (!is_timeout) {
        /* Set high capacity bit. */
        uint32_t arg = SD_OCR_CCS;

        /* Set voltage bits. */
        if (bus_voltage == SDMMC_VOLTAGE_1V8) {
            arg |= MMC_VDD_165_195;
        } else if (bus_voltage == SDMMC_VOLTAGE_3V3) {
            arg |= (MMC_VDD_33_34 | MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_30_31 | MMC_VDD_29_30 | MMC_VDD_28_29 | MMC_VDD_27_28);
        } else {
            return 0;
        }

        cmd.opcode = MMC_SEND_OP_COND;
        cmd.arg = arg;
        cmd.flags = SDMMC_RSP_R3;

        /* Try to send the command. */
        if (!sdmmc_send_cmd(device->sdmmc, &cmd, 0, 0)) {
            return 0;
        }

        uint32_t resp = 0;

        /* Try to load back the response. */
        if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R3, &resp)) {
            return 0;
        }

        /* Card Power up bit is set. */
        if (resp & MMC_CARD_BUSY) {
            /* We have a SDHC block mode card. */
            if (resp & SD_OCR_CCS) {
                device->is_block_sdhc = true;
            }
            
            return 1;
        }

        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 2000000);

        /* Delay for a minimum of 10 milliseconds. */
        mdelay(10);
    }

    return 0;
}

static int sdmmc_mmc_send_ext_csd(sdmmc_device_t *device, uint8_t *ext_csd) {
    sdmmc_command_t cmd = {};
    sdmmc_request_t req = {};

    cmd.opcode = MMC_SEND_EXT_CSD;
    cmd.arg = 0;
    cmd.flags = SDMMC_RSP_R1;

    req.data = ext_csd;
    req.blksz = 512;
    req.num_blocks = 1;
    req.is_read = true;
    req.is_multi_block = false;
    req.is_auto_cmd12 = false;

    /* Try to send the command. */
    if (!sdmmc_send_cmd(device->sdmmc, &cmd, &req, 0)) {
        return 0;
    }

    uint32_t resp = 0;

    /* Try to load back the response. */
    if (!sdmmc_load_response(device->sdmmc, SDMMC_RSP_R1, &resp)) {
        return 0;
    }

    /* Evaluate the response. */
    if (is_sdmmc_device_r1_error(resp)) {
        return 0;
    } else {
        return 1;
    }
}

static int sdmmc_mmc_set_relative_addr(sdmmc_device_t *device) {
    /* Try to send the command. */
    return sdmmc_device_send_r1_cmd(device, MMC_SET_RELATIVE_ADDR, (device->rca << 16), false, 0, 0xFFFFFFFF);
}

static int sdmmc_mmc_switch(sdmmc_device_t *device, uint32_t arg) {
    /* Try to send the command. */
    return sdmmc_device_send_r1_cmd(device, MMC_SWITCH, arg, true, 0, 0xFFFFFFFF);
}

static int sdmmc_mmc_select_bus_width(sdmmc_device_t *device, SdmmcBusWidth bus_width) {
    uint32_t arg = 0;

    /* Choose the argument for the switch command. */
    switch (bus_width) {
        case SDMMC_BUS_WIDTH_1BIT:
            return 1;
        case SDMMC_BUS_WIDTH_4BIT:
            arg = (((MMC_SWITCH_MODE_WRITE_BYTE) << 24) | ((EXT_CSD_BUS_WIDTH) << 16) | ((EXT_CSD_BUS_WIDTH_4) << 8));
            break;
        case SDMMC_BUS_WIDTH_8BIT:
            arg = (((MMC_SWITCH_MODE_WRITE_BYTE) << 24) | ((EXT_CSD_BUS_WIDTH) << 16) | ((EXT_CSD_BUS_WIDTH_8) << 8));
            break;
        default:
            return 0;
    }

    /* Try to switch the bus width. */
    if (sdmmc_mmc_switch(device, arg) && sdmmc_device_send_status(device)) {
        sdmmc_select_bus_width(device->sdmmc, bus_width);
        return 1;
    }

    return 0;
}

static int sdmmc_mmc_select_hs(sdmmc_device_t *device, bool ignore_status) {
    uint32_t arg = (((MMC_SWITCH_MODE_WRITE_BYTE) << 24) | ((EXT_CSD_HS_TIMING) << 16) | ((EXT_CSD_TIMING_HS) << 8));

    /* Try to switch to HS. */
    if (!sdmmc_mmc_switch(device, arg)) {
        return 0;
    }

    /* Check the status if necessary. */
    if (!ignore_status && !sdmmc_device_send_status(device)) {
        return 0;
    }

    /* Reconfigure the internal clock. */
    if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_MMC_HS)) {
        return 0;
    }

    /* Check the status if necessary. */
    if (!ignore_status && !sdmmc_device_send_status(device)) {
        return 0;
    }

    return 1;
}

static int sdmmc_mmc_select_hs200(sdmmc_device_t *device) {
    uint32_t arg = (((MMC_SWITCH_MODE_WRITE_BYTE) << 24) | ((EXT_CSD_HS_TIMING) << 16) | ((EXT_CSD_TIMING_HS200) << 8));

    /* Try to switch to HS200. */
    if (!sdmmc_mmc_switch(device, arg)) {
        return 0;
    }

    /* Reconfigure the internal clock. */
    if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_MMC_HS200)) {
        return 0;
    }

    /* Execute tuning procedure. */
    if (!sdmmc_execute_tuning(device->sdmmc, SDMMC_SPEED_MMC_HS200, MMC_SEND_TUNING_BLOCK_HS200)) {
        return 0;
    }

    /* Peek the current status. */
    return sdmmc_device_send_status(device);
}

static int sdmmc_mmc_select_hs400(sdmmc_device_t *device) {
    uint32_t arg = 0;

    /* Switch to HS200 first. */
    if (!sdmmc_mmc_select_hs200(device)) {
        return 0;
    }

    /* Fetch and set the tuning tap value. */
    sdmmc_set_tuning_tap_val(device->sdmmc);

    /* Switch to HS. */
    if (!sdmmc_mmc_select_hs(device, true)) {
        return 0;
    }

    arg = (((MMC_SWITCH_MODE_WRITE_BYTE) << 24) | ((EXT_CSD_BUS_WIDTH) << 16) | ((EXT_CSD_DDR_BUS_WIDTH_8) << 8));

    /* Try to switch to 8bit bus. */
    if (!sdmmc_mmc_switch(device, arg)) {
        return 0;
    }

    arg = (((MMC_SWITCH_MODE_WRITE_BYTE) << 24) | ((EXT_CSD_HS_TIMING) << 16) | ((EXT_CSD_TIMING_HS400) << 8));

    /* Try to switch to HS400. */
    if (!sdmmc_mmc_switch(device, arg)) {
        return 0;
    }

    /* Reconfigure the internal clock. */
    if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_MMC_HS400)) {
        return 0;
    }

    /* Peek the current status. */
    return sdmmc_device_send_status(device);
}

static int sdmmc_mmc_select_timing(sdmmc_device_t *device, SdmmcBusSpeed bus_speed) {
    if ((bus_speed == SDMMC_SPEED_MMC_HS400) &&
        (device->sdmmc->bus_width == SDMMC_BUS_WIDTH_8BIT) &&
        (device->ext_csd.raw_card_type & EXT_CSD_CARD_TYPE_HS400_1_8V)) {
        /* Switch to HS400. */
        return sdmmc_mmc_select_hs400(device);
    } else if (((bus_speed == SDMMC_SPEED_MMC_HS400) || (bus_speed == SDMMC_SPEED_MMC_HS200)) &&
        ((device->sdmmc->bus_width == SDMMC_BUS_WIDTH_8BIT) || (device->sdmmc->bus_width == SDMMC_BUS_WIDTH_4BIT)) &&
        (device->ext_csd.raw_card_type & EXT_CSD_CARD_TYPE_HS200_1_8V)) {
        /* Switch to HS200. */
        return sdmmc_mmc_select_hs200(device);
    } else if (device->ext_csd.raw_card_type & EXT_CSD_CARD_TYPE_HS_52) {
        /* Switch to HS. */
        return sdmmc_mmc_select_hs(device, false);
    }

    return 0;
}

static int sdmmc_mmc_select_bkops(sdmmc_device_t *device) {
    uint32_t arg = (((MMC_SWITCH_MODE_SET_BITS) << 24) | ((EXT_CSD_BKOPS_EN) << 16) | ((EXT_CSD_BKOPS_LEVEL_2) << 8));

    /* Try to enable bkops. */
    if (!sdmmc_mmc_switch(device, arg)) {
        return 0;
    }

    /* Peek the current status. */
    return sdmmc_device_send_status(device);
}

int sdmmc_mmc_select_partition(sdmmc_device_t *device, SdmmcPartitionNum partition) {
    uint32_t arg = (((MMC_SWITCH_MODE_WRITE_BYTE) << 24) | ((EXT_CSD_PART_CONFIG) << 16) | ((partition) << 8));

    /* Try to change the active partition. */
    if (!sdmmc_mmc_switch(device, arg)) {
        return 0;
    }

    /* Peek the current status. */
    return sdmmc_device_send_status(device);
}

int sdmmc_device_mmc_init(sdmmc_device_t *device, sdmmc_t *sdmmc, SdmmcBusWidth bus_width, SdmmcBusSpeed bus_speed) {
    uint32_t cid[4] = {0};
    uint32_t csd[4] = {0};
    uint8_t ext_csd[512] = {0};

    /* Initialize our device's struct. */
    memset(device, 0, sizeof(sdmmc_device_t));

    /* Try to initialize the driver. */
    if (!sdmmc_init(sdmmc, SDMMC_4, SDMMC_VOLTAGE_1V8, SDMMC_BUS_WIDTH_1BIT, SDMMC_SPEED_MMC_IDENT)) {
        sdmmc_error(sdmmc, "Failed to initialize the SDMMC driver!");
        return 0;
    }

    /* Bind the underlying driver. */
    device->sdmmc = sdmmc;

    /* Set RCA. */
    device->rca = 0x01;

    sdmmc_info(sdmmc, "SDMMC driver was successfully initialized for eMMC!");

    /* Apply at least 74 clock cycles. eMMC should be ready afterwards. */
    udelay((74000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);

    /* Instruct the eMMC to go idle. */
    if (!sdmmc_device_go_idle(device)) {
        sdmmc_error(sdmmc, "Failed to go idle!");
        return 0;
    }

    sdmmc_info(sdmmc, "eMMC went idle!");

    /* Get the eMMC's operating conditions. */
    if (!sdmmc_mmc_send_op_cond(device, SDMMC_VOLTAGE_1V8)) {
        sdmmc_error(sdmmc, "Failed to send op cond!");
        return 0;
    }

    sdmmc_info(sdmmc, "Sent op cond to eMMC!");

    /* Get the eMMC's CID. */
    if (!sdmmc_device_send_cid(device, cid)) {
        sdmmc_error(sdmmc, "Failed to get CID!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got CID from eMMC!");

    /* Set the eMMC's RCA. */
    if (!sdmmc_mmc_set_relative_addr(device)) {
        sdmmc_error(sdmmc, "Failed to set RCA!");
        return 0;
    }

    sdmmc_info(sdmmc, "RCA is now set in eMMC!");

    /* Get the eMMC card's CSD. */
    if (!sdmmc_device_send_csd(device, csd)) {
        sdmmc_error(sdmmc, "Failed to get CSD!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got CSD from eMMC!");

    /* Decode and save the CSD. */
    if (!sdmmc_mmc_decode_csd(device, csd)) {
        sdmmc_warn(sdmmc, "Got unknown CSD structure (0x%08x)!", device->csd.structure);
    }
    
    /* Reconfigure the internal clock. */
    if (!sdmmc_select_speed(device->sdmmc, SDMMC_SPEED_MMC_LEGACY)) {
        sdmmc_error(sdmmc, "Failed to apply the correct bus speed!");
        return 0;
    }

    sdmmc_info(sdmmc, "Speed mode has been adjusted!");

    /* Select the eMMC card. */
    if (!sdmmc_device_select_card(device)) {
        sdmmc_error(sdmmc, "Failed to select eMMC card!");
        return 0;
    }

    sdmmc_info(sdmmc, "eMMC card is now selected!");

    /* Change the eMMC's block length. */
    if (!sdmmc_device_set_blocklen(device, 512)) {
        sdmmc_error(sdmmc, "Failed to set eMMC's block length!");
        return 0;
    }

    sdmmc_info(sdmmc, "eMMC's block length is now 512!");

    /* Only specification version 4 and later support the next features. */
    if (device->csd.mmca_vsn < CSD_SPEC_VER_4) {
        return 1;
    }

    /* Change the eMMC's bus width. */
    if (!sdmmc_mmc_select_bus_width(device, bus_width)) {
        sdmmc_error(sdmmc, "Failed to set eMMC's bus width!");
        return 0;
    }

    sdmmc_info(sdmmc, "eMMC's bus width has been adjusted!");

    /* Get the eMMC's extended CSD. */
    if (!sdmmc_mmc_send_ext_csd(device, ext_csd)) {
        sdmmc_error(sdmmc, "Failed to get EXT_CSD!");
        return 0;
    }

    sdmmc_info(sdmmc, "Got EXT_CSD from eMMC!");

    /* Decode and save the extended CSD. */
    sdmmc_mmc_decode_ext_csd(device, ext_csd);

    /* Decode and save the CID. */
    sdmmc_mmc_decode_cid(device, cid);

    /* TODO: Handle automatic BKOPS properly. Leave it disabled for now. */
    if (false && device->ext_csd.bkops && !(device->ext_csd.auto_bkops_en & EXT_CSD_AUTO_BKOPS_MASK)) {
        sdmmc_mmc_select_bkops(device);
        sdmmc_info(sdmmc, "BKOPS is enabled!");
    } else {
        sdmmc_info(sdmmc, "BKOPS is disabled!");
    }
    
    /* Switch to high speed mode. */
    if (!sdmmc_mmc_select_timing(device, bus_speed)) {
        sdmmc_error(sdmmc, "Failed to switch to high speed mode!");
        return 0;
    }

    sdmmc_info(sdmmc, "Switched to high speed mode!");

    /* Correct any inconsistent states. */
    sdmmc_adjust_sd_clock(sdmmc);

    return 1;
}