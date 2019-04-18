/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
 
#include <stdint.h>

#include "memory_map.h"
#include "mc.h"
#include "exocfg.h"

typedef struct {
    uint64_t address;
    uint64_t size;
} saved_carveout_info_t;

static saved_carveout_info_t g_saved_carveouts[2] = {
    {0x80060000ull, KERNEL_CARVEOUT_SIZE_MAX},
    {0x00000000ull, 0x00000000ull}
};    

volatile security_carveout_t *get_carveout_by_id(unsigned int carveout) {
    if (CARVEOUT_ID_MIN <= carveout && carveout <= CARVEOUT_ID_MAX) {
         return (volatile security_carveout_t *)(MC_BASE + 0xC08ull + 0x50 * (carveout - CARVEOUT_ID_MIN));
    }
    generic_panic();
    return NULL;
}

void configure_gpu_ucode_carveout(void) {
    /* Starting in 6.0.0, Carveout 2 is configured later on and adds read permission to TSEC. */
    /* This is a helper function to make this easier... */
    volatile security_carveout_t *carveout = get_carveout_by_id(2);
    carveout->paddr_low = 0x80020000;
    carveout->paddr_high = 0;
    carveout->size_big_pages = 2;       /* 0x40000 */
    carveout->client_access_0 = 0;
    carveout->client_access_1 = 0;
    carveout->client_access_2 = (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_600) ? (BIT(CSR_GPUSRD) | BIT(CSW_GPUSWR) | BIT(CSR_TSECSRD)) : (BIT(CSR_GPUSRD) | BIT(CSW_GPUSWR));
    carveout->client_access_3 = 0;
    carveout->client_access_4 = (BIT(CSR_GPUSRD2) | BIT(CSW_GPUSWR2));
    carveout->client_force_internal_access_0 = 0;
    carveout->client_force_internal_access_1 = 0;
    carveout->client_force_internal_access_2 = 0;
    carveout->client_force_internal_access_3 = 0;
    carveout->client_force_internal_access_4 = 0;
    carveout->config = 0x440167E;
}

void configure_default_carveouts(void) {
    /* Configure Carveout 1 (UNUSED) */
    volatile security_carveout_t *carveout = get_carveout_by_id(1);
    carveout->paddr_low = 0;
    carveout->paddr_high = 0;
    carveout->size_big_pages = 0;
    carveout->client_access_0 = 0;
    carveout->client_access_1 = 0;
    carveout->client_access_2 = 0;
    carveout->client_access_3 = 0;
    carveout->client_access_4 = 0;
    carveout->client_force_internal_access_0 = 0;
    carveout->client_force_internal_access_1 = 0;
    carveout->client_force_internal_access_2 = 0;
    carveout->client_force_internal_access_3 = 0;
    carveout->client_force_internal_access_4 = 0;
    carveout->config = 0x4000006;

    /* Configure Carveout 2 (GPU UCODE) */
    if (exosphere_get_target_firmware() < ATMOSPHERE_TARGET_FIRMWARE_600) {
        configure_gpu_ucode_carveout();
    }

    /* Configure Carveout 3 (UNUSED GPU) */
    carveout = get_carveout_by_id(3);
    carveout->paddr_low = 0;
    carveout->paddr_high = 0;
    carveout->size_big_pages = 0;
    carveout->client_access_0 = 0;
    carveout->client_access_1 = 0;
    carveout->client_access_2 = (BIT(CSR_GPUSRD) | BIT(CSW_GPUSWR));
    carveout->client_access_3 = 0;
    carveout->client_access_4 = (BIT(CSR_GPUSRD2) | BIT(CSW_GPUSWR2));
    carveout->client_force_internal_access_0 = 0;
    carveout->client_force_internal_access_1 = 0;
    carveout->client_force_internal_access_2 = 0;
    carveout->client_force_internal_access_3 = 0;
    carveout->client_force_internal_access_4 = 0;
    carveout->config = 0x4401E7E;

    /* Configure default Kernel carveouts based on 2.0.0+. */
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_200) {
        /* Configure Carveout 4 (KERNEL_BUILTINS) */
        configure_kernel_carveout(4, g_saved_carveouts[0].address, g_saved_carveouts[0].size);

        /* Configure Carveout 5 (KERNEL_UNUSED) */
        configure_kernel_carveout(5, g_saved_carveouts[1].address, g_saved_carveouts[1].size);
    } else {
        for (unsigned int i = 4; i <= 5; i++) {
            carveout = get_carveout_by_id(i);
            carveout->paddr_low = 0;
            carveout->paddr_high = 0;
            carveout->size_big_pages = 0;
            carveout->client_access_0 = 0;
            carveout->client_access_1 = 0;
            carveout->client_access_2 = 0;
            carveout->client_access_3 = 0;
            carveout->client_access_4 = 0;
            carveout->client_force_internal_access_0 = 0;
            carveout->client_force_internal_access_1 = 0;
            carveout->client_force_internal_access_2 = 0;
            carveout->client_force_internal_access_3 = 0;
            carveout->client_force_internal_access_4 = 0;
            carveout->config = 0x4000006;
        }
    }
}

