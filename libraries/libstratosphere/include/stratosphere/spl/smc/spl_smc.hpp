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

#pragma once
#include "../spl_types.hpp"

namespace ams::spl::smc {

    /* Helpers for converting arguments. */
    inline u32 GetComputeAesMode(CipherMode mode, u32 keyslot) {
        return static_cast<u32>((static_cast<u32>(mode) << 4) | (keyslot & 7));
    }

    inline u32 GetPrepareEsDeviceUniqueKeyOption(EsCommonKeyType type, u32 generation) {
        return static_cast<u32>((static_cast<u32>(type) << 6) | (generation & 0x3F));
    }

    /* Functions. */
    Result SetConfig(spl::ConfigItem which, const void *address, const u64 *value, size_t num_qwords);
    Result GetConfig(u64 *out, size_t num_qwords, spl::ConfigItem which);
    Result GetResult(Result *out, AsyncOperationKey op);
    Result GetResultData(Result *out, void *out_buf, size_t out_buf_size, AsyncOperationKey op);
    Result ModularExponentiate(AsyncOperationKey *out_op, const void *base, const void *exp, size_t exp_size, const void *mod);
    Result GenerateRandomBytes(void *out, size_t size);
    Result GenerateAesKek(AccessKey *out, const KeySource &source, u32 generation, u32 option);
    Result LoadAesKey(u32 keyslot, const AccessKey &access_key, const KeySource &source);
    Result ComputeAes(AsyncOperationKey *out_op, u32 mode, const IvCtr &iv_ctr, u32 dst_addr, u32 src_addr, size_t size);
    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &source, u32 generation, u32 which);
    Result ComputeCmac(Cmac *out_mac, u32 keyslot, const void *data, size_t size);
    Result ReencryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option);
    Result DecryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key, const KeySource &source, DeviceUniqueDataMode mode);
    Result ModularExponentiateWithStorageKey(AsyncOperationKey *out_op, const void *base, const void *mod, ModularExponentiateWithStorageKeyMode mode);
    Result PrepareEsDeviceUniqueKey(AsyncOperationKey *out_op, const void *base, const void *mod, const void *label_digest, size_t label_digest_size, u32 option);
    Result LoadPreparedAesKey(u32 keyslot, const AccessKey &access_key);
    Result PrepareCommonEsTitleKey(AccessKey *out, const KeySource &source, u32 generation);

    /* Deprecated functions. */
    Result LoadEsDeviceKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option);
    Result DecryptDeviceUniqueData(size_t *out_size, void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option);
    Result DecryptAndStoreGcKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option);

    /* Atmosphere functions. */
    Result AtmosphereCopyToIram(uintptr_t iram_dst, const void *dram_src, size_t size);
    Result AtmosphereCopyFromIram(void *dram_dst, uintptr_t iram_src, size_t size);
    Result AtmosphereReadWriteRegister(uint64_t address, uint32_t mask, uint32_t value, uint32_t *out_value);
    Result AtmosphereGetEmummcConfig(void *out_config, void *out_paths, u32 storage_id);

    /* Helpers. */
    inline Result SetConfig(spl::ConfigItem which, const u64 *value, size_t num_qwords) {
        return SetConfig(which, nullptr, value, num_qwords);
    }

    inline Result SetConfig(spl::ConfigItem which, const u64 value) {
        return SetConfig(which, std::addressof(value), 1);
    }

}
