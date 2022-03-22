/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "spl_ctr_drbg.hpp"
#include "spl_device_address_mapper.hpp"
#include "spl_key_slot_cache.hpp"

namespace ams::hos {

    void InitializeVersionInternal(bool allow_approximate);

}

namespace ams::spl::impl {

    namespace {

        /* Drbg type. */
        using Drbg = CtrDrbg<crypto::AesEncryptor128, AesKeySize, false>;

        /* Convenient defines. */
        constexpr size_t DeviceAddressSpaceAlign = 4_MB;

        constexpr u32 WorkBufferBase       = 0x80000000u;
        constexpr u32 ComputeAesInMapBase  = 0x90000000u;
        constexpr u32 ComputeAesOutMapBase = 0xC0000000u;
        constexpr size_t ComputeAesSizeMax = static_cast<size_t>(ComputeAesOutMapBase - ComputeAesInMapBase);

        constexpr size_t DeviceUniqueDataIvSize       = 0x10;
        constexpr size_t DeviceUniqueDataPaddingSize  = 0x08;
        constexpr size_t DeviceUniqueDataDeviceIdSize = 0x08;
        constexpr size_t DeviceUniqueDataGmacSize     = 0x10;

        constexpr size_t DeviceUniqueDataPlainMetaDataSize = DeviceUniqueDataIvSize + DeviceUniqueDataGmacSize;
        constexpr size_t DeviceUniqueDataMetaDataSize      = DeviceUniqueDataPlainMetaDataSize + DeviceUniqueDataPaddingSize + DeviceUniqueDataDeviceIdSize;

        constexpr size_t Rsa2048BlockSize   = 0x100;
        constexpr size_t LabelDigestSizeMax = 0x20;

        constexpr size_t WorkBufferSizeMax = 0x800;

        constexpr const KeySource KeyGenerationSource = {
            .data = { 0x89, 0x61, 0x5E, 0xE0, 0x5C, 0x31, 0xB6, 0x80, 0x5F, 0xE5, 0x8F, 0x3D, 0xA2, 0x4F, 0x7A, 0xA8 }
        };

        constexpr const KeySource AesKeyDecryptionSource = {
            .data = { 0x11, 0x70, 0x24, 0x2B, 0x48, 0x69, 0x11, 0xF1, 0x11, 0xB0, 0x0C, 0x47, 0x7C, 0xC3, 0xEF, 0x7E }
        };

        constexpr s32 PhysicalAesKeySlotCount = 6;

        /* KeySlot management. */
        constinit AesKeySlotCache g_aes_keyslot_cache;
        constinit util::optional<AesKeySlotCacheEntry> g_aes_keyslot_cache_entry[PhysicalAesKeySlotCount];

        constinit bool g_is_physical_keyslot_allowed  = false;
        constinit bool g_is_modern_device_unique_data = true;

        constexpr inline bool IsVirtualAesKeySlot(s32 keyslot) {
            return AesKeySlotMin <= keyslot && keyslot <= AesKeySlotMax;
        }

        constexpr inline bool IsPhysicalAesKeySlot(s32 keyslot) {
            return keyslot < PhysicalAesKeySlotCount;
        }

        constexpr inline s32 GetVirtualAesKeySlotIndex(s32 keyslot) {
            AMS_ASSERT(IsVirtualAesKeySlot(keyslot));
            return keyslot - AesKeySlotMin;
        }

        constexpr inline s32 MakeVirtualAesKeySlot(s32 index) {
            const s32 virt_slot = index + AesKeySlotMin;
            AMS_ASSERT(IsVirtualKeySlot(virt_slot));
            return virt_slot;
        }

        enum class AesKeySlotContentType {
            None        = 0,
            AesKey      = 1,
            PreparedKey = 2,
        };

        struct AesKeySlotContents {
            AesKeySlotContentType type;
            union {
                struct {
                    AccessKey access_key;
                    KeySource key_source;
                } aes_key;
                struct {
                    AccessKey access_key;
                } prepared_key;
            };
        };

        constinit bool g_is_aes_keyslot_allocated[AesKeySlotCount];
        constinit AesKeySlotContents g_aes_keyslot_contents[AesKeySlotCount];
        constinit AesKeySlotContents g_aes_physical_keyslot_contents_for_backwards_compatibility[PhysicalAesKeySlotCount];

        void ClearPhysicalAesKeySlot(s32 keyslot) {
            AMS_ASSERT(IsPhysicalAesKeySlot(keyslot));

            AccessKey access_key = {};
            KeySource key_source = {};
            smc::LoadAesKey(keyslot, access_key, key_source);
        }

