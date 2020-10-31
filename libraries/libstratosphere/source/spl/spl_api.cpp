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

namespace ams::spl {

    namespace {

        enum class InitializeMode {
            None,
            General,
            Crypto,
            Ssl,
            Es,
            Fs,
            Manu
        };

        os::SdkMutex g_mutex;
        s32 g_initialize_count = 0;
        InitializeMode g_initialize_mode = InitializeMode::None;

        Result AllocateAesKeySlotImpl(s32 *out) {
            return serviceDispatchOut(splCryptoGetServiceSession(), 21, *out);
        }

        Result DeallocateAesKeySlotImpl(s32 slot) {
            return serviceDispatchIn(splCryptoGetServiceSession(), 22, slot);
        }

        Result GetAesKeySlotAvailableEventImpl(Handle *out) {
            return serviceDispatch(splCryptoGetServiceSession(), 23,
                .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
                .out_handles = out,
            );
        }

        void GetAesKeySlotAvailableEvent(os::SystemEvent *out) {
            /* Get libnx event. */
            Handle handle = svc::InvalidHandle;
            R_ABORT_UNLESS(GetAesKeySlotAvailableEventImpl(std::addressof(handle)));

            /* Attach to event. */
            out->AttachReadableHandle(handle, true, os::EventClearMode_ManualClear);
        }

        template<typename F>
        Result WaitAvailableKeySlotAndExecute(F f) {
            os::SystemEvent event;
            auto is_event_initialized = false;
            while (true) {
                R_TRY_CATCH(static_cast<::ams::Result>(f())) {
                    R_CATCH(spl::ResultOutOfKeySlots) {
                        if (!is_event_initialized) {
                            GetAesKeySlotAvailableEvent(std::addressof(event));
                            is_event_initialized = true;
                        }
                        event.Wait();
                        continue;
                    }
                } R_END_TRY_CATCH;

                return ResultSuccess();
            }
        }

        template<typename F>
        void Initialize(InitializeMode mode, F f) {
            std::scoped_lock lk(g_mutex);

            AMS_ASSERT(g_initialize_count >= 0);
            AMS_ABORT_UNLESS(mode != InitializeMode::None);

            if (g_initialize_count == 0) {
                AMS_ABORT_UNLESS(g_initialize_mode == InitializeMode::None);
                f();
                g_initialize_mode = mode;
            } else {
                AMS_ABORT_UNLESS(g_initialize_mode == mode);
            }

            ++g_initialize_count;
        }

    }

    void Initialize() {
        return Initialize(InitializeMode::General, [&]() {
            R_ABORT_UNLESS(splInitialize());
        });
    }

    void InitializeForCrypto() {
        return Initialize(InitializeMode::Crypto, [&]() {
            R_ABORT_UNLESS(splCryptoInitialize());
        });
    }

    void InitializeForSsl() {
        return Initialize(InitializeMode::Ssl, [&]() {
            R_ABORT_UNLESS(splSslInitialize());
        });
    }

    void InitializeForEs() {
        return Initialize(InitializeMode::Es, [&]() {
            R_ABORT_UNLESS(splEsInitialize());
        });
    }

    void InitializeForFs() {
        return Initialize(InitializeMode::Fs, [&]() {
            R_ABORT_UNLESS(splFsInitialize());
        });
    }

    void InitializeForManu() {
        return Initialize(InitializeMode::Manu, [&]() {
            R_ABORT_UNLESS(splManuInitialize());
        });
    }

