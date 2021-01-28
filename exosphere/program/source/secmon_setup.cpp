/*
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
#include <exosphere.hpp>
#include "secmon_setup.hpp"
#include "secmon_error.hpp"
#include "secmon_map.hpp"
#include "secmon_cpu_context.hpp"
#include "secmon_mariko_fatal_error.hpp"
#include "secmon_interrupt_handler.hpp"
#include "secmon_misc.hpp"
#include "smc/secmon_random_cache.hpp"
#include "smc/secmon_smc_power_management.hpp"
#include "smc/secmon_smc_se_lock.hpp"

namespace ams::secmon {

    namespace {

        constexpr inline const uintptr_t TIMER     = secmon::MemoryRegionVirtualDeviceTimer.GetAddress();
        constexpr inline const uintptr_t SYSTEM    = secmon::MemoryRegionVirtualDeviceSystem.GetAddress();
        constexpr inline const uintptr_t APB_MISC  = secmon::MemoryRegionVirtualDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t FLOW_CTLR = secmon::MemoryRegionVirtualDeviceFlowController.GetAddress();
        constexpr inline const uintptr_t PMC       = secmon::MemoryRegionVirtualDevicePmc.GetAddress();
        constexpr inline const uintptr_t MC        = secmon::MemoryRegionVirtualDeviceMemoryController.GetAddress();
        constexpr inline const uintptr_t EVP       = secmon::MemoryRegionVirtualDeviceExceptionVectors.GetAddress();
        constexpr inline const uintptr_t CLK_RST   = secmon::MemoryRegionVirtualDeviceClkRst.GetAddress();

        alignas(8) constinit u8 g_se_aes_key_slot_test_vector[se::AesBlockSize] = {};

        struct Carveout {
            uintptr_t address;
            size_t    size;
        };

        constinit Carveout g_kernel_carveouts[KernelCarveoutCount] = {
            { secmon::MemoryRegionDramDefaultKernelCarveout.GetAddress(), secmon::MemoryRegionDramDefaultKernelCarveout.GetSize(), },
            { 0, 0, },
        };

        constinit bool g_is_cold_boot = true;

        constinit se::StickyBits ExpectedSeStickyBits = {
            .se_security            = (1 << 0), /* SE_HARD_SETTING */
            .tzram_security         = 0,
            .crypto_security_perkey = (1 << pkg1::AesKeySlot_UserEnd) - 1,
            .crypto_keytable_access = {
                (0 << 7) | (1 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  0: User    keyslot. KEY. KEYUSE, UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. UIVREAD, OIVREAD, KEYREAD disabled. */
                (0 << 7) | (1 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  1: User    keyslot. KEY. KEYUSE, UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. UIVREAD, OIVREAD, KEYREAD disabled. */
                (0 << 7) | (1 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  2: User    keyslot. KEY. KEYUSE, UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. UIVREAD, OIVREAD, KEYREAD disabled. */
                (0 << 7) | (1 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  3: User    keyslot. KEY. KEYUSE, UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. UIVREAD, OIVREAD, KEYREAD disabled. */
                (0 << 7) | (1 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  4: User    keyslot. KEY. KEYUSE, UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. UIVREAD, OIVREAD, KEYREAD disabled. */
                (0 << 7) | (1 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  5: User    keyslot. KEY. KEYUSE, UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. UIVREAD, OIVREAD, KEYREAD disabled. */
                (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /*  6: Unused  keyslot. KEK. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
                (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /*  7: Unused  keyslot. KEK. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
                (0 << 7) | (0 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  8: Temp    keyslot. KEY. UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. KEYUSE, UIVREAD, OIVREAD, KEYREAD disabled. */
                (0 << 7) | (0 << 6) | (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (1 << 1) | (0 << 0), /*  9: SmcTemp keyslot. KEY. UIVUPDATE, OIVUPDATE, KEYUPDATE enabled. KEYUSE, UIVREAD, OIVREAD, KEYREAD disabled. */
                (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /* 10: Wrap1   keyslot. KEK. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
                (0 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /* 11: Wrap2   keyslot. KEY. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
                (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /* 12: DMaster keyslot. KEK. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
                (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /* 13: Master  keyslot. KEK. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
                (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /* 14: Unused  keyslot. KEK. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
                (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | (0 << 0), /* 13: Device  keyslot. KEK. KEYUSE, UIVUPDATE, UIVREAD, OIVUPDATE, OIVREAD, KEYUPDATE, KEYREAD disabled. */
            },
            .rsa_security_perkey = 0,
            .rsa_keytable_access = {
                (0 << 2) | (1 << 1) | (0 << 0), /* KEYUSE/KEYREAD disabled, KEYUPDATE enabled. */
                (0 << 2) | (1 << 1) | (0 << 0), /* KEYUSE/KEYREAD disabled, KEYUPDATE enabled. */
            },
        };

        void InitializeConfigurationContext() {
            /* Get the global context. */
            auto &ctx = ::ams::secmon::impl::GetConfigurationContext();

            /* Clear the context to zero. */
            std::memset(std::addressof(ctx), 0, sizeof(ctx));

            /* If the storage context is valid, we want to copy it to the global context. */
            if (const auto &storage_ctx = *MemoryRegionPhysicalDramMonitorConfiguration.GetPointer<const SecureMonitorStorageConfiguration>(); storage_ctx.IsValid()) {
                ctx.secmon_cfg.CopyFrom(storage_ctx);
                ctx.emummc_cfg = storage_ctx.emummc_cfg;
            } else {
                /* If we don't have a valid storage context, we can just use the default one. */
                ctx.secmon_cfg = DefaultSecureMonitorConfiguration;
            }

            /* Cache the fuse info for quick access. */
            ctx.secmon_cfg.SetFuseInfo();
        }

        void GenerateSecurityEngineAesKeySlotTestVector(void *dst, size_t size) {
            /* Clear the output. */
            AMS_ABORT_UNLESS(size == se::AesBlockSize);
            std::memset(dst, 0, se::AesBlockSize);

            /* Ensure output is seen as cleared by the se. */
            hw::FlushDataCache(dst, se::AesBlockSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Declare a block. */
            alignas(8) u8 empty_block[se::AesBlockSize];

            /* Iteratively transform an empty block. */
            #define TRANSFORM_WITH_KEY(key)                                                                   \
                __builtin_memset(empty_block, 0, sizeof(empty_block));                                        \
                se::SetEncryptedAesKey256(pkg1::AesKeySlot_Temporary, key, empty_block, sizeof(empty_block)); \
                se::DecryptAes128(dst, se::AesBlockSize, pkg1::AesKeySlot_Temporary, dst, se::AesBlockSize)

            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_RandomForUserWrap);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_RandomForKeyStorageWrap);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_Master);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_DeviceMaster);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_Device);

            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_RandomForUserWrap);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_RandomForKeyStorageWrap);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_Master);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_DeviceMaster);
            TRANSFORM_WITH_KEY(pkg1::AesKeySlot_Device);

            /* Ensure output is seen correctly by the cpu. */
            hw::FlushDataCache(dst, se::AesBlockSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Clear the temporary key slot. */
            se::ClearAesKeySlot(pkg1::AesKeySlot_Temporary);
        }

        void VerifySecurityEngineStickyBits() {
            /* On mariko, an extra sticky bit is set. */
            if (GetSocType() == fuse::SocType_Mariko) {
                ExpectedSeStickyBits.se_security |= (1 << 5);
            } else /* if (GetSocType() == fuse::SocType_Erista) */ {
                /* Erista does not support DST_KEYTABLE_ONLY, and so all keys will have the bit clear. */
                for (size_t i = 0; i < util::size(ExpectedSeStickyBits.crypto_keytable_access); ++i) {
                    ExpectedSeStickyBits.crypto_keytable_access[i] &= ~(1 << 7);
                }
            }

            if (!se::ValidateStickyBits(ExpectedSeStickyBits)) {
                SetError(pkg1::ErrorInfo_InvalidSecurityEngineStickyBits);
                AMS_ABORT("Invalid sticky bits");
            }
        }

        void VerifySecurityEngineAesKeySlotTestVector() {
            alignas(8) u8 test_vector[se::AesBlockSize];
            GenerateSecurityEngineAesKeySlotTestVector(test_vector, sizeof(test_vector));

            AMS_ABORT_UNLESS(crypto::IsSameBytes(g_se_aes_key_slot_test_vector, test_vector, se::AesBlockSize));
        }

        void ClearAesKeySlots() {
            /* Clear all non-secure monitor keys. */
            for (int i = 0; i < pkg1::AesKeySlot_SecmonStart; ++i) {
                se::ClearAesKeySlot(i);
            }

            /* Clear the secure-monitor temporary keys. */
            se::ClearAesKeySlot(pkg1::AesKeySlot_Temporary);
            se::ClearAesKeySlot(pkg1::AesKeySlot_Smc);
        }

        void ClearRsaKeySlots() {
            /* Clear all rsa keyslots. */
            for (int i = 0; i < se::RsaKeySlotCount; ++i) {
                se::ClearRsaKeySlot(i);
            }
        }

        void SetupKernelCarveouts() {
            #define MC_ENABLE_CLIENT_ACCESS(INDEX, WHICH) MC_REG_BITS_ENUM(CLIENT_ACCESS##INDEX##_##WHICH, ENABLE)

            constexpr u32 ClientAccess0     = reg::Encode(MC_ENABLE_CLIENT_ACCESS(0, PTCR),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAY0A),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAY0AB),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAY0B),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAY0BB),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAY0C),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAY0CB),
                                                          MC_ENABLE_CLIENT_ACCESS(0, AFIR),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAYHC),
                                                          MC_ENABLE_CLIENT_ACCESS(0, DISPLAYHCB),
                                                          MC_ENABLE_CLIENT_ACCESS(0, HDAR),
                                                          MC_ENABLE_CLIENT_ACCESS(0, HOST1XDMAR),
                                                          MC_ENABLE_CLIENT_ACCESS(0, HOST1XR),
                                                          MC_ENABLE_CLIENT_ACCESS(0, NVENCSRD),
                                                          MC_ENABLE_CLIENT_ACCESS(0, PPCSAHBDMAR),
                                                          MC_ENABLE_CLIENT_ACCESS(0, PPCSAHBSLVR));

            constexpr u32 ClientAccess1     = reg::Encode(MC_ENABLE_CLIENT_ACCESS(1, MPCORER),
                                                          MC_ENABLE_CLIENT_ACCESS(1, NVENCSWR),
                                                          MC_ENABLE_CLIENT_ACCESS(1, AFIW),
                                                          MC_ENABLE_CLIENT_ACCESS(1, HDAW),
                                                          MC_ENABLE_CLIENT_ACCESS(1, HOST1XW),
                                                          MC_ENABLE_CLIENT_ACCESS(1, MPCOREW),
                                                          MC_ENABLE_CLIENT_ACCESS(1, PPCSAHBDMAW),
                                                          MC_ENABLE_CLIENT_ACCESS(1, PPCSAHBSLVW));

            constexpr u32 ClientAccess2     = reg::Encode(MC_ENABLE_CLIENT_ACCESS(2, XUSB_HOSTR),
                                                          MC_ENABLE_CLIENT_ACCESS(2, XUSB_HOSTW),
                                                          MC_ENABLE_CLIENT_ACCESS(2, XUSB_DEVR),
                                                          MC_ENABLE_CLIENT_ACCESS(2, XUSB_DEVW));

            constexpr u32 ClientAccess2_100 = reg::Encode(MC_ENABLE_CLIENT_ACCESS(2, XUSB_HOSTR),
                                                          MC_ENABLE_CLIENT_ACCESS(2, XUSB_HOSTW),
                                                          MC_ENABLE_CLIENT_ACCESS(2, XUSB_DEVR),
                                                          MC_ENABLE_CLIENT_ACCESS(2, XUSB_DEVW),
                                                          MC_ENABLE_CLIENT_ACCESS(2, TSECSRD),
                                                          MC_ENABLE_CLIENT_ACCESS(2, TSECSWR));

            constexpr u32 ClientAccess3     = reg::Encode(MC_ENABLE_CLIENT_ACCESS(3, SDMMCRA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCRAA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCRAB),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCWA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCWAA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCWAB),
                                                          MC_ENABLE_CLIENT_ACCESS(3, VICSRD),
                                                          MC_ENABLE_CLIENT_ACCESS(3, VICSWR),
                                                          MC_ENABLE_CLIENT_ACCESS(3, DISPLAYD),
                                                          MC_ENABLE_CLIENT_ACCESS(3, APER),
                                                          MC_ENABLE_CLIENT_ACCESS(3, APEW),
                                                          MC_ENABLE_CLIENT_ACCESS(3, NVJPGSRD),
                                                          MC_ENABLE_CLIENT_ACCESS(3, NVJPGSWR));

            constexpr u32 ClientAccess3_100 = reg::Encode(MC_ENABLE_CLIENT_ACCESS(3, SDMMCRA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCRAA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCRAB),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCWA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCWAA),
                                                          MC_ENABLE_CLIENT_ACCESS(3, SDMMCWAB),
                                                          MC_ENABLE_CLIENT_ACCESS(3, VICSRD),
                                                          MC_ENABLE_CLIENT_ACCESS(3, VICSWR),
                                                          MC_ENABLE_CLIENT_ACCESS(3, DISPLAYD),
                                                          MC_ENABLE_CLIENT_ACCESS(3, NVDECSRD),
                                                          MC_ENABLE_CLIENT_ACCESS(3, NVDECSWR),
                                                          MC_ENABLE_CLIENT_ACCESS(3, APER),
                                                          MC_ENABLE_CLIENT_ACCESS(3, APEW),
                                                          MC_ENABLE_CLIENT_ACCESS(3, NVJPGSRD),
                                                          MC_ENABLE_CLIENT_ACCESS(3, NVJPGSWR));

            constexpr u32 ClientAccess4     = reg::Encode(MC_ENABLE_CLIENT_ACCESS(4, SESRD),
                                                          MC_ENABLE_CLIENT_ACCESS(4, SESWR));

            constexpr u32 ClientAccess4_800 = reg::Encode(MC_ENABLE_CLIENT_ACCESS(4, SESRD),
                                                          MC_ENABLE_CLIENT_ACCESS(4, SESWR),
                                                          MC_ENABLE_CLIENT_ACCESS(4, TSECRDB),
                                                          MC_ENABLE_CLIENT_ACCESS(4, TSECWRB));


            constexpr u32 ClientAccess4_100 = reg::Encode(MC_ENABLE_CLIENT_ACCESS(4, SESRD),
                                                          MC_ENABLE_CLIENT_ACCESS(4, SESWR));

            #undef MC_ENABLE_CLIENT_ACCESS

            constexpr u32 ForceInternalAccess0     = reg::Encode(MC_REG_BITS_ENUM(CLIENT_ACCESS0_AVPCARM7R, ENABLE));
            constexpr u32 ForceInternalAccess0_100 = 0;

            constexpr u32 ForceInternalAccess1     = reg::Encode(MC_REG_BITS_ENUM(CLIENT_ACCESS1_AVPCARM7W, ENABLE));
            constexpr u32 ForceInternalAccess1_100 = 0;

            constexpr u32 CarveoutConfig        = reg::Encode(MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_IS_WPR,                                 DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_FORCE_APERTURE_ID_MATCH,                DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ALLOW_APERTURE_ID_MISMATCH,             DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_RD_EN,                        DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_WR_EN,                        DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_SEND_CFG_TO_GPU,                        DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL3, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL2, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL1, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL0, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL3,  ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL2,  ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL1,  ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL0,  ENABLE_CHECKS),
                                                              MC_REG_BITS_VALUE(SECURITY_CARVEOUT_CFG0_APERTURE_ID,                                   0),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL3,                     ENABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL2,                    DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL1,                    DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL0,                     ENABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL3,                      ENABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL2,                     DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL1,                     DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL0,                      ENABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ADDRESS_TYPE,                        ANY_ADDRESS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_LOCK_MODE,                                LOCKED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_PROTECT_MODE,                          TZ_SECURE));

            constexpr u32 CarveoutConfig_100    = reg::Encode(MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_IS_WPR,                                 DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_FORCE_APERTURE_ID_MATCH,                DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ALLOW_APERTURE_ID_MISMATCH,             DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_RD_EN,                        DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_WR_EN,                        DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_SEND_CFG_TO_GPU,                        DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL3, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL2, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL1, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL0, ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL3,  ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL2,  ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL1,  ENABLE_CHECKS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL0,  ENABLE_CHECKS),
                                                              MC_REG_BITS_VALUE(SECURITY_CARVEOUT_CFG0_APERTURE_ID,                                   0),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL3,                    DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL2,                    DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL1,                    DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL0,                     ENABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL3,                     DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL2,                     DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL1,                     DISABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL0,                      ENABLED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ADDRESS_TYPE,                        ANY_ADDRESS),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_LOCK_MODE,                                LOCKED),
                                                              MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_PROTECT_MODE,                          TZ_SECURE));

            const u32 target_fw = GetTargetFirmware();
            u32 client_access_2;
            u32 client_access_3;
            u32 client_access_4;
            u32 carveout_config;
            if (target_fw >= TargetFirmware_8_1_0) {
                client_access_2 = ClientAccess2;
                client_access_3 = ClientAccess3;
                client_access_4 = ClientAccess4;
                carveout_config = CarveoutConfig;
            } else if (target_fw >= TargetFirmware_8_0_0) {
                client_access_2 = ClientAccess2;
                client_access_3 = ClientAccess3;
                client_access_4 = ClientAccess4_800;
                carveout_config = CarveoutConfig;
            } else {
                client_access_2 = ClientAccess2_100;
                client_access_3 = ClientAccess3_100;
                client_access_4 = ClientAccess4_100;
                carveout_config = CarveoutConfig_100;
            }

            /* Configure carveout 4. */
            reg::Write(MC + MC_SECURITY_CARVEOUT4_BOM,                                                                             g_kernel_carveouts[0].address >>  0);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_BOM_HI,                                                                          g_kernel_carveouts[0].address >> 32);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_SIZE_128KB,                                                                      g_kernel_carveouts[0].size / 128_KB);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS0,                                                                                        ClientAccess0);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS1,                                                                                        ClientAccess1);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS2,                                                                                      client_access_2);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS3,                                                                                      client_access_3);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS4,                                                                                      client_access_4);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS0, (target_fw >= TargetFirmware_4_0_0) ? ForceInternalAccess0 : ForceInternalAccess0_100);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS1, (target_fw >= TargetFirmware_4_0_0) ? ForceInternalAccess1 : ForceInternalAccess1_100);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS2,                                                                                     0);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS3,                                                                                     0);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS4,                                                                                     0);
            reg::Write(MC + MC_SECURITY_CARVEOUT4_CFG0,                                                                                                carveout_config);

            /* Configure carveout 5. */
            reg::Write(MC + MC_SECURITY_CARVEOUT5_BOM,                           g_kernel_carveouts[0].address >>  0);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_BOM_HI,                        g_kernel_carveouts[0].address >> 32);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_SIZE_128KB,                    g_kernel_carveouts[0].size / 128_KB);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_ACCESS0,                                      ClientAccess0);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_ACCESS1,                                      ClientAccess1);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_ACCESS2,                                    client_access_2);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_ACCESS3,                                    client_access_3);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_ACCESS4,                                    client_access_4);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS0,                                   0);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS1,                                   0);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS2,                                   0);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS3,                                   0);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS4,                                   0);
            reg::Write(MC + MC_SECURITY_CARVEOUT5_CFG0,                                              carveout_config);
        }

        void ConfigureSlaveSecurity() {
            u32 reg0, reg1, reg2;
            if (GetTargetFirmware() > TargetFirmware_1_0_0) {
                reg0 =  reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(0, SATA_AUX, ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(0, DTV,      ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(0, QSPI,     ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(0, SE,       ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(0, SATA,     ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(0, LA,       ENABLE));

                reg1 =  reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(1, SPI1, ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(1, SPI2, ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(1, SPI3, ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(1, SPI5, ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(1, SPI6, ENABLE),
                                    SLAVE_SECURITY_REG_BITS_ENUM(1, I2C6, ENABLE));

                reg2 = reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(2, SDMMC3, ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(2, DDS,    ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(2, DP2,    ENABLE));

                const auto hw_type = GetHardwareType();

                /* Switch Lite can't use HDMI, so set CEC to secure on hoag. */
                if (hw_type == fuse::HardwareType_Hoag) {
                    reg0 |= reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(0, CEC, ENABLE));
                }

                /* Icosa, Iowa, and Aula all set I2C4 to be secure. */
                if (hw_type == fuse::HardwareType_Icosa && hw_type == fuse::HardwareType_Iowa && hw_type == fuse::HardwareType_Aula) {
                    reg1 |= reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(1, I2C4, ENABLE));

                }

                /* Hoag additionally sets UART_B to secure. */
                if (hw_type == fuse::HardwareType_Hoag) {
                    reg1 |= reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(1, UART_B, ENABLE));
                }

                /* Copper and Calcio lack a lot of hardware, so set the corresponding registers to secure for them. */
                if (hw_type == fuse::HardwareType_Calcio || hw_type == fuse::HardwareType_Copper) {
                    reg1 |= reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(1, UART_B, ENABLE),
                                        SLAVE_SECURITY_REG_BITS_ENUM(1, UART_C, ENABLE),
                                        SLAVE_SECURITY_REG_BITS_ENUM(1, SPI4,   ENABLE),
                                        SLAVE_SECURITY_REG_BITS_ENUM(1, I2C2,   ENABLE),
                                        SLAVE_SECURITY_REG_BITS_ENUM(1, I2C3,   ENABLE));

                    /* Copper/Calcio have no gamecard reader, and thus set SDMMC2 as secure. */
                    reg2 |= reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(2, SDMMC2, ENABLE));
                }

                /* Mariko hardware types (not Icosa or Copper) additionally set mariko-only mmio (SE2, PKA1, FEK) as secure. */
                if (hw_type != fuse::HardwareType_Icosa && hw_type != fuse::HardwareType_Copper) {
                    reg2 |= reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(2, SE2,  ENABLE),
                                        SLAVE_SECURITY_REG_BITS_ENUM(2, PKA1, ENABLE),
                                        SLAVE_SECURITY_REG_BITS_ENUM(2, FEK,  ENABLE));
                }
            } else {
                reg0 = reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(0, SATA_AUX, ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(0, DTV,      ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(0, QSPI,     ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(0, SATA,     ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(0, LA,       ENABLE));

                reg1 = reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(1, SPI1, ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(1, SPI2, ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(1, SPI3, ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(1, SPI5, ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(1, SPI6, ENABLE),
                                   SLAVE_SECURITY_REG_BITS_ENUM(1, I2C6, ENABLE));

                reg2 = reg::Encode(SLAVE_SECURITY_REG_BITS_ENUM(2, DDS, ENABLE),
                                   REG_BITS_VALUE(5, 1, 1),  /* Note: Bit 5 is not documented in TRM. */
                                   REG_BITS_VALUE(4, 1, 1)); /* Note: Bit 4 is not documented in TRM. */
            }

            reg::Write(APB_MISC + APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0, reg0);
            reg::Write(APB_MISC + APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0, reg1);
            reg::Write(APB_MISC + APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG2_0, reg2);
        }

        void SetupSecureRegisters() {
            /* Configure timers 5-8 and watchdog timers 0-3 as secure. */
            reg::Write(TIMER + TIMER_SHARED_TIMER_SECURE_CFG, TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_TMR5, ENABLE),
                                                              TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_TMR6, ENABLE),
                                                              TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_TMR7, ENABLE),
                                                              TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_TMR8, ENABLE),
                                                              TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_WDT0, ENABLE),
                                                              TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_WDT1, ENABLE),
                                                              TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_WDT2, ENABLE),
                                                              TIMER_REG_BITS_ENUM(SHARED_TIMER_SECURE_CFG_WDT3, ENABLE));

            /* Lock cluster switching, to prevent usage of the A53 cores. */
            reg::Write(FLOW_CTLR + FLOW_CTLR_BPMP_CLUSTER_CONTROL, FLOW_REG_BITS_ENUM(BPMP_CLUSTER_CONTROL_ACTIVE_CLUSTER_LOCK,    ENABLE),
                                                                FLOW_REG_BITS_ENUM(BPMP_CLUSTER_CONTROL_CLUSTER_SWITCH_ENABLE, DISABLE),
                                                                FLOW_REG_BITS_ENUM(BPMP_CLUSTER_CONTROL_ACTIVE_CLUSTER,           FAST));

            /* Enable flow controller debug qualifier for legacy FIQs. */
            reg::Write(FLOW_CTLR + FLOW_CTLR_FLOW_DBG_QUAL, FLOW_REG_BITS_ENUM(FLOW_DBG_QUAL_FIQ2CCPLEX_ENABLE, ENABLE));

            /* Configure the PMC to disable deep power-down. */
            reg::Write(PMC + APBDEV_PMC_DPD_ENABLE, PMC_REG_BITS_ENUM(DPD_ENABLE_TSC_MULT_EN, DISABLE),
                                                    PMC_REG_BITS_ENUM(DPD_ENABLE_ON,          DISABLE));

            /* Configure the video protect region. */
            reg::Write(MC + MC_VIDEO_PROTECT_GPU_OVERRIDE_0, 1);
            reg::Write(MC + MC_VIDEO_PROTECT_GPU_OVERRIDE_1, 0);
            reg::Write(MC + MC_VIDEO_PROTECT_BOM,            0);
            reg::Write(MC + MC_VIDEO_PROTECT_SIZE_MB,        0);
            reg::Write(MC + MC_VIDEO_PROTECT_REG_CTRL,       MC_REG_BITS_ENUM(VIDEO_PROTECT_REG_CTRL_VIDEO_PROTECT_ALLOW_TZ_WRITE, DISABLED),
                                                             MC_REG_BITS_ENUM(VIDEO_PROTECT_REG_CTRL_VIDEO_PROTECT_WRITE_ACCESS,   DISABLED));

            /* Configure the SEC carveout. */
            reg::Write(MC + MC_SEC_CARVEOUT_BOM,      0);
            reg::Write(MC + MC_SEC_CARVEOUT_SIZE_MB,  0);
            reg::Write(MC + MC_SEC_CARVEOUT_REG_CTRL, MC_REG_BITS_ENUM(SEC_CARVEOUT_REG_CTRL_SEC_CARVEOUT_WRITE_ACCESS,   DISABLED));

            /* Configure the MTS carveout. */
            reg::Write(MC + MC_MTS_CARVEOUT_BOM,      0);
            reg::Write(MC + MC_MTS_CARVEOUT_SIZE_MB,  0);
            reg::Write(MC + MC_MTS_CARVEOUT_ADR_HI,   0);
            reg::Write(MC + MC_MTS_CARVEOUT_REG_CTRL, MC_REG_BITS_ENUM(MTS_CARVEOUT_REG_CTRL_MTS_CARVEOUT_WRITE_ACCESS,   DISABLED));

            /* Configure the security carveout. */
            reg::Write(MC + MC_SECURITY_CFG0, MC_REG_BITS_VALUE(SECURITY_CFG0_SECURITY_BOM,    0));
            reg::Write(MC + MC_SECURITY_CFG1, MC_REG_BITS_VALUE(SECURITY_CFG1_SECURITY_SIZE,   0));
            reg::Write(MC + MC_SECURITY_CFG3, MC_REG_BITS_VALUE(SECURITY_CFG3_SECURITY_BOM_HI, 3));

            /* Configure security carveout 1. */
            reg::Write(MC + MC_SECURITY_CARVEOUT1_BOM,                           0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_BOM_HI,                        0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_SIZE_128KB,                    0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_ACCESS0,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_ACCESS1,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_ACCESS2,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_ACCESS3,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_ACCESS4,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS0, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS1, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS2, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS3, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS4, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT1_CFG0,                          MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_IS_WPR,                                 DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_FORCE_APERTURE_ID_MATCH,                 ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ALLOW_APERTURE_ID_MISMATCH,             DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_RD_EN,                        DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_WR_EN,                        DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_SEND_CFG_TO_GPU,                        DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL3, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL2, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL1, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL0, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL3,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL2,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL1,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL0,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_VALUE(SECURITY_CARVEOUT_CFG0_APERTURE_ID,                                   0),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL3,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL2,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL1,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL0,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL3,                     DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL2,                     DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL1,                     DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL0,                     DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ADDRESS_TYPE,                  UNTRANSLATED_ONLY),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_LOCK_MODE,                                LOCKED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_PROTECT_MODE,                     LOCKBIT_SECURE));

            /* Security carveout 2 will be configured later by SetupGpuCarveout, after magic values are written to configure gpu/tsec. */

            /* Configure carveout 3. */
            reg::Write(MC + MC_SECURITY_CARVEOUT3_BOM,                           0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_BOM_HI,                        0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_SIZE_128KB,                    0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_ACCESS0,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_ACCESS1,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_ACCESS2,                MC_REG_BITS_ENUM (CLIENT_ACCESS2_GPUSRD,  ENABLE),
                                                                                 MC_REG_BITS_ENUM (CLIENT_ACCESS2_GPUSWR,  ENABLE));
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_ACCESS3,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_ACCESS4,                MC_REG_BITS_ENUM (CLIENT_ACCESS4_GPUSRD2, ENABLE),
                                                                                 MC_REG_BITS_ENUM (CLIENT_ACCESS4_GPUSWR2, ENABLE));
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS0, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS1, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS2, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS3, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS4, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT3_CFG0,                          MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_IS_WPR,                                 DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_FORCE_APERTURE_ID_MATCH,                 ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ALLOW_APERTURE_ID_MISMATCH,             DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_RD_EN,                        DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_WR_EN,                        DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_SEND_CFG_TO_GPU,                         ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL3, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL2, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL1, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL0, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL3,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL2,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL1,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL0,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_VALUE(SECURITY_CARVEOUT_CFG0_APERTURE_ID,                                   3),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL3,                     ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL2,                     ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL1,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL0,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL3,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL2,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL1,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL0,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ADDRESS_TYPE,                  UNTRANSLATED_ONLY),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_LOCK_MODE,                                LOCKED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_PROTECT_MODE,                     LOCKBIT_SECURE));

            /* If we're cold-booting and on 1.0.0, alter the default carveout size. */
            if (g_is_cold_boot && GetTargetFirmware() <= TargetFirmware_1_0_0) {
                g_kernel_carveouts[0].size = 200 * 128_KB;
            }

            /* Configure the two kernel carveouts. */
            SetupKernelCarveouts();

            /* Configure slave register security. */
            ConfigureSlaveSecurity();
        }

        void SetupSmmu() {
            /* Turn on SMMU translation for all devices. */
            reg::Write(MC + MC_SMMU_TRANSLATION_ENABLE_0, ~0u);
            reg::Write(MC + MC_SMMU_TRANSLATION_ENABLE_1, ~0u);
            reg::Write(MC + MC_SMMU_TRANSLATION_ENABLE_2, ~0u);
            reg::Write(MC + MC_SMMU_TRANSLATION_ENABLE_3, ~0u);
            reg::Write(MC + MC_SMMU_TRANSLATION_ENABLE_4, ~0u);

            /* On modern firmware, configure ASIDs 1-3 as secure, and all others as non-secure. */
            if (GetTargetFirmware() >= TargetFirmware_4_0_0) {
                reg::Write(MC + MC_SMMU_ASID_SECURITY,   MC_REG_BITS_ENUM(SMMU_ASID_SECURITY_SECURE_ASIDS_1, SECURE),
                                                         MC_REG_BITS_ENUM(SMMU_ASID_SECURITY_SECURE_ASIDS_2, SECURE),
                                                         MC_REG_BITS_ENUM(SMMU_ASID_SECURITY_SECURE_ASIDS_3, SECURE));
            } else {
                /* Legacy firmware accesses the MC directly, though, and so correspondingly we must allow ASIDs to be edited by non-secure world. */
                reg::Write(MC + MC_SMMU_ASID_SECURITY, 0);
            }

            reg::Write(MC + MC_SMMU_ASID_SECURITY_1, 0);
            reg::Write(MC + MC_SMMU_ASID_SECURITY_2, 0);
            reg::Write(MC + MC_SMMU_ASID_SECURITY_3, 0);
            reg::Write(MC + MC_SMMU_ASID_SECURITY_4, 0);
            reg::Write(MC + MC_SMMU_ASID_SECURITY_5, 0);
            reg::Write(MC + MC_SMMU_ASID_SECURITY_6, 0);
            reg::Write(MC + MC_SMMU_ASID_SECURITY_7, 0);

            /* Initialize the PTB registers to zero .*/
            reg::Write(MC + MC_SMMU_PTB_ASID, 0);
            reg::Write(MC + MC_SMMU_PTB_DATA, 0);

            /* Configure the TLB and PTC, then read TLB_CONFIG to ensure configuration takes. */
            reg::Write(MC + MC_SMMU_TLB_CONFIG, MC_REG_BITS_ENUM (SMMU_TLB_CONFIG_TLB_HIT_UNDER_MISS,          ENABLE),
                                                MC_REG_BITS_ENUM (SMMU_TLB_CONFIG_TLB_ROUND_ROBIN_ARBITRATION, ENABLE),
                                                MC_REG_BITS_VALUE(SMMU_TLB_CONFIG_TLB_ACTIVE_LINES,              0x30));

            reg::Write(MC + MC_SMMU_PTC_CONFIG, MC_REG_BITS_ENUM (SMMU_PTC_CONFIG_PTC_CACHE_ENABLE, ENABLE),
                                                MC_REG_BITS_VALUE(SMMU_PTC_CONFIG_PTC_REQ_LIMIT,         8),
                                                MC_REG_BITS_VALUE(SMMU_PTC_CONFIG_PTC_INDEX_MAP,      0x3F));

            reg::Read (MC + MC_SMMU_TLB_CONFIG);

            /* Flush the entire page table cache, and read TLB_CONFIG to ensure the flush takes. */
            reg::Write(MC + MC_SMMU_PTC_FLUSH_0, 0);
            reg::Read (MC + MC_SMMU_TLB_CONFIG);

            /* Flush the entire translation lookaside buffer, and read TLB_CONFIG to ensure the flush takes. */
            reg::Write(MC + MC_SMMU_TLB_FLUSH, 0);
            reg::Read (MC + MC_SMMU_TLB_CONFIG);

            /* Enable the SMMU, and read TLB_CONFIG to ensure the enable takes. */
            reg::Write(MC + MC_SMMU_CONFIG, MC_REG_BITS_ENUM (SMMU_CONFIG_SMMU_ENABLE, ENABLE));
            reg::Read (MC + MC_SMMU_TLB_CONFIG);
        }

        void SetupSecureEl2AndEl1SystemRegisters() {
            /* Setup actlr_el2 and actlr_el3. */
            {
                util::BitPack32 actlr = {};

                actlr.Set<hw::ActlrCortexA57::Cpuactlr>(1); /* Enable access to cpuactlr from lower EL. */
                actlr.Set<hw::ActlrCortexA57::Cpuectlr>(1); /* Enable access to cpuectlr from lower EL. */
                actlr.Set<hw::ActlrCortexA57::L2ctlr>(1);   /* Enable access to l2ctlr from lower EL. */
                actlr.Set<hw::ActlrCortexA57::L2actlr>(1);  /* Enable access to l2actlr from lower EL. */
                actlr.Set<hw::ActlrCortexA57::L2ectlr>(1);  /* Enable access to l2ectlr from lower EL. */

                HW_CPU_SET_ACTLR_EL3(actlr);
                HW_CPU_SET_ACTLR_EL2(actlr);
            }

            /* Setup hcr_el2. */
            {
                util::BitPack64 hcr = {};

                hcr.Set<hw::HcrEl2::Rw>(1); /* EL1 is aarch64 mode. */

                HW_CPU_SET_HCR_EL2(hcr);
            }

            /* Configure all domain access permissions as manager. */
            HW_CPU_SET_DACR32_EL2(~0u);

            /* Setup sctlr_el1. */
            {
                util::BitPack64 sctlr = { hw::SctlrEl1::Res1 };

                sctlr.Set<hw::SctlrEl1::M>(0);       /* Globally disable the MMU. */
                sctlr.Set<hw::SctlrEl1::A>(0);       /* Disable alignment fault checking. */
                sctlr.Set<hw::SctlrEl1::C>(0);       /* Globally disable the data and unified caches. */
                sctlr.Set<hw::SctlrEl1::Sa>(1);      /* Enable stack alignment checking. */
                sctlr.Set<hw::SctlrEl1::Sa0>(1);     /* Enable el0 stack alignment checking. */
                sctlr.Set<hw::SctlrEl1::Cp15BEn>(1); /* Enable cp15 barrier operations. */
                sctlr.Set<hw::SctlrEl1::Thee>(0);    /* Disable ThumbEE. */
                sctlr.Set<hw::SctlrEl1::Itd>(0);     /* Enable itd instructions. */
                sctlr.Set<hw::SctlrEl1::Sed>(0);     /* Enable setend instruction. */
                sctlr.Set<hw::SctlrEl1::Uma>(0);     /* Disable el0 interrupt mask access. */
                sctlr.Set<hw::SctlrEl1::I>(0);       /* Globally disable the instruction cache. */
                sctlr.Set<hw::SctlrEl1::Dze>(0);     /* Disable el0 access to dc zva instruction. */
                sctlr.Set<hw::SctlrEl1::Ntwi>(1);    /* wfi instructions in el0 trap. */
                sctlr.Set<hw::SctlrEl1::Ntwe>(1);    /* wfe instructions in el0 trap. */
                sctlr.Set<hw::SctlrEl1::Wxn>(0);     /* Do not force writable pages to be ExecuteNever. */
                sctlr.Set<hw::SctlrEl1::E0e>(0);     /* Data accesses in el0 are little endian. */
                sctlr.Set<hw::SctlrEl1::Ee>(0);      /* Exceptions should be little endian. */
                sctlr.Set<hw::SctlrEl1::Uci>(0);     /* Disable el0 access to dc cvau, dc civac, dc cvac, ic ivau. */

                HW_CPU_SET_SCTLR_EL1(sctlr);
            }

            /* Setup sctlr_el2. */
            {
                util::BitPack64 sctlr = { hw::SctlrEl2::Res1 };

                sctlr.Set<hw::SctlrEl2::M>(0);       /* Globally disable the MMU. */
                sctlr.Set<hw::SctlrEl2::A>(0);       /* Disable alignment fault checking. */
                sctlr.Set<hw::SctlrEl2::C>(0);       /* Globally disable the data and unified caches. */
                sctlr.Set<hw::SctlrEl2::Sa>(1);      /* Enable stack alignment checking. */
                sctlr.Set<hw::SctlrEl2::I>(0);       /* Globally disable the instruction cache. */
                sctlr.Set<hw::SctlrEl2::Wxn>(0);     /* Do not force writable pages to be ExecuteNever. */
                sctlr.Set<hw::SctlrEl2::Ee>(0);      /* Exceptions should be little endian. */

                HW_CPU_SET_SCTLR_EL2(sctlr);
            }

            /* Ensure instruction consistency. */
            hw::InstructionSynchronizationBarrier();
        }

        void SetupNonSecureSystemRegisters(u32 tsc_frequency) {
            /* Set cntfrq_el0. */
            HW_CPU_SET_CNTFRQ_EL0(tsc_frequency);

            /* Set cnthctl_el2. */
            {
                util::BitPack32 cnthctl = {};

                cnthctl.Set<hw::CnthctlEl2::El1PctEn>(1); /* Do not trap accesses to cntpct_el0. */
                cnthctl.Set<hw::CnthctlEl2::El1PcEn>(1);  /* Do not trap accesses to cntp_ctl_el0, cntp_cval_el0, and cntp_tval_el0. */
                cnthctl.Set<hw::CnthctlEl2::EvntEn>(0);   /* Disable the event stream. */
                cnthctl.Set<hw::CnthctlEl2::EvntDir>(0);  /* Trigger events on 0 -> 1 transition. */
                cnthctl.Set<hw::CnthctlEl2::EvntI>(0);    /* Select bit0 of cntpct_el0 as the event stream trigger. */

                HW_CPU_SET_CNTHCTL_EL2(cnthctl);
            }

            /* Ensure instruction consistency. */
            hw::InstructionSynchronizationBarrier();
        }

        void SetupGpuCarveout() {
            /* Configure carveout 2. */
            reg::Write(MC + MC_SECURITY_CARVEOUT2_BOM,                           static_cast<u32>(MemoryRegionDramGpuCarveout.GetAddress() >> 0));
            reg::Write(MC + MC_SECURITY_CARVEOUT2_BOM_HI,                        static_cast<u32>(MemoryRegionDramGpuCarveout.GetAddress() >> BITSIZEOF(u32)));
            reg::Write(MC + MC_SECURITY_CARVEOUT2_SIZE_128KB,                    MemoryRegionDramGpuCarveout.GetSize() / 128_KB);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_ACCESS0,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_ACCESS1,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_ACCESS2,                MC_REG_BITS_ENUM (CLIENT_ACCESS2_GPUSRD,  ENABLE),
                                                                                 MC_REG_BITS_ENUM (CLIENT_ACCESS2_GPUSWR,  ENABLE),
                                                                                 MC_REG_BITS_ENUM (CLIENT_ACCESS2_TSECSRD, ENABLE));
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_ACCESS3,                0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_ACCESS4,                MC_REG_BITS_ENUM (CLIENT_ACCESS4_GPUSRD2, ENABLE),
                                                                                 MC_REG_BITS_ENUM (CLIENT_ACCESS4_GPUSWR2, ENABLE));
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS0, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS1, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS2, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS3, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS4, 0);
            reg::Write(MC + MC_SECURITY_CARVEOUT2_CFG0,                          MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_IS_WPR,                                 DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_FORCE_APERTURE_ID_MATCH,                 ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ALLOW_APERTURE_ID_MISMATCH,             DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_RD_EN,                        DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_WR_EN,                        DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_SEND_CFG_TO_GPU,                         ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL3, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL2, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL1, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL0, ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL3,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL2,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL1,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL0,  ENABLE_CHECKS),
                                                                                 MC_REG_BITS_VALUE(SECURITY_CARVEOUT_CFG0_APERTURE_ID,                                   2),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL3,                     ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL2,                     ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL1,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL0,                    DISABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL3,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL2,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL1,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL0,                      ENABLED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ADDRESS_TYPE,                  UNTRANSLATED_ONLY),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_LOCK_MODE,                                LOCKED),
                                                                                 MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_PROTECT_MODE,                     LOCKBIT_SECURE));
        }

        void DisableArc() {
            /* Configure IRAM top/bottom to point to memory ends (disabling redirection). */
            reg::Write(MC + MC_IRAM_BOM, MC_REG_BITS_VALUE(IRAM_BOM_IRAM_BOM, (~0u)));
            reg::Write(MC + MC_IRAM_TOM, MC_REG_BITS_VALUE(IRAM_TOM_IRAM_TOM, ( 0u)));

            /* Lock the IRAM aperture. */
            reg::ReadWrite(MC + MC_IRAM_REG_CTRL, MC_REG_BITS_ENUM(IRAM_REG_CTRL_IRAM_CFG_WRITE_ACCESS, DISABLED));

            /* Disable the ARC clock gate override. */
            reg::ReadWrite(CLK_RST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD, CLK_RST_REG_BITS_ENUM(LVL2_CLK_GATE_OVRD_ARC_CLK_OVR_ON, OFF));

            /* Read IRAM REG CTRL to make sure our writes take. */
            reg::Read(MC + MC_IRAM_REG_CTRL);
        }

        void DisableUntranslatedDeviceMemoryAccess() {
            /* If we can (mariko only), disable GMMU accesses that bypass the SMMU. */
            /* Additionally, force all untranslated acccesses to hit one of the carveouts. */
            if (GetSocType() == fuse::SocType_Mariko) {
                reg::Write(MC + MC_UNTRANSLATED_REGION_CHECK, MC_REG_BITS_ENUM(UNTRANSLATED_REGION_CHECK_UNTRANSLATED_REGION_CHECK_ACCESS,          DISABLED),
                                                              MC_REG_BITS_ENUM(UNTRANSLATED_REGION_CHECK_REQUIRE_UNTRANSLATED_CLIENTS_HIT_CARVEOUT,  ENABLED),
                                                              MC_REG_BITS_ENUM(UNTRANSLATED_REGION_CHECK_REQUIRE_UNTRANSLATED_GPU_HIT_CARVEOUT,      ENABLED));
            }
        }

        void FinalizeCarveoutSecureScratchRegisters() {
            /* Define carveout scratch values. */
            constexpr uintptr_t WarmbootCarveoutAddress = MemoryRegionDram.GetAddress();
            constexpr size_t    WarmbootCarveoutSize    = 128_KB;

            #define MC_ENABLE_CLIENT_ACCESS(INDEX, WHICH) MC_REG_BITS_ENUM(CLIENT_ACCESS##INDEX##_##WHICH, ENABLE)

            constexpr u32 WarmbootCarveoutClientAccess0     = reg::Encode(MC_ENABLE_CLIENT_ACCESS(0, AVPCARM7R),
                                                          MC_ENABLE_CLIENT_ACCESS(0, PPCSAHBSLVR));

            constexpr u32 WarmbootCarveoutClientAccess1     = reg::Encode(MC_ENABLE_CLIENT_ACCESS(1, AVPCARM7W));

            #undef MC_ENABLE_CLIENT_ACCESS

            constexpr u32 WarmbootCarveoutForceInternalAccess0     = reg::Encode(MC_REG_BITS_ENUM(CLIENT_ACCESS0_AVPCARM7R,   ENABLE),
                                                                 MC_REG_BITS_ENUM(CLIENT_ACCESS0_PPCSAHBSLVR, ENABLE));

            constexpr u32 WarmbootCarveoutForceInternalAccess1     = reg::Encode(MC_REG_BITS_ENUM(CLIENT_ACCESS1_AVPCARM7W, ENABLE));

            constexpr u32 WarmbootCarveoutConfig = reg::Encode(MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_IS_WPR,                                 DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_FORCE_APERTURE_ID_MATCH,                DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ALLOW_APERTURE_ID_MISMATCH,             DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_RD_EN,                        DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_WR_EN,                        DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_SEND_CFG_TO_GPU,                        DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL3, ENABLE_CHECKS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL2, ENABLE_CHECKS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL1, ENABLE_CHECKS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL0, ENABLE_CHECKS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL3,  ENABLE_CHECKS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL2,  ENABLE_CHECKS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL1,  ENABLE_CHECKS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL0,  ENABLE_CHECKS),
                                                               MC_REG_BITS_VALUE(SECURITY_CARVEOUT_CFG0_APERTURE_ID,                                   0),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL3,                    DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL2,                    DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL1,                    DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL0,                     ENABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL3,                     DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL2,                     DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL1,                     DISABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL0,                      ENABLED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ADDRESS_TYPE,                        ANY_ADDRESS),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_LOCK_MODE,                              UNLOCKED),
                                                               MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_PROTECT_MODE,                     LOCKBIT_SECURE));

            /* Save the carveout values into secure scratch. */

            /* Save MC_SECURITY_CARVEOUT4_BOM. */
            reg::ReadWrite(PMC + APBDEV_PMC_SECURE_SCRATCH51, REG_BITS_VALUE( 0, 15, WarmbootCarveoutAddress >> 17));

            /* Save MC_SECURITY_CARVEOUT4_BOM_HI. */
            reg::ReadWrite(PMC + APBDEV_PMC_SECURE_SCRATCH16, REG_BITS_VALUE(30,  2, WarmbootCarveoutAddress >> 32));

            /* Save MC_SECURITY_CARVEOUT4_SIZE_128KB. */
            reg::ReadWrite(PMC + APBDEV_PMC_SECURE_SCRATCH55, REG_BITS_VALUE(12, 12, WarmbootCarveoutSize / 128_KB));

            /* Save MC_SECURITY_CARVEOUT4_CLIENT_ACCESS. */
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH74, WarmbootCarveoutClientAccess0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH75, WarmbootCarveoutClientAccess1);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH76, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH77, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH78, 0);

            /* Save MC_SECURITY_CARVEOUT4_FORCE_INTERNAL_ACCESS. */
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH99,  WarmbootCarveoutForceInternalAccess0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH100, WarmbootCarveoutForceInternalAccess1);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH101, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH102, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH103, 0);

            /* Save MC_SECURITY_CARVEOUT4_CFG0. */
            reg::ReadWrite(PMC + APBDEV_PMC_SECURE_SCRATCH39, REG_BITS_VALUE(0, 27, WarmbootCarveoutConfig));
        }

        void EnableBpmpSmmu() {
            /* Define the ASID contents. */
            constexpr int       BpmpAsid    = 1;
            constexpr uintptr_t BpmpAsidPde = MemoryRegionPhysicalDeviceSecurityEngine.GetAddress();

            /* Configure the ASID. */
            reg::Write(MC + MC_SMMU_PTB_ASID, MC_REG_BITS_VALUE(SMMU_PTB_ASID_CURRENT_ASID,  BpmpAsid));

            reg::Write(MC + MC_SMMU_PTB_DATA, MC_REG_BITS_VALUE(SMMU_PTB_DATA_ASID_PDE_BASE,  BpmpAsidPde / 4_KB),
                                              MC_REG_BITS_ENUM (SMMU_PTB_DATA_ASID_NONSECURE,            DISABLE),
                                              MC_REG_BITS_ENUM (SMMU_PTB_DATA_ASID_WRITABLE,             DISABLE),
                                              MC_REG_BITS_ENUM (SMMU_PTB_DATA_ASID_READABLE,             DISABLE));

            /* Configure the BPMP and PPCS1 to use the asid. */
            reg::Write(MC + MC_SMMU_AVPC_ASID,  MC_REG_BITS_ENUM(SMMU_AVPC_ASID_AVPC_SMMU_ENABLE, ENABLE),   MC_REG_BITS_VALUE(SMMU_AVPC_ASID_AVPC_ASID,   BpmpAsid));
            reg::Write(MC + MC_SMMU_PPCS1_ASID, MC_REG_BITS_ENUM(SMMU_PPCS1_ASID_PPCS1_SMMU_ENABLE, ENABLE), MC_REG_BITS_VALUE(SMMU_PPCS1_ASID_PPCS1_ASID, BpmpAsid));

            /* Flush the entire page table cache, and read TLB_CONFIG to ensure the flush takes. */
            reg::Write(MC + MC_SMMU_PTC_FLUSH_0, 0);
            reg::Read (MC + MC_SMMU_TLB_CONFIG);

            /* Flush the entire translation lookaside buffer, and read TLB_CONFIG to ensure the flush takes. */
            reg::Write(MC + MC_SMMU_TLB_FLUSH, 0);
            reg::Read (MC + MC_SMMU_TLB_CONFIG);
        }

        void ValidateResetExpected() {
            /* We're coming out of reset, so check that we expected to come out of reset. */
            if (!IsResetExpected()) {
                secmon::SetError(pkg1::ErrorInfo_UnexpectedReset);
                AMS_ABORT("unexpected reset");
            }
            SetResetExpected(false);
        }

        void ActmonInterruptHandler() {
            SetError(pkg1::ErrorInfo_ActivityMonitorInterrupt);
            AMS_ABORT("actmon observed bpmp wakeup");
        }

        void ExitChargerHiZMode() {
            /* Setup I2c-1. */
            pinmux::SetupI2c1();
            clkrst::EnableI2c1Clock();

            /* Initialize I2c-1. */
            i2c::Initialize(i2c::Port_1);

            /* Exit Hi-Z mode. */
            charger::ExitHiZMode();

            /* Disable clock to I2c-1. */
            clkrst::DisableI2c1Clock();
        }

        bool IsExitLp0() {
            return reg::Read(MC + MC_SECURITY_CFG3) == 0;
        }

        void SetupLogForBoot() {
            log::Initialize(secmon::GetLogPort(), secmon::GetLogBaudRate(), secmon::GetLogFlags());
            log::SendText("OHAYO\n", 6);
            log::Flush();
        }

        void LogExitLp0() {
            /* NOTE: Nintendo only does this on dev, but we will always do it. */
            if (true /* !pkg1::IsProduction() */) {
                SetupLogForBoot();
            }
        }

        void SetupForLp0Exit() {
            /* Exit HiZ mode in charger, if we need to. */
            const auto target_fw = GetTargetFirmware();
            const bool force_exit_hiz_mode = (target_fw < TargetFirmware_4_0_0) || (target_fw < TargetFirmware_8_0_0 && fuse::GetHardwareType() == fuse::HardwareType_Icosa);
            if (force_exit_hiz_mode || smc::IsChargerHiZModeEnabled()) {
                ExitChargerHiZMode();
            }

            /* Refill the random cache, which is volatile and thus wiped on warmboot. */
            smc::FillRandomCache();

            /* Unlock the security engine. */
            secmon::smc::UnlockSecurityEngine();
        }

    }

    void Setup1() {
        /* Load the global configuration context. */
        InitializeConfigurationContext();

        /* Initialize uart for logging. */
        SetupLogForBoot();

        /* Initialize the security engine. */
        se::Initialize();

        /* Initialize the gic. */
        gic::InitializeCommon();
    }

    void Setup1ForWarmboot() {
        /* Initialize the security engine. */
        se::Initialize();

        /* Initialize the gic. */
        gic::InitializeCommon();
    }

    void SaveSecurityEngineAesKeySlotTestVector() {
        GenerateSecurityEngineAesKeySlotTestVector(g_se_aes_key_slot_test_vector, sizeof(g_se_aes_key_slot_test_vector));
    }

    void SetupSocSecurity() {
        /* Set the fuse visibility. */
        clkrst::SetFuseVisibility(true);

        /* Set fuses as only secure-writable. */
        fuse::SetWriteSecureOnly();

        /* Lockout the fuses. */
        fuse::Lockout();

        /* Set the security engine to secure mode. */
        se::SetSecure(true);

        /* Verify the security engine's sticky bits. */
        VerifySecurityEngineStickyBits();

        /* Verify the security engine's Aes slots contain correct contents. */
        VerifySecurityEngineAesKeySlotTestVector();

        /* Clear aes keyslots. */
        ClearAesKeySlots();

        /* Clear rsa keyslots. */
        ClearRsaKeySlots();

        /* Overwrite keys that we want to be random with random contents. */
        se::InitializeRandom();
        se::ConfigureAutomaticContextSave();
        se::SetRandomKey(pkg1::AesKeySlot_Temporary);
        se::GenerateSrk();
        se::SetRandomKey(pkg1::AesKeySlot_TzramSaveKek);

        /* Initialize pmc secure scratch. */
        if (GetSocType() == fuse::SocType_Erista) {
            pmc::InitializeRandomScratch();
        }
        pmc::LockSecureRegister(pmc::SecureRegister_Srk);

        /* Setup secure registers. */
        SetupSecureRegisters();

        /* Setup the smmu. */
        SetupSmmu();

        /* Clear the cpu reset vector. */
        reg::Write(EVP + EVP_CPU_RESET_VECTOR, 0);

        /* Configure the SB registers to our start address. */
        constexpr u32 ResetVectorLow  = static_cast<u32>((PhysicalTzramProgramResetVector >> 0));
        constexpr u32 ResetVectorHigh = static_cast<u32>((PhysicalTzramProgramResetVector >> BITSIZEOF(u32)));

        /* Write our reset vector to the secure boot registers. */
        reg::Write(secmon::MemoryRegionVirtualDeviceSystem.GetAddress() + SB_AA64_RESET_LOW,  ResetVectorLow | 1);
        reg::Write(secmon::MemoryRegionVirtualDeviceSystem.GetAddress() + SB_AA64_RESET_HIGH, ResetVectorHigh);

        /* Disable non-secure writes to the reset vector. */
        reg::Write(secmon::MemoryRegionVirtualDeviceSystem.GetAddress() + SB_CSR, SB_REG_BITS_ENUM(CSR_NS_RST_VEC_WR_DIS, DISABLE));

        /* Read back SB_CSR to make sure our non-secure write disable takes. */
        reg::Read(secmon::MemoryRegionVirtualDeviceSystem.GetAddress() + SB_CSR);

        /* Write our reset vector to scratch registers used by warmboot, and lock those scratch registers. */
        reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH34, ResetVectorLow);
        reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH35, ResetVectorHigh);
        pmc::LockSecureRegister(pmc::SecureRegister_ResetVector);

        /* Setup the security engine interrupt. */
        constexpr int SecurityEngineInterruptId = 90;
        constexpr u8  SecurityEngineInterruptCoreMask = (1 << 3);
        gic::SetPriority      (SecurityEngineInterruptId,            gic::HighestPriority);
        gic::SetInterruptGroup(SecurityEngineInterruptId,                               0);
        gic::SetEnable        (SecurityEngineInterruptId,                            true);
        gic::SetSpiTargetCpu  (SecurityEngineInterruptId, SecurityEngineInterruptCoreMask);
        gic::SetSpiMode       (SecurityEngineInterruptId,        gic::InterruptMode_Level);

        /* Setup the activity monitor interrupt. */
        constexpr int ActivityMonitorInterruptId = 77;
        constexpr u8  ActivityMonitorInterruptCoreMask = (1 << 3);
        gic::SetPriority      (ActivityMonitorInterruptId,            gic::HighestPriority);
        gic::SetInterruptGroup(ActivityMonitorInterruptId,                               0);
        gic::SetEnable        (ActivityMonitorInterruptId,                            true);
        gic::SetSpiTargetCpu  (ActivityMonitorInterruptId, ActivityMonitorInterruptCoreMask);
        gic::SetSpiMode       (ActivityMonitorInterruptId,        gic::InterruptMode_Level);

        /* Setup the mariko fatal error interrupt. */
        constexpr u8 MarikoFatalInterruptCoreMask = 0b1111;
        gic::SetPriority      (MarikoFatalErrorInterruptId,         gic::HighestPriority);
        gic::SetInterruptGroup(MarikoFatalErrorInterruptId,                            0);
        gic::SetEnable        (MarikoFatalErrorInterruptId,                         true);
        gic::SetSpiTargetCpu  (MarikoFatalErrorInterruptId,                            0);
        gic::SetSpiMode       (MarikoFatalErrorInterruptId,     gic::InterruptMode_Level);

        /* If we're coldboot, perform one-time setup. */
        if (g_is_cold_boot) {
            /* Register all interrupt handlers. */
            SetInterruptHandler(SecurityEngineInterruptId,   SecurityEngineInterruptCoreMask,      se::HandleInterrupt);
            SetInterruptHandler(ActivityMonitorInterruptId,  ActivityMonitorInterruptCoreMask, actmon::HandleInterrupt);
            SetInterruptHandler(MarikoFatalErrorInterruptId, MarikoFatalInterruptCoreMask,     secmon::HandleMarikoFatalErrorInterrupt);

            /* We're expecting the other cores to come out of reset. */
            for (int i = 1; i < NumCores; ++i) {
                SetResetExpected(i, true);
            }

            /* We only coldboot once. */
            g_is_cold_boot = false;
        }
    }

    void SetupSocSecurityWarmboot() {
        /* Check that we're allowed to continue. */
        ValidateResetExpected();

        /* Unmap the tzram identity mapping. */
        UnmapTzram();

        /* If we're exiting LP0, there's a little more work for us to do. */
        if (IsExitLp0()) {
            /* Log that we're exiting LP0. */
            LogExitLp0();

            /* Perform initial setup. */
            Setup1ForWarmboot();

            /* Generate a random srk. */
            se::GenerateSrk();

            /* Setup the Soc security. */
            SetupSocSecurity();

            /* Set the PMC and MC as secure-only. */
            SetupPmcAndMcSecure();

            /* Perform Lp0-exit specific init. */
            SetupForLp0Exit();

            /* Setup the Soc protections. */
            SetupSocProtections();
        }

        /* Perform remaining CPU initialization. */
        SetupCpuCoreContext();
        SetupCpuSErrorDebug();
    }

    void SetupSocProtections() {
        /* Setup the GPU carveout. */
        SetupGpuCarveout();

        /* Disable the ARC. */
        DisableArc();

        /* Disable untranslated memory accesses by devices. */
        DisableUntranslatedDeviceMemoryAccess();

        /* Further protections aren't applied on <= 1.0.0. */
        if (GetTargetFirmware() <= TargetFirmware_1_0_0) {
            return;
        }

        /* Finalize and lock the carveout scratch registers. */
        FinalizeCarveoutSecureScratchRegisters();
        pmc::LockSecureRegister(pmc::SecureRegister_Carveout);

        /* Clear all the BPMP exception vectors to a fixed value. */
        constexpr u32 BpmpExceptionVector = 0x7D000000;
        reg::Write(EVP + EVP_COP_RESET_VECTOR,          BpmpExceptionVector);
        reg::Write(EVP + EVP_COP_UNDEF_VECTOR,          BpmpExceptionVector);
        reg::Write(EVP + EVP_COP_SWI_VECTOR,            BpmpExceptionVector);
        reg::Write(EVP + EVP_COP_PREFETCH_ABORT_VECTOR, BpmpExceptionVector);
        reg::Write(EVP + EVP_COP_DATA_ABORT_VECTOR,     BpmpExceptionVector);
        reg::Write(EVP + EVP_COP_RSVD_VECTOR,           BpmpExceptionVector);
        reg::Write(EVP + EVP_COP_IRQ_VECTOR,            BpmpExceptionVector);
        reg::Write(EVP + EVP_COP_FIQ_VECTOR,            BpmpExceptionVector);

        /* Disable arbitration for the bpmp. */
        reg::ReadWrite(SYSTEM + AHB_ARBITRATION_DISABLE, AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_COP, DISABLE));

        /* Turn on the SMMU for the BPMP. */
        EnableBpmpSmmu();

        /* Wait until the flow controller reports that the BPMP is halted. */
        while (!reg::HasValue(FLOW_CTLR + FLOW_CTLR_HALT_COP_EVENTS, FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_MODE, FLOW_MODE_STOP))) {
            util::WaitMicroSeconds(1);
        }

        /* Enable clock to the activity monitor. */
        clkrst::EnableActmonClock();

        /* If JTAG is disabled, disable JTAG. */
        if (!secmon::IsJtagEnabled()) {
            reg::Write(FLOW_CTLR + FLOW_CTLR_HALT_COP_EVENTS, FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_MODE, FLOW_MODE_STOP),
                                                              FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_JTAG,       DISABLED));

            /* Turn on the activity monitor to prevent booting up the bpmp. */
            actmon::StartMonitoringBpmp(ActmonInterruptHandler);
        }
    }

    void SetupPmcAndMcSecure() {
        const auto target_fw = GetTargetFirmware();

        if (target_fw >= TargetFirmware_2_0_0) {
            /* Set the PMC secure. */
            reg::ReadWrite(APB_MISC + APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0, SLAVE_SECURITY_REG_BITS_ENUM(0, PMC, ENABLE));
        }

        if (target_fw >= TargetFirmware_4_0_0) {
            /* Set the MC secure. */
            reg::ReadWrite(APB_MISC + APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0, SLAVE_SECURITY_REG_BITS_ENUM(1, MC0, ENABLE),
                                                                                             SLAVE_SECURITY_REG_BITS_ENUM(1, MC1, ENABLE),
                                                                                             SLAVE_SECURITY_REG_BITS_ENUM(1, MCB, ENABLE));
        }
    }

    void SetupCpuCoreContext() {
        /* Get the tsc frequency. */
        const u32 tsc_frequency = reg::Read(MemoryRegionVirtualDeviceSysCtr0.GetAddress() + SYSCTR0_CNTFID0);

        /* Setup the secure EL2/EL1 system registers. */
        SetupSecureEl2AndEl1SystemRegisters();

        /* Setup the non-secure system registers. */
        SetupNonSecureSystemRegisters(tsc_frequency);

        /* Reset the cpu flow controller registers. */
        flow::ResetCpuRegisters(hw::GetCurrentCoreId());

        /* Initialize the core unique gic registers. */
        gic::InitializeCoreUnique();

        /* Configure cpu fiq. */
        constexpr int FiqInterruptId = 28;
        gic::SetPriority      (FiqInterruptId, gic::HighestPriority);
        gic::SetInterruptGroup(FiqInterruptId, 0);
        gic::SetEnable        (FiqInterruptId, true);

        /* Restore the cpu's debug registers. */
        RestoreDebugRegisters();
    }

    void SetupCpuSErrorDebug() {
        /* Get whether we should enable SError debug. */
        const auto &bc_data = secmon::GetBootConfig().data;
        const bool enabled  = bc_data.IsDevelopmentFunctionEnabled() && bc_data.IsSErrorDebugEnabled();

        /* Get and set scr_el3. */
        {
            util::BitPack32 scr;
            HW_CPU_GET_SCR_EL3(scr);

            scr.Set<hw::ScrEl3::Ea>(enabled ? 0 : 1);
            HW_CPU_SET_SCR_EL3(scr);
        }

        /* Prevent reordering instructions around this call. */
        hw::InstructionSynchronizationBarrier();
    }

    void SetKernelCarveoutRegion(int index, uintptr_t address, size_t size) {
        /* Configure the carveout. */
        auto &carveout = g_kernel_carveouts[index];
        carveout.address = address;
        carveout.size    = size;

        SetupKernelCarveouts();
    }

}