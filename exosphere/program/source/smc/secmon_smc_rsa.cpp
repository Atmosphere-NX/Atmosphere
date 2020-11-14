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
#include "../secmon_key_storage.hpp"
#include "../secmon_page_mapper.hpp"
#include "secmon_smc_aes.hpp"
#include "secmon_smc_rsa.hpp"
#include "secmon_smc_se_lock.hpp"

namespace ams::secmon::smc {

    namespace {

        struct ModularExponentiateByStorageKeyOption {
            using Mode     = util::BitPack32::Field<0,  2, u32>;
            using Reserved = util::BitPack32::Field<2, 30, u32>;
        };

        struct PrepareEsDeviceUniqueKeyOption {
            using KeyGeneration = util::BitPack32::Field<0,  6, int>;
            using Type          = util::BitPack32::Field<6,  1, EsCommonKeyType>;
            using Reserved      = util::BitPack32::Field<7, 25, u32>;
        };

        constexpr const u8 ModularExponentiateByStorageKeyTable[] = {
            static_cast<u8>(ImportRsaKey_Lotus),
            static_cast<u8>(ImportRsaKey_Ssl),
            static_cast<u8>(ImportRsaKey_EsClientCert),
        };
        constexpr size_t ModularExponentiateByStorageKeyTableSize = util::size(ModularExponentiateByStorageKeyTable);

        consteval u32 GetModeForImportRsaKey(ImportRsaKey import_key) {
            for (size_t i = 0; i < ModularExponentiateByStorageKeyTableSize; ++i) {
                if (static_cast<ImportRsaKey>(ModularExponentiateByStorageKeyTable[i]) == import_key) {
                    return i;
                }
            }

            AMS_ASSUME(false);
        }

        class PrepareEsDeviceUniqueKeyAsyncArguments {
            private:
                int generation;
                EsCommonKeyType type;
                u8 label_digest[crypto::Sha256Generator::HashSize];
            public:
                void Set(int gen, EsCommonKeyType t, const u8 ld[crypto::Sha256Generator::HashSize]) {
                    this->generation = gen;
                    this->type       = t;
                    std::memcpy(this->label_digest, ld, sizeof(this->label_digest));
                }

                int GetKeyGeneration() const { return this->generation; }
                EsCommonKeyType GetCommonKeyType() const { return this->type; }
                void GetLabelDigest(u8 dst[crypto::Sha256Generator::HashSize]) const { std::memcpy(dst, this->label_digest, sizeof(this->label_digest)); }
        };

        class ModularExponentiateByStorageKeyAsyncArguments {
            private:
                u8 msg[se::RsaSize];
            public:
                void Set(const void *m, size_t m_size) {
                    AMS_UNUSED(m_size);
                    std::memcpy(this->msg, m, sizeof(this->msg));
                }

                const u8 *GetMessage() const { return this->msg; }
        };

        constinit SmcResult    g_exp_mod_result    = SmcResult::Success;

        constinit bool         g_test_exp_mod_public = false;
        constinit int          g_test_exp_mod_slot   = pkg1::RsaKeySlot_Temporary;
        constinit ImportRsaKey g_test_exp_mod_key    = {};

        constinit union {
            ModularExponentiateByStorageKeyAsyncArguments modular_exponentiate_by_storage_key;
            PrepareEsDeviceUniqueKeyAsyncArguments prepare_es_device_unique_key;
        } g_async_arguments;

        ALWAYS_INLINE ModularExponentiateByStorageKeyAsyncArguments &GetModularExponentiateByStorageKeyAsyncArguments() {
            return g_async_arguments.modular_exponentiate_by_storage_key;
        }

        ALWAYS_INLINE PrepareEsDeviceUniqueKeyAsyncArguments &GetPrepareEsDeviceUniqueKeyAsyncArguments() {
            return g_async_arguments.prepare_es_device_unique_key;
        }

        void SecurityEngineDoneHandler() {
            /* End the asynchronous operation. */
            g_exp_mod_result = SmcResult::Success;
            EndAsyncOperation();
        }

