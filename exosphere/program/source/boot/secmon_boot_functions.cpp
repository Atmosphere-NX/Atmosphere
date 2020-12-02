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
#include "../secmon_error.hpp"
#include "secmon_boot.hpp"
#include "secmon_boot_cache.hpp"
#include "secmon_boot_functions.hpp"

namespace ams::secmon::boot {

    namespace {

        constexpr inline uintptr_t SYSCTR0 = MemoryRegionVirtualDeviceSysCtr0.GetAddress();

        NOINLINE void DecryptPayload(uintptr_t dst, uintptr_t src, size_t size, const void *iv, size_t iv_size, u8 key_generation) {
            secmon::boot::DecryptPackage2(reinterpret_cast<void *>(dst), size, reinterpret_cast<void *>(src), size, secmon::boot::GetPackage2AesKey(), crypto::AesEncryptor128::KeySize, iv, iv_size, key_generation);
        }

        u32 GetChipId() {
            constexpr u32 ChipIdErista         = 0x210;
            constexpr u32 ChipIdMariko         = 0x214;

            switch (GetSocType()) {
                case fuse::SocType_Erista: return ChipIdErista;
                case fuse::SocType_Mariko: return ChipIdMariko;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    void CheckVerifyResult(bool verify_result, pkg1::ErrorInfo error_info, const char *message) {
        if (!verify_result) {
            secmon::SetError(error_info);
            AMS_ABORT(message);
        }
    }

    void ClearIramBootCode() {
        /* Clear the boot code image from where it was loaded in IRAM. */
        util::ClearMemory(MemoryRegionPhysicalIramBootCodeCode.GetPointer(), MemoryRegionPhysicalIramBootCodeCode.GetSize());
    }

    void ClearIramBootKeys() {
        /* Clear the boot keys from where they were loaded in IRAM. */
        util::ClearMemory(MemoryRegionPhysicalIramBootCodeKeys.GetPointer(), MemoryRegionPhysicalIramBootCodeKeys.GetSize());
    }

    void ClearIramDebugCode() {
        /* Clear the boot code image from where it was loaded in IRAM. */
        util::ClearMemory(MemoryRegionPhysicalDebugCode.GetPointer(), MemoryRegionPhysicalDebugCode.GetSize());
    }

    void WaitForNxBootloader(const pkg1::SecureMonitorParameters &params, pkg1::BootloaderState state) {
        /* Check NX Bootloader's state once per microsecond until it's advanced enough. */
        while (params.bootloader_state < state) {
            util::WaitMicroSeconds(1);
        }
    }

    void LoadBootConfig(const void *src) {
        pkg1::BootConfig * const dst = secmon::impl::GetBootConfigStorage();

        if (pkg1::IsProduction()) {
            std::memset(dst, 0, sizeof(*dst));
        } else {
            hw::FlushDataCache(src, sizeof(*dst));
            hw::DataSynchronizationBarrierInnerShareable();
            std::memcpy(dst, src, sizeof(*dst));
        }
    }

    void VerifyOrClearBootConfig() {
        /* On production hardware, the boot config is already cleared. */
        if (pkg1::IsProduction()) {
            return;
        }

        pkg1::BootConfig * const bc = secmon::impl::GetBootConfigStorage();

        /* Determine if the bc is valid for the device. */
        bool valid_for_device = false;
        {
            const bool valid_signature = secmon::boot::VerifyBootConfigSignature(*bc, secmon::boot::GetBootConfigRsaModulus(), se::RsaSize);
            if (valid_signature) {
                valid_for_device = secmon::boot::VerifyBootConfigEcid(*bc);
            }
        }

        /* If the boot config is not valid for the device, clear its signed data. */
        if (!valid_for_device) {
            util::ClearMemory(std::addressof(bc->signed_data), sizeof(bc->signed_data));
        }
    }

    void EnableTsc(u64 initial_tsc_value) {
        /* Write the initial value to the CNTCV registers. */
        const u32 lo = static_cast<u32>(initial_tsc_value >>  0);
        const u32 hi = static_cast<u32>(initial_tsc_value >> 32);

        reg::Write(SYSCTR0 + SYSCTR0_CNTCV0, lo);
        reg::Write(SYSCTR0 + SYSCTR0_CNTCV1, hi);

        /* Configure the system counter control register. */
        reg::Write(SYSCTR0 + SYSCTR0_CNTCR, SYSCTR0_REG_BITS_ENUM(CNTCR_HDBG, ENABLE),
                                            SYSCTR0_REG_BITS_ENUM(CNTCR_EN,   ENABLE));
    }

    void WriteGpuCarveoutMagicNumbers() {
        /* Define the magic numbers. */
        constexpr u32 GpuMagicNumber       = 0xC0EDBBCC;
        constexpr u32 SkuInfo              = 0x83;
        constexpr u32 HdcpMicroCodeVersion = 0x2;

        /* Get our pointers. */
        u32 *gpu_magic  = MemoryRegionDramGpuCarveout.GetEndPointer<u32>() - (0x004 / sizeof(*gpu_magic));
        u32 *tsec_magic = MemoryRegionDramGpuCarveout.GetEndPointer<u32>() - (0x100 / sizeof(*tsec_magic));

        /* Write the gpu magic number. */
        gpu_magic[0] = GpuMagicNumber;

        /* Write the tsec magic numbers. */
        tsec_magic[0] = SkuInfo;
        tsec_magic[1] = HdcpMicroCodeVersion;
        tsec_magic[2] = GetChipId();

        /* Flush the magic numbers. */
        hw::FlushDataCache(gpu_magic,  1 * sizeof(u32));
        hw::FlushDataCache(tsec_magic, 3 * sizeof(u32));
        hw::DataSynchronizationBarrierInnerShareable();
    }

    void UpdateBootConfigForPackage2Header(const pkg2::Package2Header &header) {
        /* Check for all-zeroes signature. */
        const bool is_unsigned = header.signature[0] == 0 && crypto::IsSameBytes(header.signature, header.signature + 1, sizeof(header.signature) - 1);
        secmon::impl::GetBootConfigStorage()->signed_data.SetPackage2SignatureVerificationDisabled(is_unsigned);

        /* Check for valid magic. */
        const bool is_decrypted = crypto::IsSameBytes(header.meta.magic, pkg2::Package2Meta::Magic::String, sizeof(header.meta.magic));
        secmon::impl::GetBootConfigStorage()->signed_data.SetPackage2EncryptionDisabled(is_decrypted);
    }

    void VerifyPackage2HeaderSignature(pkg2::Package2Header &header, bool verify) {
        const u8 * const mod  = secmon::boot::GetPackage2RsaModulus(pkg1::IsProductionForPublicKey());
        const size_t mod_size = se::RsaSize;
        if (verify) {
            CheckVerifyResult(secmon::boot::VerifyPackage2Signature(header, mod, mod_size), pkg1::ErrorInfo_InvalidPackage2Signature, "pkg2 sign FAIL");
        }
    }

    void DecryptPackage2Header(pkg2::Package2Meta *dst, const pkg2::Package2Meta &src, bool encrypted) {
        if (encrypted) {
            constexpr int IvSize = 0x10;

            /* Decrypt the header. */
            DecryptPackage2(dst, sizeof(*dst), std::addressof(src), sizeof(src), secmon::boot::GetPackage2AesKey(), crypto::AesEncryptor128::KeySize, std::addressof(src), IvSize, src.GetKeyGeneration());

            /* Copy back the iv, which encodes encrypted metadata. */
            std::memcpy(dst, std::addressof(src), IvSize);
        } else {
            std::memcpy(dst, std::addressof(src), sizeof(*dst));
        }
    }

    void VerifyPackage2Header(const pkg2::Package2Meta &meta) {
        /* Validate the metadata. */
        CheckVerifyResult(VerifyPackage2Meta(meta),    pkg1::ErrorInfo_InvalidPackage2Meta, "pkg2 meta FAIL");

        /* Validate the version. */
        CheckVerifyResult(VerifyPackage2Version(meta), pkg1::ErrorInfo_InvalidPackage2Version, "pkg2 version FAIL");
    }

    void DecryptAndLoadPackage2Payloads(uintptr_t dst, const pkg2::Package2Meta &meta, uintptr_t src, bool encrypted) {
        /* Get the key generation for crypto. */
        const u8 key_generation = meta.GetKeyGeneration();
        /* Decrypt or load each payload in order. */
        for (int i = 0; i < pkg2::PayloadCount; ++i) {
            AMS_SECMON_LOG("pkg2 payload[%d]: %09lx -> %09lx size=%08x\n", i, src, dst + meta.payload_offsets[i], meta.payload_sizes[i]);

            if (encrypted) {
                DecryptPayload(dst + meta.payload_offsets[i], src, meta.payload_sizes[i], meta.payload_ivs[i], sizeof(meta.payload_ivs[i]), key_generation);
            } else {
                std::memcpy(reinterpret_cast<void *>(dst + meta.payload_offsets[i]), reinterpret_cast<void *>(src), meta.payload_sizes[i]);
            }

            src += meta.payload_sizes[i];
        }
    }

}
