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
#include <stratosphere.hpp>
#include "spl_api_impl.hpp"
#include "spl_ctr_drbg.hpp"
#include "spl_key_slot_cache.hpp"

namespace ams::spl::impl {

    namespace {

        /* Convenient defines. */
        constexpr size_t DeviceAddressSpaceAlign = 0x400000;
        constexpr u32 WorkBufferMapBase    = 0x80000000u;
        constexpr u32 ComputeAesInMapBase  = 0x90000000u;
        constexpr u32 ComputeAesOutMapBase = 0xC0000000u;
        constexpr size_t ComputeAesSizeMax = static_cast<size_t>(ComputeAesOutMapBase - ComputeAesInMapBase);

        constexpr size_t RsaPrivateKeySize = 0x100;
        constexpr size_t DeviceUniqueDataMetaSize = 0x30;
        constexpr size_t LabelDigestSizeMax = 0x20;

        constexpr size_t WorkBufferSizeMax = 0x800;

        constexpr s32 MaxPhysicalAesKeySlots = 6;
        constexpr s32 MaxPhysicalAesKeySlotsDeprecated = 4;

        constexpr s32 MaxVirtualAesKeySlots = 9;

        /* KeySlot management. */
        KeySlotCache g_keyslot_cache;
        std::optional<KeySlotCacheEntry> g_keyslot_cache_entry[MaxPhysicalAesKeySlots];

        inline s32 GetMaxPhysicalKeySlots() {
            return (hos::GetVersion() >= hos::Version_6_0_0) ? MaxPhysicalAesKeySlots : MaxPhysicalAesKeySlotsDeprecated;
        }

        constexpr s32 VirtualKeySlotMin = 16;
        constexpr s32 VirtualKeySlotMax = VirtualKeySlotMin + MaxVirtualAesKeySlots - 1;

        constexpr inline bool IsVirtualKeySlot(s32 keyslot) {
            return VirtualKeySlotMin <= keyslot && keyslot <= VirtualKeySlotMax;
        }

        inline bool IsPhysicalKeySlot(s32 keyslot) {
            return keyslot < GetMaxPhysicalKeySlots();
        }

        constexpr inline s32 GetVirtualKeySlotIndex(s32 keyslot) {
            AMS_ASSERT(IsVirtualKeySlot(keyslot));
            return keyslot - VirtualKeySlotMin;
        }

        constexpr inline s32 MakeVirtualKeySlot(s32 index) {
            const s32 virt_slot = index + VirtualKeySlotMin;
            AMS_ASSERT(IsVirtualKeySlot(virt_slot));
            return virt_slot;
        }

        void InitializeKeySlotCache() {
            for (s32 i = 0; i < MaxPhysicalAesKeySlots; i++) {
                g_keyslot_cache_entry[i].emplace(i);
                g_keyslot_cache.AddEntry(std::addressof(g_keyslot_cache_entry[i].value()));
            }
        }

        enum class KeySlotContentType {
            None        = 0,
            AesKey      = 1,
            PreparedKey = 2,
        };

        struct KeySlotContents {
            KeySlotContentType type;
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

        const void *g_keyslot_owners[MaxVirtualAesKeySlots];
        KeySlotContents g_keyslot_contents[MaxVirtualAesKeySlots];
        KeySlotContents g_physical_keyslot_contents_for_backwards_compatibility[MaxPhysicalAesKeySlots];

        void ClearPhysicalKeySlot(s32 keyslot) {
            AMS_ASSERT(IsPhysicalKeySlot(keyslot));

            AccessKey access_key = {};
            KeySource key_source = {};
            smc::LoadAesKey(keyslot, access_key, key_source);
        }

