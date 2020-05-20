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
#include "secmon_smc_aes.hpp"
#include "secmon_smc_rsa.hpp"
#include "secmon_smc_se_lock.hpp"
#include "secmon_page_mapper.hpp"

namespace ams::secmon::smc {

    namespace {

        struct PrepareEsDeviceUniqueKeyOption {
            using KeyGeneration = util::BitPack32::Field<0,  6, int>;
            using Type          = util::BitPack32::Field<6,  1, EsCommonKeyType>;
            using Reserved      = util::BitPack32::Field<7, 25, u32>;
        };

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
                    std::memcpy(this->msg, m, sizeof(this->msg));
                }

                void GetMessage(void *dst, size_t dst_size) const { std::memcpy(dst, this->msg, sizeof(this->msg)); }
        };

        constinit bool g_exp_mod_completed = false;

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
            g_exp_mod_completed = true;
            EndAsyncOperation();
        }

        SmcResult PrepareEsDeviceUniqueKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 label_digest[crypto::Sha256Generator::HashSize];

            const uintptr_t msg_address = args.r[1];
            const uintptr_t mod_address = args.r[2];
            std::memcpy(label_digest, std::addressof(args.r[3]), sizeof(label_digest));
            const util::BitPack32 option = { static_cast<u32>(args.r[7]) };

            const auto generation = GetTargetFirmware() >= TargetFirmware_3_0_0 ? std::max(0, option.Get<PrepareEsDeviceUniqueKeyOption::KeyGeneration>() - 1) : 0;
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

            /* We're performing an operation, so the operation is not completed. */
            g_exp_mod_completed = false;

            /* Set the async arguments. */
            GetPrepareEsDeviceUniqueKeyAsyncArguments().Set(generation, type, label_digest);

            /* Load the es drm key into the security engine. */
            SMC_R_UNLESS(LoadRsaKey(pkg1::RsaKeySlot_Temporary, ImportRsaKey_EsDrmCert), NotInitialized);

            /* Trigger the asynchronous modular exponentiation. */
            se::ModularExponentiateAsync(pkg1::RsaKeySlot_Temporary, msg, sizeof(msg), SecurityEngineDoneHandler);

            return SmcResult::Success;
        }

        SmcResult GetPrepareEsDeviceUniqueKeyResult(void *dst, size_t dst_size) {
            /* Declare variables. */
            u8 key_source[se::AesBlockSize];
            u8 key[se::AesBlockSize];
            u8 access_key[se::AesBlockSize];

            /* Validate state. */
            SMC_R_UNLESS(g_exp_mod_completed,                       Busy);
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
        /* TODO */
        return SmcResult::NotImplemented;
    }

    SmcResult SmcModularExponentiateByStorageKey(SmcArguments &args) {
        /* TODO */
        return SmcResult::NotImplemented;
    }

    SmcResult SmcPrepareEsDeviceUniqueKey(SmcArguments &args) {
        return LockSecurityEngineAndInvokeAsync(args, PrepareEsDeviceUniqueKeyImpl, GetPrepareEsDeviceUniqueKeyResult);
    }

}
