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

namespace ams::spl::smc {

    Result SetConfig(spl::ConfigItem which, const void *address, const u64 *value, size_t num_qwords) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::SetConfig);
        args.X[1] = static_cast<u64>(which);
        args.X[2] = reinterpret_cast<u64>(address);
        for (size_t i = 0; i < std::min(size_t(4), num_qwords); i++) {
            args.X[3 + i] = value[i];
        }
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result GetConfig(u64 *out, size_t num_qwords, spl::ConfigItem which) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GetConfig);
        args.X[1] = static_cast<u64>(which);
        svcCallSecureMonitor(&args);

        for (size_t i = 0; i < std::min(size_t(4), num_qwords); i++) {
            out[i] = args.X[1 + i];
        }
        return static_cast<Result>(args.X[0]);
    }

    Result GetResult(Result *out, AsyncOperationKey op) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GetResult);
        args.X[1] = op.value;
        svcCallSecureMonitor(&args);

        *out = static_cast<Result>(args.X[1]);
        return static_cast<Result>(args.X[0]);
    }

    Result GetResultData(Result *out, void *out_buf, size_t out_buf_size, AsyncOperationKey op) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::GetResultData);
        args.X[1] = op.value;
        args.X[2] = reinterpret_cast<u64>(out_buf);
        args.X[3] = out_buf_size;
        svcCallSecureMonitor(&args);

        *out = static_cast<Result>(args.X[1]);
        return static_cast<Result>(args.X[0]);
    }

    Result ModularExponentiate(AsyncOperationKey *out_op, const void *base, const void *exp, size_t exp_size, const void *mod) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ModularExponentiate);
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

    Result ComputeAes(AsyncOperationKey *out_op, u32 mode, const IvCtr &iv_ctr, u32 dst_addr, u32 src_addr, size_t size) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ComputeAes);
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

    Result ReencryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ReencryptDeviceUniqueData);
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

    Result DecryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key, const KeySource &source, DeviceUniqueDataMode mode) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::DecryptDeviceUniqueData);
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

    Result ModularExponentiateWithStorageKey(AsyncOperationKey *out_op, const void *base, const void *mod, ModularExponentiateWithStorageKeyMode mode) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::ModularExponentiateWithStorageKey);
        args.X[1] = reinterpret_cast<u64>(base);
        args.X[2] = reinterpret_cast<u64>(mod);
        args.X[3] = static_cast<u32>(mode);
        svcCallSecureMonitor(&args);

        out_op->value = args.X[1];
        return static_cast<Result>(args.X[0]);
    }

    Result PrepareEsDeviceUniqueKey(AsyncOperationKey *out_op, const void *base, const void *mod, const void *label_digest, size_t label_digest_size, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::PrepareEsDeviceUniqueKey);
        args.X[1] = reinterpret_cast<u64>(base);
        args.X[2] = reinterpret_cast<u64>(mod);
        std::memset(&args.X[3], 0, 4 * sizeof(args.X[3]));
        std::memcpy(&args.X[3], label_digest, std::min(size_t(4 * sizeof(args.X[3])), label_digest_size));
        args.X[7] = option;
        svcCallSecureMonitor(&args);

        out_op->value = args.X[1];
        return static_cast<Result>(args.X[0]);
    }

    Result LoadPreparedAesKey(u32 keyslot, const AccessKey &access_key) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::LoadPreparedAesKey);
        args.X[1] = keyslot;
        args.X[2] = access_key.data64[0];
        args.X[3] = access_key.data64[1];
        svcCallSecureMonitor(&args);

        return static_cast<Result>(args.X[0]);
    }

    Result PrepareCommonEsTitleKey(AccessKey *out, const KeySource &source, u32 generation) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::PrepareCommonEsTitleKey);
        args.X[1] = source.data64[0];
        args.X[2] = source.data64[1];
        args.X[3] = generation;
        svcCallSecureMonitor(&args);

        out->data64[0] = args.X[1];
        out->data64[1] = args.X[2];
        return static_cast<Result>(args.X[0]);
    }


    /* Deprecated functions. */
    Result LoadEsDeviceKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::LoadEsDeviceKey);
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

    Result DecryptDeviceUniqueData(size_t *out_size, void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::DecryptDeviceUniqueData);
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

    Result DecryptAndStoreGcKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        SecmonArgs args;

        args.X[0] = static_cast<u64>(FunctionId::DecryptAndStoreGcKey);
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

    /* Atmosphere functions. */
    namespace {

        enum class IramCopyDirection {
            FromIram = 0,
            ToIram = 1,
        };

        inline Result AtmosphereIramCopy(uintptr_t dram_address, uintptr_t iram_address, size_t size, IramCopyDirection direction) {
            SecmonArgs args;
            args.X[0] = static_cast<u64>(FunctionId::AtmosphereIramCopy);
            args.X[1] = dram_address;
            args.X[2] = iram_address;
            args.X[3] = size;
            args.X[4] = static_cast<u64>(direction);
            svcCallSecureMonitor(&args);

            return static_cast<Result>(args.X[0]);
        }

    }

    Result AtmosphereCopyToIram(uintptr_t iram_dst, const void *dram_src, size_t size) {
        return AtmosphereIramCopy(reinterpret_cast<uintptr_t>(dram_src), iram_dst, size, IramCopyDirection::ToIram);
    }

    Result AtmosphereCopyFromIram(void *dram_dst, uintptr_t iram_src, size_t size) {
        return AtmosphereIramCopy(reinterpret_cast<uintptr_t>(dram_dst), iram_src, size, IramCopyDirection::FromIram);
    }

    Result AtmosphereReadWriteRegister(uint64_t address, uint32_t mask, uint32_t value, uint32_t *out_value) {
        SecmonArgs args;
        args.X[0] = static_cast<u64>(FunctionId::AtmosphereReadWriteRegister);
        args.X[1] = address;
        args.X[2] = mask;
        args.X[3] = value;
        svcCallSecureMonitor(&args);

        *out_value = static_cast<uint32_t>(args.X[1]);
        return static_cast<Result>(args.X[0]);
    }

    Result AtmosphereGetEmummcConfig(void *out_config, void *out_paths, u32 storage_id) {
        const u64 paths = reinterpret_cast<u64>(out_paths);
        AMS_ABORT_UNLESS(util::IsAligned(paths, os::MemoryPageSize));

        SecmonArgs args = {};
        args.X[0] = static_cast<u64>(FunctionId::AtmosphereGetEmummcConfig);
        args.X[1] = storage_id;
        args.X[2] = paths;
        svcCallSecureMonitor(&args);

        std::memcpy(out_config, &args.X[1], sizeof(args) - sizeof(args.X[0]));
        return static_cast<Result>(args.X[0]);
    }

}
