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
    inline u32 GetCryptAesMode(CipherMode mode, u32 keyslot) {
        return static_cast<u32>((static_cast<u32>(mode) << 4) | (keyslot & 7));
    }

    inline u32 GetUnwrapEsKeyOption(EsKeyType type, u32 generation) {
        return static_cast<u32>((static_cast<u32>(type) << 6) | (generation & 0x3F));
    }

    /* Functions. */
    Result SetConfig(SplConfigItem which, const u64 *value, size_t num_qwords);
    Result GetConfig(u64 *out, size_t num_qwords, SplConfigItem which);
    Result CheckStatus(Result *out, AsyncOperationKey op);
    Result GetResult(Result *out, void *out_buf, size_t out_buf_size, AsyncOperationKey op);
    Result ExpMod(AsyncOperationKey *out_op, const void *base, const void *exp, size_t exp_size, const void *mod);
    Result GenerateRandomBytes(void *out, size_t size);
    Result GenerateAesKek(AccessKey *out, const KeySource &source, u32 generation, u32 option);
    Result LoadAesKey(u32 keyslot, const AccessKey &access_key, const KeySource &source);
    Result CryptAes(AsyncOperationKey *out_op, u32 mode, const IvCtr &iv_ctr, u32 dst_addr, u32 src_addr, size_t size);
    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &source, u32 generation, u32 which);
    Result ComputeCmac(Cmac *out_mac, u32 keyslot, const void *data, size_t size);
    Result ReEncryptRsaPrivateKey(void *data, size_t size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option);
    Result DecryptOrImportRsaPrivateKey(void *data, size_t size, const AccessKey &access_key, const KeySource &source, DecryptOrImportMode mode);
    Result SecureExpMod(AsyncOperationKey *out_op, const void *base, const void *mod, SecureExpModMode mode);
    Result UnwrapTitleKey(AsyncOperationKey *out_op, const void *base, const void *mod, const void *label_digest, size_t label_digest_size, u32 option);
    Result LoadTitleKey(u32 keyslot, const AccessKey &access_key);
    Result UnwrapCommonTitleKey(AccessKey *out, const KeySource &source, u32 generation);

    /* Deprecated functions. */
    Result ImportEsKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option);
    Result DecryptRsaPrivateKey(size_t *out_size, void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option);
    Result ImportSecureExpModKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option);

    /* Atmosphere functions. */
    Result AtmosphereCopyToIram(uintptr_t iram_dst, const void *dram_src, size_t size);
    Result AtmosphereCopyFromIram(void *dram_dst, uintptr_t iram_src, size_t size);
    Result AtmosphereReadWriteRegister(uint64_t address, uint32_t mask, uint32_t value, uint32_t *out_value);
    Result AtmosphereWriteAddress(void *dst, const void *src, size_t size);
    Result AtmosphereGetEmummcConfig(void *out_config, void *out_paths, u32 storage_id);

    /* Helpers. */
    inline Result SetConfig(SplConfigItem which, const u64 value) {
        return SetConfig(which, &value, 1);
    }

    template<typename T>
    inline Result AtmosphereWriteAddress(void *dst, const T value) {
        static_assert(std::is_integral<T>::value && sizeof(T) <= 8 && (sizeof(T) & (sizeof(T) - 1)) == 0, "AtmosphereWriteAddress requires integral type.");
        return AtmosphereWriteAddress(dst, &value, sizeof(T));
    }

}
