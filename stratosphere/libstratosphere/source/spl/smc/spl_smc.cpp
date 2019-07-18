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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/spl/smc/spl_smc.hpp>

namespace sts::spl::smc {

    Result SetConfig(SplConfigItem which, const u64 *value, size_t num_qwords) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::SetConfig);
        args.X[1] = which;
        args.X[2] = 0;
        for (size_t i = 0; i < std::min(size_t(4), num_qwords); i++) {
            args.X[3 + i] = value[i];
        }
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result GetConfig(u64 *out, size_t num_qwords, SplConfigItem which) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GetConfig);
        args.X[1] = which;
        svcCallSecureMonitor(&args);

        for (size_t i = 0; i < std::min(size_t(4), num_qwords); i++) {
            out[i] = args.X[1 + i];
        }
        return static_cast<Result>(args.X[0]);
    }

    Result CheckStatus(Result *out, AsyncOperationKey op) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::CheckStatus);
        args.X[1] = op.value;
        svcCallSecureMonitor(&args);

        *out = static_cast<Result>(args.X[1]);
        return static_cast<Result>(args.X[0]);
    }

    Result GetResult(Result *out, void *out_buf, size_t out_buf_size, AsyncOperationKey op) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GetResult);
        args.X[1] = op.value;
        args.X[2] = reinterpret_cast<u64>(out_buf);
        args.X[3] = out_buf_size;
        svcCallSecureMonitor(&args);

        *out = static_cast<Result>(args.X[1]);
        return static_cast<Result>(args.X[0]);
    }

    Result ExpMod(AsyncOperationKey *out_op, const void *base, const void *exp, size_t exp_size, const void *mod) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ExpMod);
        args.X[1] = reinterpret_cast<u64>(base);
        args.X[2] = reinterpret_cast<u64>(exp);
        args.X[3] = reinterpret_cast<u64>(mod);
        args.X[4] = exp_size;
        svcCallSecureMonitor(&args);

        out_op->value = args.X[1];
        return static_cast<Result>(args.X[0]);
    }

    Result GenerateRandomBytes(void *out, size_t size) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GenerateRandomBytes);
        args.X[1] = size;
        svcCallSecureMonitor(&args);

        if (args.X[0] == static_cast<u64>(Result::Success) && (size <= sizeof(args) - sizeof(args.X[0]))) {
            std::memcpy(out, &args.X[1], size);
        }
        return static_cast<Result>(args.X[0]);
    }

    Result GenerateAesKek(AccessKey *out, const KeySource &source, u32 generation, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GenerateAesKek);
        args.X[1] = source.data64[0];
        args.X[2] = source.data64[1];
        args.X[3] = generation;
        args.X[4] = option;
        svcCallSecureMonitor(&args);

        out->data64[0] = args.X[1];
        out->data64[1] = args.X[2];
        return static_cast<Result>(args.X[0]);
    }

    Result LoadAesKey(u32 keyslot, const AccessKey &access_key, const KeySource &source) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::LoadAesKey);
        args.X[1] = keyslot;
        args.X[2] = access_key.data64[0];
        args.X[3] = access_key.data64[1];
        args.X[4] = source.data64[0];
        args.X[5] = source.data64[1];
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result CryptAes(AsyncOperationKey *out_op, u32 mode, const IvCtr &iv_ctr, u32 dst_addr, u32 src_addr, size_t size) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::CryptAes);
        args.X[1] = mode;
        args.X[2] = iv_ctr.data64[0];
        args.X[3] = iv_ctr.data64[1];
        args.X[4] = src_addr;
        args.X[5] = dst_addr;
        args.X[6] = size;
        svcCallSecureMonitor(&args);

        out_op->value = args.X[1];
        return static_cast<Result>(args.X[0]);
    }

    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &source, u32 generation, u32 which) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GenerateSpecificAesKey);
        args.X[1] = source.data64[0];
        args.X[2] = source.data64[1];
        args.X[3] = generation;
        args.X[4] = which;
        svcCallSecureMonitor(&args);

        out_key->data64[0] = args.X[1];
        out_key->data64[1] = args.X[2];
        return static_cast<Result>(args.X[0]);
    }

    Result ComputeCmac(Cmac *out_mac, u32 keyslot, const void *data, size_t size) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ComputeCmac);
        args.X[1] = keyslot;
        args.X[2] = reinterpret_cast<u64>(data);
        args.X[3] = size;
        svcCallSecureMonitor(&args);

        out_mac->data64[0] = args.X[1];
        out_mac->data64[1] = args.X[2];
        return static_cast<Result>(args.X[0]);
    }

    Result ReEncryptRsaPrivateKey(void *data, size_t size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ReEncryptRsaPrivateKey);
        args.X[1] = reinterpret_cast<u64>(&access_key_dec);
        args.X[2] = reinterpret_cast<u64>(&access_key_enc);
        args.X[3] = option;
        args.X[4] = reinterpret_cast<u64>(data);
        args.X[5] = size;
        args.X[6] = reinterpret_cast<u64>(&source_dec);
        args.X[7] = reinterpret_cast<u64>(&source_enc);
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result DecryptOrImportRsaPrivateKey(void *data, size_t size, const AccessKey &access_key, const KeySource &source, DecryptOrImportMode mode) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::DecryptOrImportRsaPrivateKey);
        args.X[1] = access_key.data64[0];
        args.X[2] = access_key.data64[1];
        args.X[3] = static_cast<u32>(mode);
        args.X[4] = reinterpret_cast<u64>(data);
        args.X[5] = size;
        args.X[6] = source.data64[0];
        args.X[7] = source.data64[1];
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result SecureExpMod(AsyncOperationKey *out_op, const void *base, const void *mod, SecureExpModMode mode) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::SecureExpMod);
        args.X[1] = reinterpret_cast<u64>(base);
        args.X[2] = reinterpret_cast<u64>(mod);
        args.X[3] = static_cast<u32>(mode);
        svcCallSecureMonitor(&args);

        out_op->value = args.X[1];
        return static_cast<Result>(args.X[0]);
    }

    Result UnwrapTitleKey(AsyncOperationKey *out_op, const void *base, const void *mod, const void *label_digest, size_t label_digest_size, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::UnwrapTitleKey);
        args.X[1] = reinterpret_cast<u64>(base);
        args.X[2] = reinterpret_cast<u64>(mod);
        std::memset(&args.X[3], 0, 4 * sizeof(args.X[3]));
        std::memcpy(&args.X[3], label_digest, std::min(size_t(4 * sizeof(args.X[3])), label_digest_size));
        args.X[7] = option;
        svcCallSecureMonitor(&args);

        out_op->value = args.X[1];
        return static_cast<Result>(args.X[0]);
    }

    Result LoadTitleKey(u32 keyslot, const AccessKey &access_key) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::LoadTitleKey);
        args.X[1] = keyslot;
        args.X[2] = access_key.data64[0];
        args.X[3] = access_key.data64[1];
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result UnwrapCommonTitleKey(AccessKey *out, const KeySource &source, u32 generation) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::UnwrapCommonTitleKey);
        args.X[1] = source.data64[0];
        args.X[2] = source.data64[1];
        args.X[3] = generation;
        svcCallSecureMonitor(&args);

        out->data64[0] = args.X[1];
        out->data64[1] = args.X[2];
        return static_cast<Result>(args.X[0]);
    }


    /* Deprecated functions. */
    Result ImportEsKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ImportEsKey);
        args.X[1] = access_key.data64[0];
        args.X[2] = access_key.data64[1];
        args.X[3] = option;
        args.X[4] = reinterpret_cast<u64>(data);
        args.X[5] = size;
        args.X[6] = source.data64[0];
        args.X[7] = source.data64[1];
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result DecryptRsaPrivateKey(size_t *out_size, void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::DecryptRsaPrivateKey);
        args.X[1] = access_key.data64[0];
        args.X[2] = access_key.data64[1];
        args.X[3] = option;
        args.X[4] = reinterpret_cast<u64>(data);
        args.X[5] = size;
        args.X[6] = source.data64[0];
        args.X[7] = source.data64[1];
        svcCallSecureMonitor(&args);

        *out_size = static_cast<size_t>(args.X[1]);
        return static_cast<Result>(args.X[0]);
    }

    Result ImportSecureExpModKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ImportSecureExpModKey);
        args.X[1] = access_key.data64[0];
        args.X[2] = access_key.data64[1];
        args.X[3] = option;
        args.X[4] = reinterpret_cast<u64>(data);
        args.X[5] = size;
        args.X[6] = source.data64[0];
        args.X[7] = source.data64[1];
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

}
