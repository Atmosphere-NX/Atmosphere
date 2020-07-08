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
    Result GetConfig(u64 *out, spl::ConfigItem which);
    Result ModularExponentiate(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size);
    Result SetConfig(spl::ConfigItem which, u64 value);
    Result GenerateRandomBytes(void *out, size_t size);
    Result IsDevelopment(bool *out);
    Result SetBootReason(BootReasonValue boot_reason);
    Result GetBootReason(BootReasonValue *out);

    /* Crypto. */
    Result GenerateAesKek(AccessKey *out_access_key, const KeySource &key_source, u32 generation, u32 option);
    Result LoadAesKey(s32 keyslot, const void *owner, const AccessKey &access_key, const KeySource &key_source);
    Result GenerateAesKey(AesKey *out_key, const AccessKey &access_key, const KeySource &key_source);
    Result DecryptAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 option);
    Result ComputeCtr(void *dst, size_t dst_size, s32 keyslot, const void *owner, const void *src, size_t src_size, const IvCtr &iv_ctr);
    Result ComputeCmac(Cmac *out_cmac, s32 keyslot, const void *owner, const void *data, size_t size);
    Result AllocateAesKeySlot(s32 *out_keyslot, const void *owner);
    Result DeallocateAesKeySlot(s32 keyslot, const void *owner);

    /* RSA. */
    Result DecryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option);

    /* SSL */
    Result DecryptAndStoreSslClientCertKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source);
    Result ModularExponentiateWithSslClientCertKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size);

    /* ES */
    Result LoadEsDeviceKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option);
    Result PrepareEsTitleKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation);
    Result PrepareCommonEsTitleKey(AccessKey *out_access_key, const KeySource &key_source, u32 generation);
    Result DecryptAndStoreDrmDeviceCertKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source);
    Result ModularExponentiateWithDrmDeviceCertKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size);
    Result PrepareEsArchiveKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation);
    Result LoadPreparedAesKey(s32 keyslot, const void *owner, const AccessKey &access_key);

    /* FS */
    Result DecryptAndStoreGcKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option);
    Result DecryptGcMessage(u32 *out_size, void *dst, size_t dst_size, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size);
    Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 which);
    Result LoadPreparedAesKey(s32 keyslot, const void *owner, const AccessKey &access_key);
    Result GetPackage2Hash(void *dst, const size_t size);

    /* Manu. */
    Result ReencryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option);

    /* Helper. */
    Result DeallocateAllAesKeySlots(const void *owner);
    Handle GetAesKeySlotAvailableEventHandle();

}