        s32 GetPhysicalKeySlot(s32 keyslot, bool load) {
            s32 phys_slot = -1;
            KeySlotContents *contents = nullptr;

            if (hos::GetVersion() == hos::Version_1_0_0 && IsPhysicalKeySlot(keyslot)) {
                /* On 1.0.0, we allow the use of physical keyslots. */
                phys_slot = keyslot;
                contents = std::addressof(g_physical_keyslot_contents_for_backwards_compatibility[phys_slot]);

                /* If the physical slot is already loaded, we're good. */
                if (g_keyslot_cache.FindPhysical(phys_slot)) {
                    return phys_slot;
                }
            } else {
                /* This should be a virtual keyslot. */
                AMS_ASSERT(IsVirtualKeySlot(keyslot));

                /* Try to find a physical slot in the cache. */
                if (g_keyslot_cache.Find(std::addressof(phys_slot), keyslot)) {
                    return phys_slot;
                }

                /* Allocate a physical slot. */
                phys_slot = g_keyslot_cache.Allocate(keyslot);
                contents = std::addressof(g_keyslot_contents[GetVirtualKeySlotIndex(keyslot)]);
            }

            /* Ensure the contents of the keyslot. */
            if (load) {
                switch (contents->type) {
                    case KeySlotContentType::None:
                        ClearPhysicalKeySlot(phys_slot);
                        break;
                    case KeySlotContentType::AesKey:
                        R_ABORT_UNLESS(smc::ConvertResult(smc::LoadAesKey(phys_slot, contents->aes_key.access_key, contents->aes_key.key_source)));
                        break;
                    case KeySlotContentType::PreparedKey:
                        R_ABORT_UNLESS(smc::ConvertResult(smc::LoadPreparedAesKey(phys_slot, contents->prepared_key.access_key)));
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            return phys_slot;
        }

        Result LoadVirtualAesKey(s32 keyslot, const AccessKey &access_key, const KeySource &key_source) {
            /* Ensure we can load into the slot. */
            const s32 phys_slot = GetPhysicalKeySlot(keyslot, false);
            R_TRY(smc::ConvertResult(smc::LoadAesKey(phys_slot, access_key, key_source)));

            /* Update our contents. */
            const s32 index = GetVirtualKeySlotIndex(keyslot);

            g_keyslot_contents[index].type               = KeySlotContentType::AesKey;
            g_keyslot_contents[index].aes_key.access_key = access_key;
            g_keyslot_contents[index].aes_key.key_source = key_source;

            return ResultSuccess();
        }

        Result LoadVirtualPreparedAesKey(s32 keyslot, const AccessKey &access_key) {
            /* Ensure we can load into the slot. */
            const s32 phys_slot = GetPhysicalKeySlot(keyslot, false);
            R_TRY(smc::ConvertResult(smc::LoadPreparedAesKey(phys_slot, access_key)));

            /* Update our contents. */
            const s32 index = GetVirtualKeySlotIndex(keyslot);

            g_keyslot_contents[index].type                    = KeySlotContentType::PreparedKey;
            g_keyslot_contents[index].prepared_key.access_key = access_key;

            return ResultSuccess();
        }

        /* Type definitions. */
        class ScopedAesKeySlot {
            private:
                s32 slot;
                bool has_slot;
            public:
                ScopedAesKeySlot() : slot(-1), has_slot(false) {
                    /* ... */
                }
                ~ScopedAesKeySlot() {
                    if (this->has_slot) {
                        DeallocateAesKeySlot(slot, this);
                    }
                }

                u32 GetKeySlot() const {
                    return this->slot;
                }

                Result Allocate() {
                    R_TRY(AllocateAesKeySlot(&this->slot, this));
                    this->has_slot = true;
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

        class DeviceAddressSpaceMapHelper {
            private:
                Handle das_hnd;
                u64 dst_addr;
                u64 src_addr;
                size_t size;
                u32 perm;
            public:
                DeviceAddressSpaceMapHelper(Handle h, u64 dst, u64 src, size_t sz, u32 p) : das_hnd(h), dst_addr(dst), src_addr(src), size(sz), perm(p) {
                    R_ABORT_UNLESS(svcMapDeviceAddressSpaceAligned(this->das_hnd, dd::GetCurrentProcessHandle(), this->src_addr, this->size, this->dst_addr, this->perm));
                }
                ~DeviceAddressSpaceMapHelper() {
                    R_ABORT_UNLESS(svcUnmapDeviceAddressSpace(this->das_hnd, dd::GetCurrentProcessHandle(), this->src_addr, this->size, this->dst_addr));
                }
        };

        /* Global variables. */
        CtrDrbg g_drbg;
        os::InterruptEventType g_se_event;
        os::SystemEventType g_se_keyslot_available_event;

        Handle g_se_das_hnd;
        u32 g_se_mapped_work_buffer_addr;
        alignas(os::MemoryPageSize) u8 g_work_buffer[2 * WorkBufferSizeMax];

        os::Mutex g_async_op_lock(false);

        BootReasonValue g_boot_reason;
        bool g_boot_reason_set;

        /* Boot Reason accessors. */
        BootReasonValue GetBootReason() {
            return g_boot_reason;
        }

        bool IsBootReasonSet() {
            return g_boot_reason_set;
        }

        /* Initialization functionality. */
        void InitializeCtrDrbg() {
            u8 seed[CtrDrbg::SeedSize];
            AMS_ABORT_UNLESS(smc::GenerateRandomBytes(seed, sizeof(seed)) == smc::Result::Success);

            g_drbg.Initialize(seed);
        }

        void InitializeSeEvents() {
            u64 irq_num;
            AMS_ABORT_UNLESS(smc::GetConfig(&irq_num, 1, ConfigItem::SecurityEngineInterruptNumber) == smc::Result::Success);
            os::InitializeInterruptEvent(std::addressof(g_se_event), irq_num, os::EventClearMode_AutoClear);

            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_se_keyslot_available_event), os::EventClearMode_AutoClear, true));
            os::SignalSystemEvent(std::addressof(g_se_keyslot_available_event));
        }

        void InitializeDeviceAddressSpace() {

            /* Create Address Space. */
            R_ABORT_UNLESS(svcCreateDeviceAddressSpace(&g_se_das_hnd, 0, (1ul << 32)));

            /* Attach it to the SE. */
            R_ABORT_UNLESS(svcAttachDeviceAddressSpace(svc::DeviceName_Se, g_se_das_hnd));

            const u64 work_buffer_addr = reinterpret_cast<u64>(g_work_buffer);
            g_se_mapped_work_buffer_addr = WorkBufferMapBase + (work_buffer_addr % DeviceAddressSpaceAlign);

            /* Map the work buffer for the SE. */
            R_ABORT_UNLESS(svcMapDeviceAddressSpaceAligned(g_se_das_hnd, dd::GetCurrentProcessHandle(), work_buffer_addr, sizeof(g_work_buffer), g_se_mapped_work_buffer_addr, 3));
        }

        /* Internal RNG functionality. */
        Result GenerateRandomBytesInternal(void *out, size_t size) {
            if (!g_drbg.GenerateRandomBytes(out, size)) {
                /* We need to reseed. */
                {
                    u8 seed[CtrDrbg::SeedSize];

                    smc::Result res = smc::GenerateRandomBytes(seed, sizeof(seed));
                    if (res != smc::Result::Success) {
                        return smc::ConvertResult(res);
                    }

                    g_drbg.Reseed(seed);
                    g_drbg.GenerateRandomBytes(out, size);
                }
            }

            return ResultSuccess();
        }

        /* Internal async implementation functionality. */
        void WaitSeOperationComplete() {
            os::WaitInterruptEvent(std::addressof(g_se_event));
        }

        smc::Result WaitCheckStatus(smc::AsyncOperationKey op_key) {
            WaitSeOperationComplete();

            smc::Result op_res;
            smc::Result res = smc::GetResult(&op_res, op_key);
            if (res != smc::Result::Success) {
                return res;
            }

            return op_res;
        }

        smc::Result WaitGetResult(void *out_buf, size_t out_buf_size, smc::AsyncOperationKey op_key) {
            WaitSeOperationComplete();

            smc::Result op_res;
            smc::Result res = smc::GetResultData(&op_res, out_buf, out_buf_size, op_key);
            if (res != smc::Result::Success) {
                return res;
            }

            return op_res;
        }

        /* Internal KeySlot utility. */
        Result ValidateAesKeySlot(s32 keyslot, const void *owner) {
            /* Allow the use of physical keyslots on 1.0.0. */
            if (hos::GetVersion() == hos::Version_1_0_0) {
                R_SUCCEED_IF(IsPhysicalKeySlot(keyslot));
            }

            R_UNLESS(IsVirtualKeySlot(keyslot), spl::ResultInvalidKeySlot());

            const s32 index = GetVirtualKeySlotIndex(keyslot);
            R_UNLESS(g_keyslot_owners[index] == owner, spl::ResultInvalidKeySlot());
            return ResultSuccess();
        }

        /* Helper to do a single AES block decryption. */
        smc::Result DecryptAesBlock(s32 keyslot, void *dst, const void *src) {
            struct DecryptAesBlockLayout {
                SeCryptContext crypt_ctx;
                u8 in_block[AES_BLOCK_SIZE] __attribute__((aligned(AES_BLOCK_SIZE)));
                u8 out_block[AES_BLOCK_SIZE] __attribute__((aligned(AES_BLOCK_SIZE)));
            };
            DecryptAesBlockLayout *layout = reinterpret_cast<DecryptAesBlockLayout *>(g_work_buffer);

            layout->crypt_ctx.in.num_entries = 0;
            layout->crypt_ctx.in.address = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, in_block);
            layout->crypt_ctx.in.size = sizeof(layout->in_block);
            layout->crypt_ctx.out.num_entries = 0;
            layout->crypt_ctx.out.address = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, out_block);
            layout->crypt_ctx.out.size = sizeof(layout->out_block);