        s32 GetPhysicalAesKeySlot(s32 keyslot, bool load) {
            s32 phys_slot = -1;
            AesKeySlotContents *contents = nullptr;

            if (g_is_physical_keyslot_allowed && IsPhysicalAesKeySlot(keyslot)) {
                /* On 1.0.0, we allow the use of physical keyslots. */
                phys_slot = keyslot;
                contents = std::addressof(g_aes_physical_keyslot_contents_for_backwards_compatibility[phys_slot]);

                /* If the physical slot is already loaded, we're good. */
                if (g_aes_keyslot_cache.FindPhysical(phys_slot)) {
                    return phys_slot;
                }
            } else {
                /* This should be a virtual keyslot. */
                AMS_ASSERT(IsVirtualAesKeySlot(keyslot));

                /* Try to find a physical slot in the cache. */
                if (g_aes_keyslot_cache.Find(std::addressof(phys_slot), keyslot)) {
                    return phys_slot;
                }

                /* Allocate a physical slot. */
                phys_slot = g_aes_keyslot_cache.Allocate(keyslot);
                contents = std::addressof(g_aes_keyslot_contents[GetVirtualAesKeySlotIndex(keyslot)]);
            }

            /* Ensure the contents of the keyslot. */
            if (load) {
                switch (contents->type) {
                    case AesKeySlotContentType::None:
                        ClearPhysicalAesKeySlot(phys_slot);
                        break;
                    case AesKeySlotContentType::AesKey:
                        R_ABORT_UNLESS(smc::ConvertResult(smc::LoadAesKey(phys_slot, contents->aes_key.access_key, contents->aes_key.key_source)));
                        break;
                    case AesKeySlotContentType::PreparedKey:
                        R_ABORT_UNLESS(smc::ConvertResult(smc::LoadPreparedAesKey(phys_slot, contents->prepared_key.access_key)));
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            return phys_slot;
        }

        /* Type definitions. */
        class ScopedAesKeySlot {
            private:
                s32 m_slot_index;
                bool m_allocated;
            public:
                ScopedAesKeySlot() : m_slot_index(-1), m_allocated(false) {
                    /* ... */
                }
                ~ScopedAesKeySlot() {
                    if (m_allocated) {
                        DeallocateAesKeySlot(m_slot_index);
                    }
                }

                s32 GetIndex() const {
                    return m_slot_index;
                }

                Result Allocate() {
                    R_TRY(AllocateAesKeySlot(std::addressof(m_slot_index)));
                    m_allocated = true;
                    return ResultSuccess();
                }
        };

        struct SeLinkedListEntry {
            u32 num_entries;
            u32 address;
            u32 size;
        };

        struct SeCryptContext {
            SeLinkedListEntry in;
            SeLinkedListEntry out;
        };

        /* Global variables. */
        alignas(os::MemoryPageSize) constinit u8      g_work_buffer[WorkBufferSizeMax];
        constinit util::TypedStorage<Drbg>            g_drbg;
        constinit os::InterruptName                   g_interrupt_name;
        constinit os::InterruptEventType              g_interrupt;
        constinit util::TypedStorage<os::SystemEvent> g_aes_keyslot_available_event;
        constinit os::SdkMutex                        g_operation_lock;
        constinit dd::DeviceAddressSpaceType          g_device_address_space;
        constinit u32                                 g_work_buffer_mapped_address;

        constinit BootReasonValue                     g_boot_reason;
        constinit bool                                g_is_boot_reason_initialized;

        /* Initialization functionality. */
        void InitializeAsyncOperation() {
            u64 interrupt_number;
            impl::GetConfig(std::addressof(interrupt_number), ConfigItem::SecurityEngineInterruptNumber);
            g_interrupt_name = static_cast<os::InterruptName>(interrupt_number);

            os::InitializeInterruptEvent(std::addressof(g_interrupt), g_interrupt_name, os::EventClearMode_AutoClear);
        }

        void InitializeDeviceAddressSpace() {
            /* Create device address space. */
            R_ABORT_UNLESS(dd::CreateDeviceAddressSpace(std::addressof(g_device_address_space), 0, (1ul << 32)));

            /* Attach to the security engine. */
            R_ABORT_UNLESS(dd::AttachDeviceAddressSpace(std::addressof(g_device_address_space), dd::DeviceName_Se));

            /* Map work buffer into the device. */
            const uintptr_t work_buffer_address = reinterpret_cast<uintptr_t>(g_work_buffer);
            g_work_buffer_mapped_address = WorkBufferBase + (work_buffer_address % DeviceAddressSpaceAlign);

            R_ABORT_UNLESS(dd::MapDeviceAddressSpaceAligned(std::addressof(g_device_address_space), dd::GetCurrentProcessHandle(), work_buffer_address, dd::DeviceAddressSpaceMemoryRegionAlignment, g_work_buffer_mapped_address, dd::MemoryPermission_ReadWrite));
        }

        void InitializeCtrDrbg() {
            u8 seed[Drbg::SeedSize];
            AMS_ABORT_UNLESS(smc::GenerateRandomBytes(seed, sizeof(seed)) == smc::Result::Success);

            util::ConstructAt(g_drbg);
            util::GetReference(g_drbg).Initialize(seed, sizeof(seed), nullptr, 0, nullptr, 0);
        }

        void InitializeKeySlots() {
            const auto fw_ver = hos::GetVersion();
            g_is_physical_keyslot_allowed  = fw_ver <  hos::Version_2_0_0;
            g_is_modern_device_unique_data = fw_ver >= hos::Version_5_0_0;

            for (s32 i = 0; i < PhysicalAesKeySlotCount; i++) {
                g_aes_keyslot_cache_entry[i].emplace(i);
                g_aes_keyslot_cache.AddEntry(std::addressof(g_aes_keyslot_cache_entry[i].value()));
            }

            util::ConstructAt(g_aes_keyslot_available_event, os::EventClearMode_ManualClear, true);
            util::GetReference(g_aes_keyslot_available_event).Signal();
        }

        void WaitOperation() {
            os::WaitInterruptEvent(std::addressof(g_interrupt));
        }

        smc::Result WaitAndGetResult(smc::AsyncOperationKey op_key) {
            WaitOperation();

            smc::Result async_res;
            if (const smc::Result res = smc::GetResult(std::addressof(async_res), op_key); res != smc::Result::Success) {
                return res;
            }

            return async_res;
        }

        smc::Result WaitAndGetResultData(void *dst, size_t size, smc::AsyncOperationKey op_key) {
            WaitOperation();

            smc::Result async_res;
            if (const smc::Result res = smc::GetResultData(std::addressof(async_res), dst, size, op_key); res != smc::Result::Success) {
                return res;
            }

            return async_res;
        }

        smc::Result DecryptAes(void *dst, s32 keyslot, const void *src) {
            struct DecryptAesLayout {
                SeCryptContext crypt_ctx;
                u8 padding[8];
                u8 in_buffer[crypto::AesEncryptor128::BlockSize];
                u8 out_buffer[crypto::AesEncryptor128::BlockSize];
            };

            auto &layout = *reinterpret_cast<DecryptAesLayout *>(g_work_buffer);

            layout.crypt_ctx.in.num_entries  = 0;
            layout.crypt_ctx.in.address      = g_work_buffer_mapped_address + AMS_OFFSETOF(DecryptAesLayout, in_buffer);
            layout.crypt_ctx.in.size         = sizeof(layout.in_buffer);
            layout.crypt_ctx.out.num_entries = 0;
            layout.crypt_ctx.out.address     = g_work_buffer_mapped_address + AMS_OFFSETOF(DecryptAesLayout, out_buffer);
            layout.crypt_ctx.out.size        = sizeof(layout.out_buffer);

            std::memcpy(layout.in_buffer, src, sizeof(layout.in_buffer));

            os::FlushDataCache(std::addressof(layout), sizeof(layout));
            {
                std::scoped_lock lk(g_operation_lock);

                smc::AsyncOperationKey op_key;
                const IvCtr iv_ctr    = {};
                const u32 mode        = smc::GetComputeAesMode(smc::CipherMode::CbcDecrypt, GetPhysicalAesKeySlot(keyslot, true));
                const u32 dst_ll_addr = g_work_buffer_mapped_address + AMS_OFFSETOF(DecryptAesLayout, crypt_ctx.out);
                const u32 src_ll_addr = g_work_buffer_mapped_address + AMS_OFFSETOF(DecryptAesLayout, crypt_ctx.in);

                smc::Result res = smc::ComputeAes(std::addressof(op_key), dst_ll_addr, mode, iv_ctr, src_ll_addr, sizeof(layout.out_buffer));
                if (res != smc::Result::Success) {
                    return res;
                }

                res = WaitAndGetResult(op_key);
                if (res != smc::Result::Success) {
                    return res;
                }
            }
            os::FlushDataCache(std::addressof(layout.out_buffer), sizeof(layout.out_buffer));

            std::memcpy(dst, layout.out_buffer, sizeof(layout.out_buffer));

            return smc::Result::Success;
        }

        Result GenerateRandomBytesImpl(void *out, size_t size) {
            AMS_ASSERT(size <= Drbg::RequestSizeMax);

            if (!util::GetReference(g_drbg).Generate(out, size, nullptr, 0)) {
                /* We need to reseed. */
                {
                    u8 seed[Drbg::SeedSize];

                    if (smc::Result res = smc::GenerateRandomBytes(seed, sizeof(seed)); res != smc::Result::Success) {
                        return smc::ConvertResult(res);
                    }

                    util::GetReference(g_drbg).Reseed(seed, sizeof(seed), nullptr, 0);
                }
                util::GetReference(g_drbg).Generate(out, size, nullptr, 0);
            }

            return ResultSuccess();
        }

        Result DecryptAndStoreDeviceUniqueKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
            struct DecryptAndStoreDeviceUniqueKeyLayout {
                u8 data[DeviceUniqueDataMetaDataSize + 2 * Rsa2048BlockSize + 0x10];
            };
            auto &layout = *reinterpret_cast<DecryptAndStoreDeviceUniqueKeyLayout *>(g_work_buffer);

            /* Validate size. */
            R_UNLESS(src_size <= sizeof(DecryptAndStoreDeviceUniqueKeyLayout), spl::ResultInvalidBufferSize());

            std::memcpy(layout.data, src, src_size);

            if (g_is_modern_device_unique_data) {
                return smc::ConvertResult(smc::DecryptDeviceUniqueData(layout.data, src_size, access_key, key_source, static_cast<smc::DeviceUniqueDataMode>(option)));
            } else {
                return smc::ConvertResult(smc::DecryptAndStoreGcKey(layout.data, src_size, access_key, key_source, option));
            }
        }

        Result ModularExponentiateWithStorageKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size, smc::ModularExponentiateWithStorageKeyMode mode) {
            struct ModularExponentiateWithStorageKeyLayout {
                u8 base[Rsa2048BlockSize];
                u8 mod[Rsa2048BlockSize];
            };
            auto &layout = *reinterpret_cast<ModularExponentiateWithStorageKeyLayout *>(g_work_buffer);

            /* Validate sizes. */
            R_UNLESS(base_size <= sizeof(layout.base),   spl::ResultInvalidBufferSize());
            R_UNLESS(mod_size  <= sizeof(layout.mod),    spl::ResultInvalidBufferSize());
            R_UNLESS(out_size  <= sizeof(g_work_buffer), spl::ResultInvalidBufferSize());

            /* Copy data into work buffer. */
            const size_t base_ofs = sizeof(layout.base) - base_size;
            const size_t mod_ofs  = sizeof(layout.mod)  - mod_size;

            std::memset(layout.base, 0, sizeof(layout.base));
            std::memset(layout.mod,  0, sizeof(layout.mod));

            std::memcpy(layout.base + base_ofs, base, base_size);
            std::memcpy(layout.mod  + mod_ofs,  mod,  mod_size);

            /* Do exp mod operation. */
            {
                std::scoped_lock lk(g_operation_lock);

                smc::AsyncOperationKey op_key;
                smc::Result res = smc::ModularExponentiateWithStorageKey(std::addressof(op_key), layout.base, layout.mod, mode);
                if (res != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }

                res = WaitAndGetResultData(g_work_buffer, out_size, op_key);
                if (res != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }
            }

            /* Copy result. */
            if (out != g_work_buffer) {
                std::memcpy(out, g_work_buffer, out_size);
            }

            return ResultSuccess();
        }

        Result PrepareEsDeviceUniqueKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, smc::EsDeviceUniqueKeyType type, u32 generation) {
            struct PrepareEsDeviceUniqueKeyLayout {
                u8 base[Rsa2048BlockSize];
                u8 mod[Rsa2048BlockSize];
            };
            auto &layout = *reinterpret_cast<PrepareEsDeviceUniqueKeyLayout *>(g_work_buffer);

            /* Validate sizes. */
            R_UNLESS(base_size <= sizeof(layout.base),        spl::ResultInvalidBufferSize());
            R_UNLESS(mod_size <= sizeof(layout.mod),          spl::ResultInvalidBufferSize());
            R_UNLESS(label_digest_size <= LabelDigestSizeMax, spl::ResultInvalidBufferSize());

            /* Copy data into work buffer. */
            const size_t base_ofs = sizeof(layout.base) - base_size;
            const size_t mod_ofs  = sizeof(layout.mod)  - mod_size;

            std::memset(layout.base, 0, sizeof(layout.base));
            std::memset(layout.mod,  0, sizeof(layout.mod));

            std::memcpy(layout.base + base_ofs, base, base_size);
            std::memcpy(layout.mod  + mod_ofs,  mod,  mod_size);

            /* Do exp mod operation. */
            {
                std::scoped_lock lk(g_operation_lock);

                smc::AsyncOperationKey op_key;
                smc::Result res = smc::PrepareEsDeviceUniqueKey(std::addressof(op_key), layout.base, layout.mod, label_digest, label_digest_size, smc::GetPrepareEsDeviceUniqueKeyOption(type, generation));
                if (res != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }

                res = WaitAndGetResultData(g_work_buffer, sizeof(*out_access_key), op_key);
                if (res != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }
            }

            std::memcpy(out_access_key, g_work_buffer, sizeof(*out_access_key));
            return ResultSuccess();
        }

    }

    /* Initialization. */
    void Initialize() {
        /* Initialize async operation. */
        InitializeAsyncOperation();

        /* Initialize device address space for the SE. */
        InitializeDeviceAddressSpace();

        /* Initialize the Drbg. */
        InitializeCtrDrbg();

        /* Initialize the keyslot cache. */
        InitializeKeySlots();
    }

    /* General. */
    Result GetConfig(u64 *out, ConfigItem key) {
        /* Nintendo explicitly blacklists package2 hash, which must be gotten via bespoke api. */
        R_UNLESS(key != ConfigItem::Package2Hash, spl::ResultInvalidArgument());

        smc::Result res = smc::GetConfig(out, 1, key);

        /* Nintendo has some special handling here for hardware type/hardware state. */
        if (key == ConfigItem::HardwareType && res == smc::Result::InvalidArgument) {
            *out = static_cast<u64>(HardwareType::Icosa);
            res = smc::Result::Success;
        }
        if (key == ConfigItem::HardwareState && res == smc::Result::InvalidArgument) {
            *out = HardwareState_Development;
            res = smc::Result::Success;
        }

        return smc::ConvertResult(res);
    }

    Result ModularExponentiate(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size) {
        struct ModularExponentiateLayout {
            u8 base[Rsa2048BlockSize];
            u8 exp[Rsa2048BlockSize];
            u8 mod[Rsa2048BlockSize];
        };
        auto &layout = *reinterpret_cast<ModularExponentiateLayout *>(g_work_buffer);

        /* Validate sizes. */
        R_UNLESS(base_size <= sizeof(layout.base),   spl::ResultInvalidBufferSize());
        R_UNLESS(exp_size  <= sizeof(layout.exp),    spl::ResultInvalidBufferSize());
        R_UNLESS(mod_size  <= sizeof(layout.mod),    spl::ResultInvalidBufferSize());
        R_UNLESS(out_size  <= sizeof(g_work_buffer), spl::ResultInvalidBufferSize());

        /* Copy data into work buffer. */
        const size_t base_ofs = sizeof(layout.base) - base_size;
        const size_t mod_ofs  = sizeof(layout.mod)  - mod_size;

        std::memset(layout.base, 0, sizeof(layout.base));
        std::memset(layout.mod,  0, sizeof(layout.mod));

        std::memcpy(layout.base + base_ofs, base, base_size);
        std::memcpy(layout.mod  + mod_ofs,  mod,  mod_size);

        std::memcpy(layout.exp, exp, exp_size);

        /* Do exp mod operation. */
        {
            std::scoped_lock lk(g_operation_lock);

            smc::AsyncOperationKey op_key;
            smc::Result res = smc::ModularExponentiate(std::addressof(op_key), layout.base, layout.exp, exp_size, layout.mod);
            if (res != smc::Result::Success) {
                return smc::ConvertResult(res);
            }

            res = WaitAndGetResultData(g_work_buffer, out_size, op_key);
            if (res != smc::Result::Success) {
                return smc::ConvertResult(res);
            }
        }

        std::memcpy(out, g_work_buffer, out_size);
        return ResultSuccess();
    }

    Result SetConfig(ConfigItem key, u64 value) {
        R_TRY(smc::ConvertResult(smc::SetConfig(key, value)));

        /* Work around for temporary version. */
        if (key == ConfigItem::ExosphereApiVersion) {
            hos::InitializeVersionInternal(false);
        }
        return ResultSuccess();
    }

    Result GenerateRandomBytes(void *out, size_t size) {
        for (size_t offset = 0; offset < size; offset += Drbg::RequestSizeMax) {
            R_TRY(GenerateRandomBytesImpl(static_cast<u8 *>(out) + offset, std::min(size - offset, Drbg::RequestSizeMax)));
        }

        return ResultSuccess();
    }

    Result IsDevelopment(bool *out) {
        u64 hardware_state;
        R_TRY(impl::GetConfig(std::addressof(hardware_state), ConfigItem::HardwareState));

        *out = (hardware_state == HardwareState_Development);
        return ResultSuccess();
    }

    Result SetBootReason(BootReasonValue boot_reason) {
        R_UNLESS(!g_is_boot_reason_initialized, spl::ResultBootReasonAlreadyInitialized());

        g_boot_reason                = boot_reason;
        g_is_boot_reason_initialized = true;
        return ResultSuccess();
    }

    Result GetBootReason(BootReasonValue *out) {
        R_UNLESS(g_is_boot_reason_initialized, spl::ResultBootReasonNotInitialized());

        *out = g_boot_reason;
        return ResultSuccess();
    }

    /* Crypto. */
    Result GenerateAesKek(AccessKey *out_access_key, const KeySource &key_source, u32 generation, u32 option) {
        return smc::ConvertResult(smc::GenerateAesKek(out_access_key, key_source, generation, option));
    }

    Result LoadAesKey(s32 keyslot, const AccessKey &access_key, const KeySource &key_source) {
        /* Ensure we can load into the slot. */
        const s32 phys_slot = GetPhysicalAesKeySlot(keyslot, false);
        R_TRY(smc::ConvertResult(smc::LoadAesKey(phys_slot, access_key, key_source)));

        /* Update our contents. */
        const s32 index = GetVirtualAesKeySlotIndex(keyslot);

        g_aes_keyslot_contents[index].type               = AesKeySlotContentType::AesKey;
        g_aes_keyslot_contents[index].aes_key.access_key = access_key;
        g_aes_keyslot_contents[index].aes_key.key_source = key_source;

        return ResultSuccess();
    }

    Result GenerateAesKey(AesKey *out_key, const AccessKey &access_key, const KeySource &key_source) {
        ScopedAesKeySlot keyslot_holder;
        R_TRY(keyslot_holder.Allocate());

        R_TRY(LoadAesKey(keyslot_holder.GetIndex(), access_key, KeyGenerationSource));

        return smc::ConvertResult(DecryptAes(out_key, keyslot_holder.GetIndex(), std::addressof(key_source)));
    }

    Result DecryptAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 option) {
        AccessKey access_key;
        R_TRY(GenerateAesKek(std::addressof(access_key), AesKeyDecryptionSource, generation, option));

        return GenerateAesKey(out_key, access_key, key_source);
    }

    Result ComputeCtr(void *dst, size_t dst_size, s32 keyslot, const void *src, size_t src_size, const IvCtr &iv_ctr) {
        /* Succeed immediately if there's nothing to compute. */
        R_SUCCEED_IF(src_size == 0);

        /* Validate sizes. */
        R_UNLESS(src_size <= dst_size,                    spl::ResultInvalidBufferSize());
        R_UNLESS(util::IsAligned(src_size, AesBlockSize), spl::ResultInvalidBufferSize());

        /* We can only map 4_MB aligned buffers for the SE, so determine where to map our buffers. */
        const uintptr_t src_addr = reinterpret_cast<uintptr_t>(src);
        const uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);

        const uintptr_t src_addr_aligned = util::AlignDown(src_addr, dd::DeviceAddressSpaceMemoryRegionAlignment);
        const uintptr_t dst_addr_aligned = util::AlignDown(dst_addr, dd::DeviceAddressSpaceMemoryRegionAlignment);

        const size_t src_size_aligned = util::AlignUp(src_addr + src_size, dd::DeviceAddressSpaceMemoryRegionAlignment) - src_addr_aligned;
        const size_t dst_size_aligned = util::AlignUp(dst_addr + dst_size, dd::DeviceAddressSpaceMemoryRegionAlignment) - dst_addr_aligned;

        const u32 src_se_map_addr = ComputeAesInMapBase  + (src_addr_aligned % DeviceAddressSpaceAlign);
        const u32 dst_se_map_addr = ComputeAesOutMapBase + (dst_addr_aligned % DeviceAddressSpaceAlign);

        const u32 src_se_addr = ComputeAesInMapBase  + (src_addr % DeviceAddressSpaceAlign);
        const u32 dst_se_addr = ComputeAesOutMapBase + (dst_addr % DeviceAddressSpaceAlign);

        /* Validate aligned sizes. */
        R_UNLESS(src_size_aligned <= ComputeAesSizeMax, spl::ResultInvalidBufferSize());
        R_UNLESS(dst_size_aligned <= ComputeAesSizeMax, spl::ResultInvalidBufferSize());

        /* Helpers for mapping/unmapping. */
        DeviceAddressMapper src_mapper(std::addressof(g_device_address_space), src_addr_aligned, src_size_aligned, src_se_map_addr, dd::MemoryPermission_ReadOnly);
        DeviceAddressMapper dst_mapper(std::addressof(g_device_address_space), dst_addr_aligned, dst_size_aligned, dst_se_map_addr, dd::MemoryPermission_WriteOnly);

        /* Setup SE linked list entries. */
        auto &crypt_ctx = *reinterpret_cast<SeCryptContext *>(g_work_buffer);
        crypt_ctx.in.num_entries  = 0;
        crypt_ctx.in.address      = src_se_addr;
        crypt_ctx.in.size         = src_size;
        crypt_ctx.out.num_entries = 0;
        crypt_ctx.out.address     = dst_se_addr;
        crypt_ctx.out.size        = dst_size;

        os::FlushDataCache(std::addressof(crypt_ctx), sizeof(crypt_ctx));
        os::FlushDataCache(src, src_size);
        os::FlushDataCache(dst, dst_size);
        {
            std::scoped_lock lk(g_operation_lock);

            const u32 mode = smc::GetComputeAesMode(smc::CipherMode::Ctr, GetPhysicalAesKeySlot(keyslot, true));
            const u32 dst_ll_addr = g_work_buffer_mapped_address + AMS_OFFSETOF(SeCryptContext, out);
            const u32 src_ll_addr = g_work_buffer_mapped_address + AMS_OFFSETOF(SeCryptContext, in);

            smc::AsyncOperationKey op_key;
            smc::Result res = smc::ComputeAes(std::addressof(op_key), dst_ll_addr, mode, iv_ctr, src_ll_addr, src_size);
            if (res != smc::Result::Success) {
                return smc::ConvertResult(res);
            }

            res = WaitAndGetResult(op_key);
            if (res != smc::Result::Success) {
                return smc::ConvertResult(res);
            }
        }
        os::FlushDataCache(dst, dst_size);

        return ResultSuccess();
    }

    Result ComputeCmac(Cmac *out_cmac, s32 keyslot, const void *data, size_t size) {
        R_UNLESS(size <= sizeof(g_work_buffer), spl::ResultInvalidBufferSize());

        std::memcpy(g_work_buffer, data, size);
        return smc::ConvertResult(smc::ComputeCmac(out_cmac, GetPhysicalAesKeySlot(keyslot, true), g_work_buffer, size));
    }

    Result AllocateAesKeySlot(s32 *out_keyslot) {
        /* Find an unused keyslot. */
        for (s32 i = 0; i < AesKeySlotCount; ++i) {
            if (!g_is_aes_keyslot_allocated[i]) {
                g_is_aes_keyslot_allocated[i]  = true;
                g_aes_keyslot_contents[i].type = AesKeySlotContentType::None;
                *out_keyslot = MakeVirtualAesKeySlot(i);
                return ResultSuccess();
            }
        }

        util::GetReference(g_aes_keyslot_available_event).Clear();
        return spl::ResultNoAvailableKeySlot();
    }

    Result DeallocateAesKeySlot(s32 keyslot) {
        /* Only virtual keyslots can be freed. */
        R_UNLESS(IsVirtualAesKeySlot(keyslot), spl::ResultInvalidKeySlot());

        /* Check that the virtual keyslot is allocated. */
        const s32 index = GetVirtualAesKeySlotIndex(keyslot);
        R_UNLESS(g_is_aes_keyslot_allocated[index], spl::ResultInvalidKeySlot());

        /* Clear the physical keyslot, if we're cached. */
        s32 phys_slot;
        if (g_aes_keyslot_cache.Release(std::addressof(phys_slot), keyslot)) {
            ClearPhysicalAesKeySlot(phys_slot);
        }

        /* Clear the virtual keyslot. */
        g_aes_keyslot_contents[index].type = AesKeySlotContentType::None;
        g_is_aes_keyslot_allocated[index]  = false;

        util::GetReference(g_aes_keyslot_available_event).Signal();
        return ResultSuccess();
    }

    Result TestAesKeySlot(s32 *out_index, bool *out_virtual, s32 keyslot) {
        if (g_is_physical_keyslot_allowed && IsPhysicalAesKeySlot(keyslot)) {
            *out_index   = keyslot;
            *out_virtual = false;
            return ResultSuccess();
        }

        R_UNLESS(IsVirtualAesKeySlot(keyslot), spl::ResultInvalidKeySlot());

        const s32 index = GetVirtualAesKeySlotIndex(keyslot);
        R_UNLESS(g_is_aes_keyslot_allocated[index], spl::ResultInvalidKeySlot());

        *out_index   = index;
        *out_virtual = true;
        return ResultSuccess();
    }

    os::SystemEvent *GetAesKeySlotAvailableEvent() {
        return util::GetPointer(g_aes_keyslot_available_event);
    }

    /* RSA. */
    Result DecryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        struct DecryptDeviceUniqueDataLayout {
            u8 data[Rsa2048BlockSize + DeviceUniqueDataMetaDataSize];
        };
        auto &layout = *reinterpret_cast<DecryptDeviceUniqueDataLayout *>(g_work_buffer);

        /* Validate size. */
        R_UNLESS(src_size >= DeviceUniqueDataMetaDataSize,          spl::ResultInvalidBufferSize());
        R_UNLESS(src_size <= sizeof(DecryptDeviceUniqueDataLayout), spl::ResultInvalidBufferSize());

        std::memcpy(layout.data, src, src_size);

        smc::Result smc_res;
        size_t copy_size = 0;
        if (g_is_modern_device_unique_data) {
            copy_size = std::min(dst_size, src_size - DeviceUniqueDataMetaDataSize);
            smc_res   = smc::DecryptDeviceUniqueData(layout.data, src_size, access_key, key_source, static_cast<smc::DeviceUniqueDataMode>(option));
        } else {
            smc_res   = smc::DecryptDeviceUniqueData(std::addressof(copy_size), layout.data, src_size, access_key, key_source, option);
            copy_size = std::min(dst_size, copy_size);
        }

        if (smc_res == smc::Result::Success) {
            std::memcpy(dst, layout.data, copy_size);
        }

        return smc::ConvertResult(smc_res);
    }

    /* SSL */
    Result DecryptAndStoreSslClientCertKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source) {
        return DecryptAndStoreDeviceUniqueKey(src, src_size, access_key, key_source, static_cast<u32>(smc::DeviceUniqueDataMode::DecryptAndStoreSslKey));
    }

    Result ModularExponentiateWithSslClientCertKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size) {
        return ModularExponentiateWithStorageKey(out, out_size, base, base_size, mod, mod_size, smc::ModularExponentiateWithStorageKeyMode::Ssl);
    }

    /* ES */
    Result LoadEsDeviceKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        if (g_is_modern_device_unique_data) {
            return DecryptAndStoreDeviceUniqueKey(src, src_size, access_key, key_source, option);
        } else {
            struct LoadEsDeviceKeyLayout {
                u8 data[DeviceUniqueDataMetaDataSize + 2 * Rsa2048BlockSize + 0x10];
            };
            auto &layout = *reinterpret_cast<LoadEsDeviceKeyLayout *>(g_work_buffer);

            /* Validate size. */
            R_UNLESS(src_size <= sizeof(layout.data), spl::ResultInvalidBufferSize());

            std::memcpy(layout.data, src, src_size);

            return smc::ConvertResult(smc::LoadEsDeviceKey(layout.data, src_size, access_key, key_source, option));
        }
    }

    Result PrepareEsTitleKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation) {
        return PrepareEsDeviceUniqueKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, smc::EsDeviceUniqueKeyType::TitleKey, generation);
    }

    Result PrepareCommonEsTitleKey(AccessKey *out_access_key, const KeySource &key_source, u32 generation) {
        return smc::ConvertResult(smc::PrepareCommonEsTitleKey(out_access_key, key_source, generation));
    }

    Result DecryptAndStoreDrmDeviceCertKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source) {
        return DecryptAndStoreDeviceUniqueKey(src, src_size, access_key, key_source, static_cast<u32>(smc::DeviceUniqueDataMode::DecryptAndStoreDrmDeviceCertKey));
    }

    Result ModularExponentiateWithDrmDeviceCertKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size) {
        return ModularExponentiateWithStorageKey(out, out_size, base, base_size, mod, mod_size, smc::ModularExponentiateWithStorageKeyMode::DrmDeviceCert);
    }

    Result PrepareEsArchiveKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation) {
        return PrepareEsDeviceUniqueKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, smc::EsDeviceUniqueKeyType::ArchiveKey, generation);
    }

    /* FS */
    Result DecryptAndStoreGcKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        return DecryptAndStoreDeviceUniqueKey(src, src_size, access_key, key_source, option);
    }

    Result DecryptGcMessage(u32 *out_size, void *dst, size_t dst_size, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size) {
        /* Validate sizes. */
        R_UNLESS(dst_size <= sizeof(g_work_buffer),       spl::ResultInvalidBufferSize());
        R_UNLESS(label_digest_size == LabelDigestSizeMax, spl::ResultInvalidBufferSize());

        /* Nintendo doesn't check this result code, but we will. */
        R_TRY(ModularExponentiateWithStorageKey(g_work_buffer, Rsa2048BlockSize, base, base_size, mod, mod_size, smc::ModularExponentiateWithStorageKeyMode::Gc));

        const auto data_size = crypto::DecodeRsa2048OaepSha256(dst, dst_size, label_digest, label_digest_size, g_work_buffer, Rsa2048BlockSize);
        R_UNLESS(data_size > 0, spl::ResultDecryptionFailed());

        *out_size = static_cast<u32>(data_size);
        return ResultSuccess();
    }

    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 which) {
        return smc::ConvertResult(smc::GenerateSpecificAesKey(out_key, key_source, generation, which));
    }

    Result LoadPreparedAesKey(s32 keyslot, const AccessKey &access_key) {
        /* Ensure we can load into the slot. */
        const s32 phys_slot = GetPhysicalAesKeySlot(keyslot, false);
        R_TRY(smc::ConvertResult(smc::LoadPreparedAesKey(phys_slot, access_key)));

        /* Update our contents. */
        const s32 index = GetVirtualAesKeySlotIndex(keyslot);

        g_aes_keyslot_contents[index].type                    = AesKeySlotContentType::PreparedKey;
        g_aes_keyslot_contents[index].prepared_key.access_key = access_key;

        return ResultSuccess();
    }

    Result GetPackage2Hash(void *dst, const size_t size) {
        u64 hash[4];
        R_UNLESS(size >= sizeof(hash), spl::ResultInvalidBufferSize());

        const smc::Result smc_res = smc::GetConfig(hash, 4, ConfigItem::Package2Hash);
        if (smc_res != smc::Result::Success) {
            return smc::ConvertResult(smc_res);
        }

        std::memcpy(dst, hash, sizeof(hash));
        return ResultSuccess();
    }

    /* Manu. */
    Result ReencryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
        struct ReencryptDeviceUniqueDataLayout {
            u8 data[DeviceUniqueDataMetaDataSize + 2 * Rsa2048BlockSize + 0x10];
            AccessKey access_key_dec;
            KeySource source_dec;
            AccessKey access_key_enc;
            KeySource source_enc;
        };
        auto &layout = *reinterpret_cast<ReencryptDeviceUniqueDataLayout *>(g_work_buffer);

        /* Validate size. */
        R_UNLESS(src_size  > DeviceUniqueDataMetaDataSize, spl::ResultInvalidBufferSize());
        R_UNLESS(src_size <= sizeof(layout.data),          spl::ResultInvalidBufferSize());

        std::memcpy(layout.data, src, src_size);
        layout.access_key_dec = access_key_dec;
        layout.source_dec     = source_dec;
        layout.access_key_enc = access_key_enc;
        layout.source_enc     = source_enc;

        const smc::Result smc_res = smc::ReencryptDeviceUniqueData(layout.data, src_size, layout.access_key_dec, layout.source_dec, layout.access_key_enc, layout.source_enc, option);
        if (smc_res == smc::Result::Success) {
            std::memcpy(dst, layout.data, std::min(dst_size, src_size));
        }

        return smc::ConvertResult(smc_res);
    }

}