void configure_kernel_carveout(unsigned int carveout_id, uint64_t address, uint64_t size) {
    if (carveout_id != 4 && carveout_id != 5) {
        generic_panic();
    }
    
    g_saved_carveouts[carveout_id-4].address = address;
    g_saved_carveouts[carveout_id-4].size = size;

    volatile security_carveout_t *carveout = get_carveout_by_id(carveout_id);
    carveout->paddr_low = (uint32_t)(address & 0xFFFFFFFF);
    carveout->paddr_high = (uint32_t)(address >> 32);
    carveout->size_big_pages = (uint32_t)(size >> 17);
    carveout->client_access_0 = (BIT(CSR_PTCR) | BIT(CSR_DISPLAY0A) | BIT(CSR_DISPLAY0AB) | BIT(CSR_DISPLAY0B) | BIT(CSR_DISPLAY0BB) | BIT(CSR_DISPLAY0C) | BIT(CSR_DISPLAY0CB) | BIT(CSR_AFIR) | BIT(CSR_DISPLAYHC) | BIT(CSR_DISPLAYHCB) | BIT(CSR_HDAR) | BIT(CSR_HOST1XDMAR) | BIT(CSR_HOST1XR) | BIT(CSR_NVENCSRD) | BIT(CSR_PPCSAHBDMAR) | BIT(CSR_PPCSAHBSLVR));
    carveout->client_access_1 = (BIT(CSR_MPCORER) | BIT(CSW_NVENCSWR) | BIT(CSW_AFIW) | BIT(CSW_HDAW) | BIT(CSW_HOST1XW) | BIT(CSW_MPCOREW) | BIT(CSW_PPCSAHBDMAW) | BIT(CSW_PPCSAHBSLVW));
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_800) {
        carveout->client_access_2 = (BIT(CSR_XUSB_HOSTR) | BIT(CSW_XUSB_HOSTW) | BIT(CSR_XUSB_DEVR) | BIT(CSW_XUSB_DEVW)); 
        carveout->client_access_3 = (BIT(CSR_SDMMCRA) | BIT(CSR_SDMMCRAA) | BIT(CSR_SDMMCRAB) | BIT(CSW_SDMMCWA) | BIT(CSW_SDMMCWAA) | BIT(CSW_SDMMCWAB) | BIT(CSR_VICSRD) | BIT(CSW_VICSWR) | BIT(CSR_DISPLAYD) | BIT(CSR_NVDECSRD) | BIT(CSW_NVDECSWR) | BIT(CSR_APER) | BIT(CSW_APEW) | BIT(CSR_NVJPGSRD) | BIT(CSW_NVJPGSWR));
        carveout->client_access_4 = (BIT(CSR_SESRD) | BIT(CSW_SESWR) | BIT(CSR_TSECSRDB) | BIT(CSW_TSECSWRB));
    } else {
        carveout->client_access_2 = (BIT(CSR_XUSB_HOSTR) | BIT(CSW_XUSB_HOSTW) | BIT(CSR_XUSB_DEVR) | BIT(CSW_XUSB_DEVW) | BIT(CSR_TSECSRD) | BIT(CSW_TSECSWR));
        carveout->client_access_3 = (BIT(CSR_SDMMCRA) | BIT(CSR_SDMMCRAA) | BIT(CSR_SDMMCRAB) | BIT(CSW_SDMMCWA) | BIT(CSW_SDMMCWAA) | BIT(CSW_SDMMCWAB) | BIT(CSR_VICSRD) | BIT(CSW_VICSWR) | BIT(CSR_DISPLAYD) | BIT(CSR_NVDECSRD) | BIT(CSW_NVDECSWR) | BIT(CSR_APER) | BIT(CSW_APEW) | BIT(CSR_NVJPGSRD) | BIT(CSW_NVJPGSWR));
        carveout->client_access_4 = (BIT(CSR_SESRD) | BIT(CSW_SESWR));
    }
    carveout->client_force_internal_access_0 = ((exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_400) && (carveout_id == 4)) ? BIT(CSR_AVPCARM7R) : 0;
    carveout->client_force_internal_access_1 = ((exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_400) && (carveout_id == 4)) ? BIT(CSW_AVPCARM7W) : 0;
    carveout->client_force_internal_access_2 = 0;
    carveout->client_force_internal_access_3 = 0;
    carveout->client_force_internal_access_4 = 0;
    carveout->config = (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_800) ? 0x4CB : 0x8B;
}