            std::memcpy(layout->in_block, src, sizeof(layout->in_block));

            armDCacheFlush(layout, sizeof(*layout));
            {
                std::scoped_lock lk(g_async_op_lock);
                smc::AsyncOperationKey op_key;
                const IvCtr iv_ctr = {};
                const u32 mode = smc::GetComputeAesMode(smc::CipherMode::CbcDecrypt, GetPhysicalKeySlot(keyslot, true));
                const u32 dst_ll_addr = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, crypt_ctx.out);
                const u32 src_ll_addr = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, crypt_ctx.in);

                smc::Result res = smc::ComputeAes(&op_key, mode, iv_ctr, dst_ll_addr, src_ll_addr, sizeof(layout->in_block));
                if (res != smc::Result::Success) {
                    return res;
                }

                if ((res = WaitCheckStatus(op_key)) != smc::Result::Success) {
                    return res;
                }
            }
            armDCacheFlush(layout, sizeof(*layout));

            std::memcpy(dst, layout->out_block, sizeof(layout->out_block));
            return smc::Result::Success;
        }

        /* Implementation wrappers for API commands. */
        Result DecryptAndStoreDeviceUniqueKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
            struct DecryptAndStoreDeviceUniqueKeyLayout {
                u8 data[DeviceUniqueDataMetaSize + 2 * RsaPrivateKeySize + 0x10];
            };
            DecryptAndStoreDeviceUniqueKeyLayout *layout = reinterpret_cast<DecryptAndStoreDeviceUniqueKeyLayout *>(g_work_buffer);

            /* Validate size. */
            R_UNLESS(src_size <= sizeof(DecryptAndStoreDeviceUniqueKeyLayout), spl::ResultInvalidSize());
            std::memcpy(layout, src, src_size);

            armDCacheFlush(layout, sizeof(*layout));
            smc::Result smc_res;
            if (hos::GetVersion() >= hos::Version_5_0_0) {
                smc_res = smc::DecryptDeviceUniqueData(layout->data, src_size, access_key, key_source, static_cast<smc::DeviceUniqueDataMode>(option));
            } else {
                smc_res = smc::DecryptAndStoreGcKey(layout->data, src_size, access_key, key_source, option);
            }

            return smc::ConvertResult(smc_res);
        }

        Result ModularExponentiateWithStorageKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size, smc::ModularExponentiateWithStorageKeyMode mode) {
            struct ModularExponentiateWithStorageKeyLayout {
                u8 base[0x100];
                u8 mod[0x100];
            };
            ModularExponentiateWithStorageKeyLayout *layout = reinterpret_cast<ModularExponentiateWithStorageKeyLayout *>(g_work_buffer);

            /* Validate sizes. */
            R_UNLESS(base_size <= sizeof(layout->base), spl::ResultInvalidSize());
            R_UNLESS(mod_size <= sizeof(layout->mod), spl::ResultInvalidSize());
            R_UNLESS(out_size <= WorkBufferSizeMax, spl::ResultInvalidSize());

            /* Copy data into work buffer. */
            const size_t base_ofs = sizeof(layout->base) - base_size;
            const size_t mod_ofs = sizeof(layout->mod) - mod_size;
            std::memset(layout, 0, sizeof(*layout));
            std::memcpy(layout->base + base_ofs, base, base_size);
            std::memcpy(layout->mod + mod_ofs, mod, mod_size);

            /* Do exp mod operation. */
            armDCacheFlush(layout, sizeof(*layout));
            {
                std::scoped_lock lk(g_async_op_lock);
                smc::AsyncOperationKey op_key;

                smc::Result res = smc::ModularExponentiateWithStorageKey(&op_key, layout->base, layout->mod, mode);
                if (res != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }

                if ((res = WaitGetResult(g_work_buffer, out_size, op_key)) != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }
            }
            armDCacheFlush(g_work_buffer, sizeof(out_size));

            std::memcpy(out, g_work_buffer, out_size);
            return ResultSuccess();
        }

        Result PrepareEsDeviceUniqueKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation, smc::EsCommonKeyType type) {
            struct PrepareEsDeviceUniqueKeyLayout {
                u8 base[0x100];
                u8 mod[0x100];
            };
            PrepareEsDeviceUniqueKeyLayout *layout = reinterpret_cast<PrepareEsDeviceUniqueKeyLayout *>(g_work_buffer);

            /* Validate sizes. */
            R_UNLESS(base_size <= sizeof(layout->base), spl::ResultInvalidSize());
            R_UNLESS(mod_size <= sizeof(layout->mod), spl::ResultInvalidSize());
            R_UNLESS(label_digest_size <= LabelDigestSizeMax, spl::ResultInvalidSize());

            /* Copy data into work buffer. */
            const size_t base_ofs = sizeof(layout->base) - base_size;
            const size_t mod_ofs = sizeof(layout->mod) - mod_size;
            std::memset(layout, 0, sizeof(*layout));
            std::memcpy(layout->base + base_ofs, base, base_size);
            std::memcpy(layout->mod + mod_ofs, mod, mod_size);

            /* Do exp mod operation. */
            armDCacheFlush(layout, sizeof(*layout));
            {
                std::scoped_lock lk(g_async_op_lock);
                smc::AsyncOperationKey op_key;

                smc::Result res = smc::PrepareEsDeviceUniqueKey(&op_key, layout->base, layout->mod, label_digest, label_digest_size, smc::GetPrepareEsDeviceUniqueKeyOption(type, generation));
                if (res != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }

                if ((res = WaitGetResult(g_work_buffer, sizeof(*out_access_key), op_key)) != smc::Result::Success) {
                    return smc::ConvertResult(res);
                }
            }
            armDCacheFlush(g_work_buffer, sizeof(*out_access_key));

            std::memcpy(out_access_key, g_work_buffer, sizeof(*out_access_key));
            return ResultSuccess();
        }


    }

    /* Initialization. */
    void Initialize() {
        /* Initialize the Drbg. */
        InitializeCtrDrbg();
        /* Initialize SE interrupt + keyslot events. */
        InitializeSeEvents();
        /* Initialize DAS for the SE. */
        InitializeDeviceAddressSpace();
        /* Initialize the keyslot cache. */
        InitializeKeySlotCache();
    }

    /* General. */
    Result GetConfig(u64 *out, ConfigItem which) {
        /* Nintendo explicitly blacklists package2 hash here, amusingly. */
        /* This is not blacklisted in safemode, but we're never in safe mode... */
        R_UNLESS(which != ConfigItem::Package2Hash, spl::ResultInvalidArgument());

        smc::Result res = smc::GetConfig(out, 1, which);

        /* Nintendo has some special handling here for hardware type/is_retail. */
        if (res == smc::Result::InvalidArgument) {
            switch (which) {
                case ConfigItem::HardwareType:
                    *out = static_cast<u64>(HardwareType::Icosa);
                    res = smc::Result::Success;
                    break;
                case ConfigItem::HardwareState:
                    *out = HardwareState_Development;
                    res = smc::Result::Success;
                    break;
                default:
                    break;
            }
        }

        return smc::ConvertResult(res);
    }

    Result ModularExponentiate(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size) {
        struct ModularExponentiateLayout {
            u8 base[0x100];
            u8 exp[0x100];
            u8 mod[0x100];
        };
        ModularExponentiateLayout *layout = reinterpret_cast<ModularExponentiateLayout *>(g_work_buffer);

        /* Validate sizes. */
        R_UNLESS(base_size <= sizeof(layout->base), spl::ResultInvalidSize());
        R_UNLESS(exp_size <= sizeof(layout->exp),   spl::ResultInvalidSize());
        R_UNLESS(mod_size <= sizeof(layout->mod),   spl::ResultInvalidSize());
        R_UNLESS(out_size <= WorkBufferSizeMax,     spl::ResultInvalidSize());

        /* Copy data into work buffer. */
        const size_t base_ofs = sizeof(layout->base) - base_size;
        const size_t mod_ofs = sizeof(layout->mod) - mod_size;
        std::memset(layout, 0, sizeof(*layout));
        std::memcpy(layout->base + base_ofs, base, base_size);
        std::memcpy(layout->exp, exp, exp_size);
        std::memcpy(layout->mod + mod_ofs, mod, mod_size);

        /* Do exp mod operation. */
        armDCacheFlush(layout, sizeof(*layout));
        {
            std::scoped_lock lk(g_async_op_lock);
            smc::AsyncOperationKey op_key;

            smc::Result res = smc::ModularExponentiate(&op_key, layout->base, layout->exp, exp_size, layout->mod);
            if (res != smc::Result::Success) {
                return smc::ConvertResult(res);
            }

            if ((res = WaitGetResult(g_work_buffer, out_size, op_key)) != smc::Result::Success) {
                return smc::ConvertResult(res);
            }
        }
        armDCacheFlush(g_work_buffer, sizeof(out_size));

        std::memcpy(out, g_work_buffer, out_size);
        return ResultSuccess();
    }

    Result SetConfig(ConfigItem which, u64 value) {
        return smc::ConvertResult(smc::SetConfig(which, &value, 1));
    }

    Result GenerateRandomBytes(void *out, size_t size) {
        u8 *cur_dst = reinterpret_cast<u8 *>(out);

        for (size_t ofs = 0; ofs < size; ofs += CtrDrbg::MaxRequestSize) {
            const size_t cur_size = std::min(size - ofs, CtrDrbg::MaxRequestSize);

            R_TRY(GenerateRandomBytesInternal(cur_dst, size));
            cur_dst += cur_size;
        }

        return ResultSuccess();
    }

    Result IsDevelopment(bool *out) {
        u64 hardware_state;
        R_TRY(impl::GetConfig(&hardware_state, ConfigItem::HardwareState));

        *out = (hardware_state == HardwareState_Development);
        return ResultSuccess();
    }

    Result SetBootReason(BootReasonValue boot_reason) {
        R_UNLESS(!IsBootReasonSet(), spl::ResultBootReasonAlreadySet());

        g_boot_reason = boot_reason;
        g_boot_reason_set = true;
        return ResultSuccess();
    }

    Result GetBootReason(BootReasonValue *out) {
        R_UNLESS(IsBootReasonSet(), spl::ResultBootReasonNotSet());

        *out = GetBootReason();
        return ResultSuccess();
    }

    /* Crypto. */
    Result GenerateAesKek(AccessKey *out_access_key, const KeySource &key_source, u32 generation, u32 option) {
        return smc::ConvertResult(smc::GenerateAesKek(out_access_key, key_source, generation, option));
    }

    Result LoadAesKey(s32 keyslot, const void *owner, const AccessKey &access_key, const KeySource &key_source) {
        R_TRY(ValidateAesKeySlot(keyslot, owner));
        return LoadVirtualAesKey(keyslot, access_key, key_source);
    }

    Result GenerateAesKey(AesKey *out_key, const AccessKey &access_key, const KeySource &key_source) {
        static constexpr KeySource s_generate_aes_key_source = {
            .data = {0x89, 0x61, 0x5E, 0xE0, 0x5C, 0x31, 0xB6, 0x80, 0x5F, 0xE5, 0x8F, 0x3D, 0xA2, 0x4F, 0x7A, 0xA8}
        };

        ScopedAesKeySlot keyslot_holder;
        R_TRY(keyslot_holder.Allocate());

        R_TRY(LoadVirtualAesKey(keyslot_holder.GetKeySlot(), access_key, s_generate_aes_key_source));

        return smc::ConvertResult(DecryptAesBlock(keyslot_holder.GetKeySlot(), out_key, &key_source));
    }

    Result DecryptAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 option) {
        static constexpr KeySource s_decrypt_aes_key_source = {
            .data = {0x11, 0x70, 0x24, 0x2B, 0x48, 0x69, 0x11, 0xF1, 0x11, 0xB0, 0x0C, 0x47, 0x7C, 0xC3, 0xEF, 0x7E}
        };

        AccessKey access_key;
        R_TRY(GenerateAesKek(&access_key, s_decrypt_aes_key_source, generation, option));

        return GenerateAesKey(out_key, access_key, key_source);
    }

    Result ComputeCtr(void *dst, size_t dst_size, s32 keyslot, const void *owner, const void *src, size_t src_size, const IvCtr &iv_ctr) {
        R_TRY(ValidateAesKeySlot(keyslot, owner));

        /* Succeed immediately if there's nothing to crypt. */
        if (src_size == 0) {
            return ResultSuccess();
        }

        /* Validate sizes. */
        R_UNLESS(src_size <= dst_size,                      spl::ResultInvalidSize());
        R_UNLESS(util::IsAligned(src_size, AES_BLOCK_SIZE), spl::ResultInvalidSize());

        /* We can only map 0x400000 aligned buffers for the SE. With that in mind, we have some math to do. */
        const uintptr_t src_addr = reinterpret_cast<uintptr_t>(src);
        const uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);
        const uintptr_t src_addr_page_aligned = util::AlignDown(src_addr, os::MemoryPageSize);
        const uintptr_t dst_addr_page_aligned = util::AlignDown(dst_addr, os::MemoryPageSize);
        const size_t src_size_page_aligned = util::AlignUp(src_addr + src_size, os::MemoryPageSize) - src_addr_page_aligned;
        const size_t dst_size_page_aligned = util::AlignUp(dst_addr + dst_size, os::MemoryPageSize) - dst_addr_page_aligned;
        const u32 src_se_map_addr = ComputeAesInMapBase + (src_addr_page_aligned % DeviceAddressSpaceAlign);
        const u32 dst_se_map_addr = ComputeAesOutMapBase + (dst_addr_page_aligned % DeviceAddressSpaceAlign);
        const u32 src_se_addr = ComputeAesInMapBase + (src_addr % DeviceAddressSpaceAlign);
        const u32 dst_se_addr = ComputeAesOutMapBase + (dst_addr % DeviceAddressSpaceAlign);

        /* Validate aligned sizes. */
        R_UNLESS(src_size_page_aligned <= ComputeAesSizeMax, spl::ResultInvalidSize());
        R_UNLESS(dst_size_page_aligned <= ComputeAesSizeMax, spl::ResultInvalidSize());

        /* Helpers for mapping/unmapping. */
        DeviceAddressSpaceMapHelper in_mapper(g_se_das_hnd,  src_se_map_addr, src_addr_page_aligned, src_size_page_aligned, 1);
        DeviceAddressSpaceMapHelper out_mapper(g_se_das_hnd, dst_se_map_addr, dst_addr_page_aligned, dst_size_page_aligned, 2);

        /* Setup SE linked list entries. */
        SeCryptContext *crypt_ctx = reinterpret_cast<SeCryptContext *>(g_work_buffer);
        crypt_ctx->in.num_entries = 0;
        crypt_ctx->in.address = src_se_addr;
        crypt_ctx->in.size = src_size;
        crypt_ctx->out.num_entries = 0;
        crypt_ctx->out.address = dst_se_addr;
        crypt_ctx->out.size = dst_size;

        armDCacheFlush(crypt_ctx, sizeof(*crypt_ctx));
        armDCacheFlush(const_cast<void *>(src), src_size);
        armDCacheFlush(dst, dst_size);
        {
            std::scoped_lock lk(g_async_op_lock);
            smc::AsyncOperationKey op_key;
            const u32 mode = smc::GetComputeAesMode(smc::CipherMode::Ctr, GetPhysicalKeySlot(keyslot, true));
            const u32 dst_ll_addr = g_se_mapped_work_buffer_addr + offsetof(SeCryptContext, out);
            const u32 src_ll_addr = g_se_mapped_work_buffer_addr + offsetof(SeCryptContext, in);

            smc::Result res = smc::ComputeAes(&op_key, mode, iv_ctr, dst_ll_addr, src_ll_addr, src_size);
            if (res != smc::Result::Success) {
                return smc::ConvertResult(res);
            }

            if ((res = WaitCheckStatus(op_key)) != smc::Result::Success) {
                return smc::ConvertResult(res);
            }
        }
        armDCacheFlush(dst, dst_size);

        return ResultSuccess();
    }

    Result ComputeCmac(Cmac *out_cmac, s32 keyslot, const void *owner, const void *data, size_t size) {
        R_TRY(ValidateAesKeySlot(keyslot, owner));

        R_UNLESS(size <= WorkBufferSizeMax, spl::ResultInvalidSize());

        std::memcpy(g_work_buffer, data, size);
        return smc::ConvertResult(smc::ComputeCmac(out_cmac, GetPhysicalKeySlot(keyslot, true), g_work_buffer, size));
    }

    Result AllocateAesKeySlot(s32 *out_keyslot, const void *owner) {
        /* Find a virtual keyslot. */
        for (s32 i = 0; i < MaxVirtualAesKeySlots; i++) {
            if (g_keyslot_owners[i] == nullptr) {
                g_keyslot_owners[i]   = owner;
                g_keyslot_contents[i] = { .type = KeySlotContentType::None };
                *out_keyslot = MakeVirtualKeySlot(i);
                return ResultSuccess();
            }
        }

        os::ClearSystemEvent(std::addressof(g_se_keyslot_available_event));
        return spl::ResultOutOfKeySlots();
    }

    Result DeallocateAesKeySlot(s32 keyslot, const void *owner) {
        /* Only virtual keyslots can be freed. */
        R_UNLESS(IsVirtualKeySlot(keyslot), spl::ResultInvalidKeySlot());

        /* Ensure the keyslot is owned. */
        R_TRY(ValidateAesKeySlot(keyslot, owner));

        /* Clear the physical keyslot, if we're cached. */
        s32 phys_slot;
        if (g_keyslot_cache.Release(std::addressof(phys_slot), keyslot)) {
            ClearPhysicalKeySlot(phys_slot);
        }

        /* Clear the virtual keyslot. */
        const auto index               = GetVirtualKeySlotIndex(keyslot);
        g_keyslot_owners[index]        = nullptr;
        g_keyslot_contents[index].type = KeySlotContentType::None;

        os::SignalSystemEvent(std::addressof(g_se_keyslot_available_event));
        return ResultSuccess();
    }

    /* RSA. */
    Result DecryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        struct DecryptDeviceUniqueDataLayout {
            u8 data[RsaPrivateKeySize + DeviceUniqueDataMetaSize];
        };
        DecryptDeviceUniqueDataLayout *layout = reinterpret_cast<DecryptDeviceUniqueDataLayout *>(g_work_buffer);

        /* Validate size. */
        R_UNLESS(src_size >= DeviceUniqueDataMetaSize,              spl::ResultInvalidSize());
        R_UNLESS(src_size <= sizeof(DecryptDeviceUniqueDataLayout), spl::ResultInvalidSize());

        std::memcpy(layout->data, src, src_size);
        armDCacheFlush(layout, sizeof(*layout));

        smc::Result smc_res;
        size_t copy_size = 0;
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            copy_size = std::min(dst_size, src_size - DeviceUniqueDataMetaSize);
            smc_res = smc::DecryptDeviceUniqueData(layout->data, src_size, access_key, key_source, static_cast<smc::DeviceUniqueDataMode>(option));
        } else {
            smc_res = smc::DecryptDeviceUniqueData(&copy_size, layout->data, src_size, access_key, key_source, option);
            copy_size = std::min(dst_size, copy_size);
        }

        armDCacheFlush(layout, sizeof(*layout));
        if (smc_res == smc::Result::Success) {
            std::memcpy(dst, layout->data, copy_size);
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
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            return DecryptAndStoreDeviceUniqueKey(src, src_size, access_key, key_source, option);
        } else {
            struct LoadEsDeviceKeyLayout {
                u8 data[DeviceUniqueDataMetaSize + 2 * RsaPrivateKeySize + 0x10];
            };
            LoadEsDeviceKeyLayout *layout = reinterpret_cast<LoadEsDeviceKeyLayout *>(g_work_buffer);

            /* Validate size. */
            R_UNLESS(src_size <= sizeof(LoadEsDeviceKeyLayout), spl::ResultInvalidSize());

            std::memcpy(layout, src, src_size);

            armDCacheFlush(layout, sizeof(*layout));
            return smc::ConvertResult(smc::LoadEsDeviceKey(layout->data, src_size, access_key, key_source, option));
        }
    }

    Result PrepareEsTitleKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation) {
        return PrepareEsDeviceUniqueKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, generation, smc::EsCommonKeyType::TitleKey);
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
        return PrepareEsDeviceUniqueKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, generation, smc::EsCommonKeyType::ArchiveKey);
    }

    /* FS */
    Result DecryptAndStoreGcKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        return DecryptAndStoreDeviceUniqueKey(src, src_size, access_key, key_source, option);
    }

    Result DecryptGcMessage(u32 *out_size, void *dst, size_t dst_size, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size) {
        /* Validate sizes. */
        R_UNLESS(dst_size <= WorkBufferSizeMax,           spl::ResultInvalidSize());
        R_UNLESS(label_digest_size == LabelDigestSizeMax, spl::ResultInvalidSize());

        /* Nintendo doesn't check this result code, but we will. */
        R_TRY(ModularExponentiateWithStorageKey(g_work_buffer, 0x100, base, base_size, mod, mod_size, smc::ModularExponentiateWithStorageKeyMode::Gc));

        size_t data_size = crypto::DecodeRsa2048OaepSha256(dst, dst_size, label_digest, label_digest_size, g_work_buffer, 0x100);
        R_UNLESS(data_size > 0, spl::ResultDecryptionFailed());

        *out_size = static_cast<u32>(data_size);
        return ResultSuccess();
    }

    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 which) {
        return smc::ConvertResult(smc::GenerateSpecificAesKey(out_key, key_source, generation, which));
    }

    Result LoadPreparedAesKey(s32 keyslot, const void *owner, const AccessKey &access_key) {
        R_TRY(ValidateAesKeySlot(keyslot, owner));
        return LoadVirtualPreparedAesKey(keyslot, access_key);
    }

    Result GetPackage2Hash(void *dst, const size_t size) {
        u64 hash[4];
        R_UNLESS(size >= sizeof(hash), spl::ResultInvalidSize());

        smc::Result smc_res;
        if ((smc_res = smc::GetConfig(hash, 4, ConfigItem::Package2Hash)) != smc::Result::Success) {
            return smc::ConvertResult(smc_res);
        }

        std::memcpy(dst, hash, sizeof(hash));
        return ResultSuccess();
    }

    /* Manu. */
    Result ReencryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
        struct ReencryptDeviceUniqueDataLayout {
            u8 data[DeviceUniqueDataMetaSize + 2 * RsaPrivateKeySize + 0x10];
            AccessKey access_key_dec;
            KeySource source_dec;
            AccessKey access_key_enc;
            KeySource source_enc;
        };
        ReencryptDeviceUniqueDataLayout *layout = reinterpret_cast<ReencryptDeviceUniqueDataLayout *>(g_work_buffer);

        /* Validate size. */
        R_UNLESS(src_size >= DeviceUniqueDataMetaSize, spl::ResultInvalidSize());
        R_UNLESS(src_size <= sizeof(ReencryptDeviceUniqueDataLayout), spl::ResultInvalidSize());

        std::memcpy(layout, src, src_size);
        layout->access_key_dec = access_key_dec;
        layout->source_dec = source_dec;
        layout->access_key_enc = access_key_enc;
        layout->source_enc = source_enc;

        armDCacheFlush(layout, sizeof(*layout));

        smc::Result smc_res = smc::ReencryptDeviceUniqueData(layout->data, src_size, layout->access_key_dec, layout->source_dec, layout->access_key_enc, layout->source_enc, option);
        if (smc_res == smc::Result::Success) {
            size_t copy_size = std::min(dst_size, src_size);
            armDCacheFlush(layout, copy_size);
            std::memcpy(dst, layout->data, copy_size);
        }

        return smc::ConvertResult(smc_res);
    }

    /* Helper. */
    Result DeallocateAllAesKeySlots(const void *owner) {
        for (s32 slot = VirtualKeySlotMin; slot <= VirtualKeySlotMax; ++slot) {
            if (g_keyslot_owners[GetVirtualKeySlotIndex(slot)] == owner) {
                DeallocateAesKeySlot(slot, owner);
            }
        }
        return ResultSuccess();
    }

    Handle GetAesKeySlotAvailableEventHandle() {
        return os::GetReadableHandleOfSystemEvent(std::addressof(g_se_keyslot_available_event));
    }

}
