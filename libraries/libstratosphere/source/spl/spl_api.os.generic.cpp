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

namespace ams::spl {

    namespace {

        bool InitializeImpl() {
            /* Initialize implementation api. */
            impl::Initialize();
            return true;
        }

        void EnsureInitialized() {
            AMS_FUNCTION_LOCAL_STATIC(bool, s_initialized, InitializeImpl());
            AMS_ABORT_UNLESS(s_initialized);
        }

        Result WaitAvailableKeySlotAndExecute(auto f) {
            os::SystemEvent *event = nullptr;
            while (true) {
                R_TRY_CATCH(static_cast<::ams::Result>(f())) {
                    R_CATCH(spl::ResultNoAvailableKeySlot) {
                        if (event == nullptr) {
                            event = impl::GetAesKeySlotAvailableEvent();
                        }
                        event->Wait();
                        continue;
                    }
                } R_END_TRY_CATCH;

                R_SUCCEED();
            }
        }

    }

    Result AllocateAesKeySlot(s32 *out_slot) {
        EnsureInitialized();

        R_RETURN(WaitAvailableKeySlotAndExecute([&]() -> Result {
            R_RETURN(impl::AllocateAesKeySlot(out_slot));
        }));
    }

    Result DeallocateAesKeySlot(s32 slot) {
        EnsureInitialized();

        R_RETURN(impl::DeallocateAesKeySlot(slot));
    }

    Result GenerateAesKek(AccessKey *out_access_key, const void *key_source, size_t key_source_size, s32 generation, u32 option) {
        EnsureInitialized();

        /* Check key size (assumed valid). */
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        AMS_UNUSED(key_source_size);

        /* AccessKey *out_access_key, const KeySource &key_source, u32 generation, u32 option */
        R_RETURN(impl::GenerateAesKek(out_access_key, *static_cast<const KeySource *>(key_source), generation, option));
    }

    Result LoadAesKey(s32 slot, const AccessKey &access_key, const void *key_source, size_t key_source_size) {
        EnsureInitialized();

        AMS_ASSERT(key_source_size == sizeof(KeySource));
        AMS_UNUSED(key_source_size);

        R_RETURN(impl::LoadAesKey(slot, access_key, *static_cast<const KeySource *>(key_source)));
    }

    Result GenerateAesKey(void *dst, size_t dst_size, const AccessKey &access_key, const void *key_source, size_t key_source_size) {
        EnsureInitialized();

        AMS_ASSERT(dst_size >= sizeof(AesKey));
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        AMS_UNUSED(dst_size, key_source_size);

        R_RETURN(WaitAvailableKeySlotAndExecute([&]() -> Result {
            R_RETURN(impl::GenerateAesKey(static_cast<AesKey *>(dst), access_key, *static_cast<const KeySource *>(key_source)));
        }));
    }

    Result ComputeCtr(void *dst, size_t dst_size, s32 slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
        EnsureInitialized();

        AMS_ASSERT(iv_size >= sizeof(IvCtr));
        AMS_UNUSED(iv_size);
        AMS_ASSERT(dst_size >= src_size);

        R_RETURN(impl::ComputeCtr(dst, dst_size, slot, src, src_size, *static_cast<const IvCtr *>(iv)));
    }

    Result DecryptAesKey(void *dst, size_t dst_size, const void *key_source, size_t key_source_size, s32 generation, u32 option) {
        EnsureInitialized();

        AMS_ASSERT(dst_size >= crypto::AesEncryptor128::KeySize);
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        AMS_UNUSED(dst_size, key_source_size);

        R_RETURN(WaitAvailableKeySlotAndExecute([&]() -> Result {
            R_RETURN(impl::DecryptAesKey(static_cast<AesKey *>(dst), *static_cast<const KeySource *>(key_source), static_cast<u32>(generation), option));
        }));
    }

    Result LoadPreparedAesKey(s32 slot, const AccessKey &access_key) {
        EnsureInitialized();

        R_RETURN(impl::LoadPreparedAesKey(slot, access_key));
    }

    Result PrepareCommonEsTitleKey(AccessKey *out, const void *key_source, const size_t key_source_size, int generation) {
        EnsureInitialized();

        AMS_ASSERT(key_source_size == sizeof(KeySource));
        AMS_UNUSED(key_source_size);

        R_RETURN(impl::PrepareCommonEsTitleKey(out, *static_cast<const KeySource *>(key_source), generation));
    }

}
