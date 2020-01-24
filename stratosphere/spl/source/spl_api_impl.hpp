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
#include <stratosphere.hpp>

namespace ams::spl::impl {

    /* Initialization. */
    void Initialize();

    /* General. */
    Result GetConfig(u64 *out, SplConfigItem which);
    Result ExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size);
    Result SetConfig(SplConfigItem which, u64 value);
    Result GenerateRandomBytes(void *out, size_t size);
    Result IsDevelopment(bool *out);
    Result SetBootReason(BootReasonValue boot_reason);
    Result GetBootReason(BootReasonValue *out);

    /* Crypto. */
    Result GenerateAesKek(AccessKey *out_access_key, const KeySource &key_source, u32 generation, u32 option);
    Result LoadAesKey(u32 keyslot, const void *owner, const AccessKey &access_key, const KeySource &key_source);
    Result GenerateAesKey(AesKey *out_key, const AccessKey &access_key, const KeySource &key_source);
    Result DecryptAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 option);
    Result CryptAesCtr(void *dst, size_t dst_size, u32 keyslot, const void *owner, const void *src, size_t src_size, const IvCtr &iv_ctr);
    Result ComputeCmac(Cmac *out_cmac, u32 keyslot, const void *owner, const void *data, size_t size);
    Result AllocateAesKeyslot(u32 *out_keyslot, const void *owner);
    Result FreeAesKeyslot(u32 keyslot, const void *owner);

    /* RSA. */
    Result DecryptRsaPrivateKey(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option);

    /* SSL */
    Result ImportSslKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source);
    Result SslExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size);

    /* ES */
    Result ImportEsKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option);
    Result UnwrapTitleKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation);
    Result UnwrapCommonTitleKey(AccessKey *out_access_key, const KeySource &key_source, u32 generation);
    Result ImportDrmKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source);
    Result DrmExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size);
    Result UnwrapElicenseKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation);
    Result LoadElicenseKey(u32 keyslot, const void *owner, const AccessKey &access_key);

    /* FS */
    Result ImportLotusKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option);
    Result DecryptLotusMessage(u32 *out_size, void *dst, size_t dst_size, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size);
    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 which);
    Result LoadTitleKey(u32 keyslot, const void *owner, const AccessKey &access_key);
    Result GetPackage2Hash(void *dst, const size_t size);

    /* Manu. */
    Result ReEncryptRsaPrivateKey(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option);

    /* Helper. */
    Result FreeAesKeyslots(const void *owner);
    Handle GetAesKeyslotAvailableEventHandle();

}