        void TestRsaPublicKey(ImportRsaKey which, int slot, const void *mod, size_t mod_size, se::DoneHandler handler) {
            /* Declare a buffer for our test message. */
            u8 msg[se::RsaSize];
            std::memset(msg, 'D', sizeof(msg));

            /* Provisionally import the modulus. */
            ImportRsaKeyModulusProvisionally(which, mod, mod_size);

            /* Load the provisional public key into the slot. */
            LoadProvisionalRsaPublicKey(slot, which);

            /* Perform the test exponentiation. */
            se::ModularExponentiateAsync(slot, msg, sizeof(msg), handler);
        }

        void TestRsaPrivateKey(ImportRsaKey which, int slot, se::DoneHandler handler) {
            /* Get the result of the public key test. */
            u8 msg[se::RsaSize];
            se::GetRsaResult(msg, sizeof(msg));

            /* Load the provisional private key into the slot. */
            LoadProvisionalRsaKey(slot, which);

            /* Perform the test exponentiation. */
            se::ModularExponentiateAsync(slot, msg, sizeof(msg), handler);
        }

        void VerifyTestRsaKeyResult(ImportRsaKey which) {
            /* Get the result of the test. */
            u8 msg[se::RsaSize];
            se::GetRsaResult(msg, sizeof(msg));

            /* Validate the result. */
            const bool is_valid = (msg[0] == 'D') & (crypto::IsSameBytes(msg, msg + 1, sizeof(msg) - 1));

            /* If the test passes, the key is no longer provisional. */
            if (is_valid) {
                CommitRsaKeyModulus(which);
            }
        }

        void TestRsaKeyDoneHandler() {
            if (g_test_exp_mod_public) {
                /* If we're testing the public key, we still have another exponentiation to do to test the private key. */
                g_test_exp_mod_public = false;

                /* Test the private key. */
                TestRsaPrivateKey(g_test_exp_mod_key, g_test_exp_mod_slot, TestRsaKeyDoneHandler);
            } else {
                /* We're testing the private key, so validate the result. */
                VerifyTestRsaKeyResult(g_test_exp_mod_key);

                /* If the test passed, we can proceed to perform the intended exponentiation. */
                if (LoadRsaKey(g_test_exp_mod_slot, g_test_exp_mod_key)) {
                    se::ModularExponentiateAsync(pkg1::RsaKeySlot_Temporary, GetModularExponentiateByStorageKeyAsyncArguments().GetMessage(), se::RsaSize, SecurityEngineDoneHandler);
                } else {
                    /* The test failed, so end the asynchronous operation. */
                    g_exp_mod_result = SmcResult::InvalidArgument;
                    EndAsyncOperation();
                }
            }
        }

        SmcResult ModularExponentiateImpl(SmcArguments &args) {
            /* Decode arguments. */
            const uintptr_t msg_address = args.r[1];
            const uintptr_t exp_address = args.r[2];
            const uintptr_t mod_address = args.r[3];
            const size_t    exp_size    = args.r[4];

            /* Validate arguments. */
            SMC_R_UNLESS(util::IsAligned(exp_size, sizeof(u32)), InvalidArgument);
            SMC_R_UNLESS(exp_size <= se::RsaSize, InvalidArgument);

            /* Copy the message and modulus from the user. */
            alignas(8) u8 msg[se::RsaSize];
            alignas(8) u8 exp[se::RsaSize];
            alignas(8) u8 mod[se::RsaSize];
            {
                UserPageMapper mapper(msg_address);
                SMC_R_UNLESS(mapper.Map(),                                       InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(msg, msg_address, sizeof(msg)), InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(exp, exp_address, exp_size),    InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(mod, mod_address, sizeof(mod)), InvalidArgument);
            }

            /* We're performing an operation, so set the result to busy. */
            g_exp_mod_result = SmcResult::Busy;

            /* Load the key into the temporary keyslot. */
            se::SetRsaKey(pkg1::RsaKeySlot_Temporary, mod, sizeof(mod), exp, exp_size);

            /* Begin the asynchronous exponentiation. */
            se::ModularExponentiateAsync(pkg1::RsaKeySlot_Temporary, msg, sizeof(msg), SecurityEngineDoneHandler);

            return SmcResult::Success;
        }

        SmcResult ModularExponentiateByStorageKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            const uintptr_t msg_address = args.r[1];
            const uintptr_t mod_address = args.r[2];
            const util::BitPack32 option = { static_cast<u32>(args.r[3]) };

            const auto mode       = GetTargetFirmware() >= TargetFirmware_5_0_0 ? option.Get<ModularExponentiateByStorageKeyOption::Mode>() : GetModeForImportRsaKey(ImportRsaKey_Lotus);
            const auto reserved   = option.Get<PrepareEsDeviceUniqueKeyOption::Reserved>();

            /* Validate arguments. */
            SMC_R_UNLESS(reserved == 0,                                   InvalidArgument);
            SMC_R_UNLESS(mode < ModularExponentiateByStorageKeyTableSize, InvalidArgument);

            /* Convert the mode to an import key. */
            const auto import_key = static_cast<ImportRsaKey>(ModularExponentiateByStorageKeyTable[mode]);

            /* Copy the message and modulus from the user. */
            alignas(8) u8 msg[se::RsaSize];
            alignas(8) u8 mod[se::RsaSize];
            {
                UserPageMapper mapper(msg_address);
                SMC_R_UNLESS(mapper.Map(),                                       InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(msg, msg_address, sizeof(msg)), InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(mod, mod_address, sizeof(mod)), InvalidArgument);
            }

            /* We're performing an operation, so set the result to busy. */
            g_exp_mod_result = SmcResult::Busy;

            /* In the ideal case, the key pair is already verified. If it is, we can use it directly. */
            if (LoadRsaKey(pkg1::RsaKeySlot_Temporary, import_key)) {
                se::ModularExponentiateAsync(pkg1::RsaKeySlot_Temporary, msg, sizeof(msg), SecurityEngineDoneHandler);
            } else {
                /* Set the async arguments. */
                GetModularExponentiateByStorageKeyAsyncArguments().Set(msg, sizeof(msg));

                /* Test the rsa key. */
                g_test_exp_mod_slot   = pkg1::RsaKeySlot_Temporary;
                g_test_exp_mod_key    = import_key;
                g_test_exp_mod_public = true;

                TestRsaPublicKey(import_key, pkg1::RsaKeySlot_Temporary, mod, sizeof(mod), TestRsaKeyDoneHandler);
            }

            return SmcResult::Success;
        }

        SmcResult PrepareEsDeviceUniqueKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 label_digest[crypto::Sha256Generator::HashSize];

            const uintptr_t msg_address = args.r[1];
            const uintptr_t mod_address = args.r[2];
            std::memcpy(label_digest, std::addressof(args.r[3]), sizeof(label_digest));
            const util::BitPack32 option = { static_cast<u32>(args.r[7]) };

            const auto generation = GetTargetFirmware() >= TargetFirmware_3_0_0 ? std::max<int>(pkg1::KeyGeneration_1_0_0, option.Get<PrepareEsDeviceUniqueKeyOption::KeyGeneration>() - 1) : pkg1::KeyGeneration_1_0_0;
            const auto type       = option.Get<PrepareEsDeviceUniqueKeyOption::Type>();
            const auto reserved   = option.Get<PrepareEsDeviceUniqueKeyOption::Reserved>();

            /* Validate arguments. */
            SMC_R_UNLESS(reserved == 0,                          InvalidArgument);
            SMC_R_UNLESS(pkg1::IsValidKeyGeneration(generation), InvalidArgument);
            SMC_R_UNLESS(generation <= GetKeyGeneration(),       InvalidArgument);
            SMC_R_UNLESS(type < EsCommonKeyType_Count,           InvalidArgument);