    void Finalize() {
        std::scoped_lock lk(g_mutex);
        AMS_ASSERT(g_initialize_count > 0);
        AMS_ABORT_UNLESS(g_initialize_mode != InitializeMode::None);

        if ((--g_initialize_count) == 0) {
            switch (g_initialize_mode) {
                case InitializeMode::General: splExit();       break;
                case InitializeMode::Crypto:  splCryptoExit(); break;
                case InitializeMode::Ssl:     splSslExit();    break;
                case InitializeMode::Es:      splEsExit();     break;
                case InitializeMode::Fs:      splFsExit();     break;
                case InitializeMode::Manu:    splManuExit();   break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
            g_initialize_mode = InitializeMode::None;
        }
    }

    Result AllocateAesKeySlot(s32 *out_slot) {
        return WaitAvailableKeySlotAndExecute([&]() -> Result {
            return AllocateAesKeySlotImpl(out_slot);
        });
    }

    Result DeallocateAesKeySlot(s32 slot) {
        return DeallocateAesKeySlotImpl(slot);
    }

    Result GenerateAesKek(AccessKey *access_key, const void *key_source, size_t key_source_size, s32 generation, u32 option) {
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        return splCryptoGenerateAesKek(key_source, generation, option, static_cast<void *>(access_key));
    }

    Result LoadAesKey(s32 slot, const AccessKey &access_key, const void *key_source, size_t key_source_size) {
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        return splCryptoLoadAesKey(std::addressof(access_key), key_source, static_cast<u32>(slot));
    }

    Result GenerateAesKey(void *dst, size_t dst_size, const AccessKey &access_key, const void *key_source, size_t key_source_size) {
        AMS_ASSERT(dst_size >= crypto::AesEncryptor128::KeySize);
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        return WaitAvailableKeySlotAndExecute([&]() -> Result {
            return splCryptoGenerateAesKey(std::addressof(access_key), key_source, dst);
        });
    }

    Result GenerateSpecificAesKey(void *dst, size_t dst_size, const void *key_source, size_t key_source_size, s32 generation, u32 option) {
        AMS_ASSERT(dst_size >= crypto::AesEncryptor128::KeySize);
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        return splFsGenerateSpecificAesKey(key_source, static_cast<u32>(generation), option, dst);
    }

    Result ComputeCtr(void *dst, size_t dst_size, s32 slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
        AMS_ASSERT(iv_size >= 0x10);
        AMS_ASSERT(dst_size >= src_size);

        return splCryptoCryptAesCtr(src, dst, src_size, static_cast<s32>(slot), iv);
    }

    Result DecryptAesKey(void *dst, size_t dst_size, const void *key_source, size_t key_source_size, s32 generation, u32 option) {
        AMS_ASSERT(dst_size >= crypto::AesEncryptor128::KeySize);
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        return WaitAvailableKeySlotAndExecute([&]() -> Result {
            return splCryptoDecryptAesKey(key_source, static_cast<u32>(generation), option, dst);
        });
    }

    Result GetConfig(u64 *out, ConfigItem item) {
        return splGetConfig(static_cast<::SplConfigItem>(item), out);
    }

    bool IsDevelopment() {
        bool is_dev;
        R_ABORT_UNLESS(splIsDevelopment(std::addressof(is_dev)));
        return is_dev;
    }

    MemoryArrangement GetMemoryArrangement() {
        u64 mode = 0;
        R_ABORT_UNLESS(spl::GetConfig(std::addressof(mode), spl::ConfigItem::MemoryMode));
        switch (mode & 0x3F) {
            case 2:
                return MemoryArrangement_StandardForAppletDev;
            case 3:
                return MemoryArrangement_StandardForSystemDev;
            case 17:
                return MemoryArrangement_Expanded;
            case 18:
                return MemoryArrangement_ExpandedForAppletDev;
            default:
                return MemoryArrangement_Standard;
        }
    }

    Result SetBootReason(BootReasonValue boot_reason) {
        static_assert(sizeof(boot_reason) == sizeof(u32));

        u32 v;
        std::memcpy(std::addressof(v), std::addressof(boot_reason), sizeof(v));

        return splSetBootReason(v);
    }

    Result GetBootReason(BootReasonValue *out) {
        static_assert(sizeof(*out) == sizeof(u32));

        u32 v;
        R_TRY(splGetBootReason(std::addressof(v)));

        std::memcpy(out, std::addressof(v), sizeof(*out));
        return ResultSuccess();
    }

    SocType GetSocType() {
        switch (GetHardwareType()) {
            case HardwareType::Icosa:
            case HardwareType::Copper:
                return SocType_Erista;
            case HardwareType::Hoag:
            case HardwareType::Iowa:
            case HardwareType::_Five_:
                return SocType_Mariko;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result GetPackage2Hash(void *dst, size_t dst_size) {
        AMS_ASSERT(dst_size >= crypto::Sha256Generator::HashSize);
        return splFsGetPackage2Hash(dst);
    }

    Result GenerateRandomBytes(void *out, size_t buffer_size) {
        return splGetRandomBytes(out, buffer_size);
    }

    Result LoadPreparedAesKey(s32 slot, const AccessKey &access_key) {
        if (g_initialize_mode == InitializeMode::Fs) {
            return splFsLoadTitlekey(std::addressof(access_key), static_cast<u32>(slot));
        } else {
            /* TODO: libnx binding not available. */
            /* return splEsLoadTitlekey(std::addressof(access_key), static_cast<u32>(slot)); */
            AMS_ABORT_UNLESS(false);
        }
    }

}
