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

namespace ams::spl::smc {

    Result SetConfig(AsyncOperationKey *out_op, spl::ConfigItem key, const u64 *value, size_t num_qwords, const void *sign) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::SetConfig);
        args.r[1] = static_cast<u64>(key);
        args.r[2] = reinterpret_cast<u64>(sign);

        for (size_t i = 0; i < std::min(static_cast<size_t>(4), num_qwords); i++) {
            args.r[3 + i] = value[i];
        }
        svc::CallSecureMonitor(std::addressof(args));

        out_op->value = args.r[1];
        return static_cast<Result>(args.r[0]);
    }

    Result GetConfig(u64 *out, size_t num_qwords, spl::ConfigItem key) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::GetConfig);
        args.r[1] = static_cast<u64>(key);
        svc::CallSecureMonitor(std::addressof(args));

        for (size_t i = 0; i < std::min(static_cast<size_t>(4), num_qwords); i++) {
            out[i] = args.r[1 + i];
        }
        return static_cast<Result>(args.r[0]);
    }

    Result GetResult(Result *out, AsyncOperationKey op) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::GetResult);
        args.r[1] = op.value;
        svc::CallSecureMonitor(std::addressof(args));

        *out = static_cast<Result>(args.r[1]);
        return static_cast<Result>(args.r[0]);
    }

    Result GetResultData(Result *out, void *out_buf, size_t out_buf_size, AsyncOperationKey op) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::GetResultData);
        args.r[1] = op.value;
        args.r[2] = reinterpret_cast<u64>(out_buf);
        args.r[3] = out_buf_size;
        svc::CallSecureMonitor(std::addressof(args));

        *out = static_cast<Result>(args.r[1]);
        return static_cast<Result>(args.r[0]);
    }

    Result ModularExponentiate(AsyncOperationKey *out_op, const void *base, const void *exp, size_t exp_size, const void *mod) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::ModularExponentiate);
        args.r[1] = reinterpret_cast<u64>(base);
        args.r[2] = reinterpret_cast<u64>(exp);
        args.r[3] = reinterpret_cast<u64>(mod);
        args.r[4] = exp_size;
        svc::CallSecureMonitor(std::addressof(args));

        out_op->value = args.r[1];
        return static_cast<Result>(args.r[0]);
    }

    Result GenerateRandomBytes(void *out, size_t size) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::GenerateRandomBytes);
        args.r[1] = size;
        svc::CallSecureMonitor(std::addressof(args));

        if (args.r[0] == static_cast<u64>(Result::Success) && (size <= sizeof(args) - sizeof(args.r[0]))) {
            std::memcpy(out, std::addressof(args.r[1]), size);
        }
        return static_cast<Result>(args.r[0]);
    }

    Result GenerateAesKek(AccessKey *out, const KeySource &source, u32 generation, u32 option) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::GenerateAesKek);
        args.r[1] = source.data64[0];
        args.r[2] = source.data64[1];
        args.r[3] = generation;
        args.r[4] = option;
        svc::CallSecureMonitor(std::addressof(args));

        out->data64[0] = args.r[1];
        out->data64[1] = args.r[2];
        return static_cast<Result>(args.r[0]);
    }

    Result LoadAesKey(u32 keyslot, const AccessKey &access_key, const KeySource &source) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::LoadAesKey);
        args.r[1] = keyslot;
        args.r[2] = access_key.data64[0];
        args.r[3] = access_key.data64[1];
        args.r[4] = source.data64[0];
        args.r[5] = source.data64[1];
        svc::CallSecureMonitor(std::addressof(args));

        return static_cast<Result>(args.r[0]);
    }

    Result ComputeAes(AsyncOperationKey *out_op, u32 dst_addr, u32 mode, const IvCtr &iv_ctr, u32 src_addr, size_t size) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::ComputeAes);
        args.r[1] = mode;
        args.r[2] = iv_ctr.data64[0];
        args.r[3] = iv_ctr.data64[1];
        args.r[4] = src_addr;
        args.r[5] = dst_addr;
        args.r[6] = size;
        svc::CallSecureMonitor(std::addressof(args));

        out_op->value = args.r[1];
        return static_cast<Result>(args.r[0]);
    }

    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &source, u32 generation, u32 which) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::GenerateSpecificAesKey);
        args.r[1] = source.data64[0];
        args.r[2] = source.data64[1];
        args.r[3] = generation;
        args.r[4] = which;
        svc::CallSecureMonitor(std::addressof(args));

        out_key->data64[0] = args.r[1];
        out_key->data64[1] = args.r[2];
        return static_cast<Result>(args.r[0]);
    }

    Result ComputeCmac(Cmac *out_mac, u32 keyslot, const void *data, size_t size) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::ComputeCmac);
        args.r[1] = keyslot;
        args.r[2] = reinterpret_cast<u64>(data);
        args.r[3] = size;
        svc::CallSecureMonitor(std::addressof(args));

        out_mac->data64[0] = args.r[1];
        out_mac->data64[1] = args.r[2];
        return static_cast<Result>(args.r[0]);
    }

    Result ReencryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::ReencryptDeviceUniqueData);
        args.r[1] = reinterpret_cast<u64>(std::addressof(access_key_dec));
        args.r[2] = reinterpret_cast<u64>(std::addressof(access_key_enc));
        args.r[3] = option;
        args.r[4] = reinterpret_cast<u64>(data);
        args.r[5] = size;
        args.r[6] = reinterpret_cast<u64>(std::addressof(source_dec));
        args.r[7] = reinterpret_cast<u64>(std::addressof(source_enc));
        svc::CallSecureMonitor(std::addressof(args));

        return static_cast<Result>(args.r[0]);
    }

    Result DecryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key, const KeySource &source, DeviceUniqueDataMode mode) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::DecryptDeviceUniqueData);
        args.r[1] = access_key.data64[0];
        args.r[2] = access_key.data64[1];
        args.r[3] = static_cast<u32>(mode);
        args.r[4] = reinterpret_cast<u64>(data);
        args.r[5] = size;
        args.r[6] = source.data64[0];
        args.r[7] = source.data64[1];
        svc::CallSecureMonitor(std::addressof(args));

        return static_cast<Result>(args.r[0]);
    }

    Result ModularExponentiateWithStorageKey(AsyncOperationKey *out_op, const void *base, const void *mod, ModularExponentiateWithStorageKeyMode mode) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::ModularExponentiateWithStorageKey);
        args.r[1] = reinterpret_cast<u64>(base);
        args.r[2] = reinterpret_cast<u64>(mod);
        args.r[3] = static_cast<u32>(mode);
        svc::CallSecureMonitor(std::addressof(args));

        out_op->value = args.r[1];
        return static_cast<Result>(args.r[0]);
    }

    Result PrepareEsDeviceUniqueKey(AsyncOperationKey *out_op, const void *base, const void *mod, const void *label_digest, size_t label_digest_size, u32 option) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::PrepareEsDeviceUniqueKey);
        args.r[1] = reinterpret_cast<u64>(base);
        args.r[2] = reinterpret_cast<u64>(mod);
        std::memset(std::addressof(args.r[3]), 0, 4 * sizeof(args.r[3]));
        std::memcpy(std::addressof(args.r[3]), label_digest, std::min(static_cast<size_t>(4 * sizeof(args.r[3])), label_digest_size));
        args.r[7] = option;
        svc::CallSecureMonitor(std::addressof(args));

        out_op->value = args.r[1];
        return static_cast<Result>(args.r[0]);
    }

    Result LoadPreparedAesKey(u32 keyslot, const AccessKey &access_key) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::LoadPreparedAesKey);
        args.r[1] = keyslot;
        args.r[2] = access_key.data64[0];
        args.r[3] = access_key.data64[1];
        svc::CallSecureMonitor(std::addressof(args));

        return static_cast<Result>(args.r[0]);
    }

    Result PrepareCommonEsTitleKey(AccessKey *out, const KeySource &source, u32 generation) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::PrepareCommonEsTitleKey);
        args.r[1] = source.data64[0];
        args.r[2] = source.data64[1];
        args.r[3] = generation;
        svc::CallSecureMonitor(std::addressof(args));

        out->data64[0] = args.r[1];
        out->data64[1] = args.r[2];
        return static_cast<Result>(args.r[0]);
    }


    /* Deprecated functions. */
    Result LoadEsDeviceKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::LoadEsDeviceKey);
        args.r[1] = access_key.data64[0];
        args.r[2] = access_key.data64[1];
        args.r[3] = option;
        args.r[4] = reinterpret_cast<u64>(data);
        args.r[5] = size;
        args.r[6] = source.data64[0];
        args.r[7] = source.data64[1];
        svc::CallSecureMonitor(std::addressof(args));

        return static_cast<Result>(args.r[0]);
    }

    Result DecryptDeviceUniqueData(size_t *out_size, void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::DecryptDeviceUniqueData);
        args.r[1] = access_key.data64[0];
        args.r[2] = access_key.data64[1];
        args.r[3] = option;
        args.r[4] = reinterpret_cast<u64>(data);
        args.r[5] = size;
        args.r[6] = source.data64[0];
        args.r[7] = source.data64[1];
        svc::CallSecureMonitor(std::addressof(args));

        *out_size = static_cast<size_t>(args.r[1]);
        return static_cast<Result>(args.r[0]);
    }

    Result DecryptAndStoreGcKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
        svc::SecureMonitorArguments args;

        args.r[0] = static_cast<u64>(FunctionId::DecryptAndStoreGcKey);
        args.r[1] = access_key.data64[0];
        args.r[2] = access_key.data64[1];
        args.r[3] = option;
        args.r[4] = reinterpret_cast<u64>(data);
        args.r[5] = size;
        args.r[6] = source.data64[0];
        args.r[7] = source.data64[1];
        svc::CallSecureMonitor(std::addressof(args));

        return static_cast<Result>(args.r[0]);
    }

    /* Atmosphere functions. */
    namespace {

        enum class IramCopyDirection {
            FromIram = 0,
            ToIram = 1,
        };

        inline Result AtmosphereIramCopy(uintptr_t dram_address, uintptr_t iram_address, size_t size, IramCopyDirection direction) {
            svc::SecureMonitorArguments args;
            args.r[0] = static_cast<u64>(FunctionId::AtmosphereIramCopy);
            args.r[1] = dram_address;
            args.r[2] = iram_address;
            args.r[3] = size;
            args.r[4] = static_cast<u64>(direction);
            svc::CallSecureMonitor(std::addressof(args));

            return static_cast<Result>(args.r[0]);
        }

    }

    Result AtmosphereCopyToIram(uintptr_t iram_dst, const void *dram_src, size_t size) {
        return AtmosphereIramCopy(reinterpret_cast<uintptr_t>(dram_src), iram_dst, size, IramCopyDirection::ToIram);
    }

    Result AtmosphereCopyFromIram(void *dram_dst, uintptr_t iram_src, size_t size) {
        return AtmosphereIramCopy(reinterpret_cast<uintptr_t>(dram_dst), iram_src, size, IramCopyDirection::FromIram);
    }

    Result AtmosphereReadWriteRegister(uint64_t address, uint32_t mask, uint32_t value, uint32_t *out_value) {
        svc::SecureMonitorArguments args;
        args.r[0] = static_cast<u64>(FunctionId::AtmosphereReadWriteRegister);
        args.r[1] = address;
        args.r[2] = mask;
        args.r[3] = value;
        svc::CallSecureMonitor(std::addressof(args));

        *out_value = static_cast<uint32_t>(args.r[1]);
        return static_cast<Result>(args.r[0]);
    }

    Result AtmosphereGetEmummcConfig(void *out_config, void *out_paths, u32 storage_id) {
        const u64 paths = reinterpret_cast<u64>(out_paths);
        AMS_ABORT_UNLESS(util::IsAligned(paths, os::MemoryPageSize));

        svc::SecureMonitorArguments args = {};
        args.r[0] = static_cast<u64>(FunctionId::AtmosphereGetEmummcConfig);
        args.r[1] = storage_id;
        args.r[2] = paths;
        svc::CallSecureMonitor(std::addressof(args));

        std::memcpy(out_config, std::addressof(args.r[1]), sizeof(args) - sizeof(args.r[0]));
        return static_cast<Result>(args.r[0]);
    }

}