            /* Copy the message and modulus from the user. */
            alignas(8) u8 msg[se::RsaSize];
            alignas(8) u8 mod[se::RsaSize];
            {
                UserPageMapper mapper(msg_address);
                SMC_R_UNLESS(mapper.Map(),                                       InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(msg, msg_address, sizeof(msg)), InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(mod, mod_address, sizeof(mod)), InvalidArgument);
            }

            /* We're performing an operation, so set the result to busy. */
            g_exp_mod_result = SmcResult::Busy;

            /* Set the async arguments. */
            GetPrepareEsDeviceUniqueKeyAsyncArguments().Set(generation, type, label_digest);

            /* Load the es drm key into the security engine. */
            SMC_R_UNLESS(LoadRsaKey(pkg1::RsaKeySlot_Temporary, ImportRsaKey_EsDrmCert), NotInitialized);

            /* Trigger the asynchronous modular exponentiation. */
            se::ModularExponentiateAsync(pkg1::RsaKeySlot_Temporary, msg, sizeof(msg), SecurityEngineDoneHandler);

            return SmcResult::Success;
        }

        SmcResult GetModularExponentiateResult(void *dst, size_t dst_size) {
            /* Validate state. */
            SMC_R_TRY(g_exp_mod_result);
            SMC_R_UNLESS(dst_size == se::RsaSize, InvalidArgument);

            /* We want to relinquish our security engine lock at the end of scope. */
            ON_SCOPE_EXIT { UnlockSecurityEngine(); };

            /* Get the result of the exponentiation. */
            se::GetRsaResult(dst, se::RsaSize);

            return SmcResult::Success;
        }

        SmcResult GetPrepareEsDeviceUniqueKeyResult(void *dst, size_t dst_size) {
            /* Declare variables. */
            u8 key_source[se::AesBlockSize];
            u8 key[se::AesBlockSize];
            u8 access_key[se::AesBlockSize];

            /* Validate state. */
            SMC_R_TRY(g_exp_mod_result);
            SMC_R_UNLESS(dst_size == sizeof(access_key), InvalidArgument);

            /* We want to relinquish our security engine lock at the end of scope. */
            ON_SCOPE_EXIT { UnlockSecurityEngine(); };

            /* Get the async args. */
            const auto &async_args = GetPrepareEsDeviceUniqueKeyAsyncArguments();

            /* Get the exponentiation output. */
            alignas(8) u8 msg[se::RsaSize];
            se::GetRsaResult(msg, sizeof(msg));

            /* Decode the key. */
            {
                /* Get the label digest. */
                u8 label_digest[crypto::Sha256Generator::HashSize];
                async_args.GetLabelDigest(label_digest);

                /* Decode the key source. */
                const size_t key_source_size = se::DecodeRsaOaepSha256(key_source, sizeof(key_source), msg, sizeof(msg), label_digest, sizeof(label_digest));
                SMC_R_UNLESS(key_source_size == sizeof(key_source), InvalidArgument);
            }

            /* Decrypt the key. */
            DecryptWithEsCommonKey(key, sizeof(key), key_source, sizeof(key_source), async_args.GetCommonKeyType(), async_args.GetKeyGeneration());
            PrepareEsAesKey(access_key, sizeof(access_key), key, sizeof(key));

            /* Copy the access key to output. */
            std::memcpy(dst, access_key, sizeof(access_key));

            return SmcResult::Success;
        }

    }

    SmcResult SmcModularExponentiate(SmcArguments &args) {
        return LockSecurityEngineAndInvokeAsync(args, ModularExponentiateImpl, GetModularExponentiateResult);
    }

    SmcResult SmcModularExponentiateByStorageKey(SmcArguments &args) {
        return LockSecurityEngineAndInvokeAsync(args, ModularExponentiateByStorageKeyImpl, GetModularExponentiateResult);
    }

    SmcResult SmcPrepareEsDeviceUniqueKey(SmcArguments &args) {
        return LockSecurityEngineAndInvokeAsync(args, PrepareEsDeviceUniqueKeyImpl, GetPrepareEsDeviceUniqueKeyResult);
    }

}
