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

#include "spl_secmon_wrapper.hpp"
#include "spl_smc_wrapper.hpp"
#include "spl_ctr_drbg.hpp"

/* Convenient. */
constexpr size_t DeviceAddressSpaceAlignSize = 0x400000;
constexpr size_t DeviceAddressSpaceAlignMask = DeviceAddressSpaceAlignSize - 1;
constexpr u32 WorkBufferMapBase  = 0x80000000u;
constexpr u32 CryptAesInMapBase  = 0x90000000u;
constexpr u32 CryptAesOutMapBase = 0xC0000000u;
constexpr size_t CryptAesSizeMax = static_cast<size_t>(CryptAesOutMapBase - CryptAesInMapBase);

constexpr size_t RsaPrivateKeySize = 0x100;
constexpr size_t RsaPrivateKeyMetaSize = 0x30;
constexpr size_t LabelDigestSizeMax = 0x20;

/* Types. */
struct SeLinkedListEntry {
    u32 num_entries;
    u32 address;
    u32 size;
};

struct SeCryptContext {
    SeLinkedListEntry in;
    SeLinkedListEntry out;
};

class DeviceAddressSpaceMapHelper {
    private:
        Handle das_hnd;
        u64 dst_addr;
        u64 src_addr;
        size_t size;
        u32 perm;
    public:
        DeviceAddressSpaceMapHelper(Handle h, u64 dst, u64 src, size_t sz, u32 p) : das_hnd(h), dst_addr(dst), src_addr(src), size(sz), perm(p) {
            if (R_FAILED(svcMapDeviceAddressSpaceAligned(this->das_hnd, CUR_PROCESS_HANDLE, this->src_addr, this->size, this->dst_addr, this->perm))) {
                std::abort();
            }
        }
        ~DeviceAddressSpaceMapHelper() {
            if (R_FAILED(svcUnmapDeviceAddressSpace(this->das_hnd, CUR_PROCESS_HANDLE, this->src_addr, this->size, this->dst_addr))) {
                std::abort();
            }
        }
};

/* Globals. */
static CtrDrbg g_drbg;
static Event g_se_event;
static IEvent *g_se_keyslot_available_event = nullptr;

static Handle g_se_das_hnd;
static u32 g_se_mapped_work_buffer_addr;
static __attribute__((aligned(0x1000))) u8 g_work_buffer[0x1000];
constexpr size_t MaxWorkBufferSize = sizeof(g_work_buffer) / 2;

static HosMutex g_async_op_lock;

void SecureMonitorWrapper::InitializeCtrDrbg() {
    u8 seed[CtrDrbg::SeedSize];

    if (SmcWrapper::GenerateRandomBytes(seed, sizeof(seed)) != SmcResult_Success) {
        std::abort();
    }

    g_drbg.Initialize(seed);
}

void SecureMonitorWrapper::InitializeSeEvents() {
    u64 irq_num;
    SmcWrapper::GetConfig(&irq_num, 1, SplConfigItem_SecurityEngineIrqNumber);
    Handle hnd;
    if (R_FAILED(svcCreateInterruptEvent(&hnd, irq_num, 1))) {
        std::abort();
    }
    eventLoadRemote(&g_se_event, hnd, true);

    g_se_keyslot_available_event = CreateWriteOnlySystemEvent();
    g_se_keyslot_available_event->Signal();
}

void SecureMonitorWrapper::InitializeDeviceAddressSpace() {
    constexpr u64 DeviceName_SE = 29;

    /* Create Address Space. */
    if (R_FAILED(svcCreateDeviceAddressSpace(&g_se_das_hnd, 0, (1ul << 32)))) {
        std::abort();
    }
    /* Attach it to the SE. */
    if (R_FAILED(svcAttachDeviceAddressSpace(DeviceName_SE, g_se_das_hnd))) {
        std::abort();
    }

    const u64 work_buffer_addr = reinterpret_cast<u64>(g_work_buffer);
    g_se_mapped_work_buffer_addr = WorkBufferMapBase + (work_buffer_addr & DeviceAddressSpaceAlignMask);

    /* Map the work buffer for the SE. */
    if (R_FAILED(svcMapDeviceAddressSpaceAligned(g_se_das_hnd, CUR_PROCESS_HANDLE, work_buffer_addr, sizeof(g_work_buffer), g_se_mapped_work_buffer_addr, 3))) {
        std::abort();
    }
}

void SecureMonitorWrapper::Initialize() {
    /* Initialize the Drbg. */
    InitializeCtrDrbg();
    /* Initialize SE interrupt + keyslot events. */
    InitializeSeEvents();
    /* Initialize DAS for the SE. */
    InitializeDeviceAddressSpace();
}

void SecureMonitorWrapper::CalcMgf1AndXor(void *dst, size_t dst_size, const void *src, size_t src_size) {
    uint8_t *dst_u8 = reinterpret_cast<u8 *>(dst);

    u32 ctr = 0;
    while (dst_size > 0) {
        size_t cur_size = SHA256_HASH_SIZE;
        if (cur_size > dst_size) {
            cur_size = dst_size;
        }
        dst_size -= cur_size;

        u32 ctr_be = __builtin_bswap32(ctr++);
        u8 hash[SHA256_HASH_SIZE];
        {
            Sha256Context ctx;
            sha256ContextCreate(&ctx);
            sha256ContextUpdate(&ctx, src, src_size);
            sha256ContextUpdate(&ctx, &ctr_be, sizeof(ctr_be));
            sha256ContextGetHash(&ctx, hash);
        }

        for (size_t i = 0; i < cur_size; i++) {
            *(dst_u8++) ^= hash[i];
        }
    }
}

size_t SecureMonitorWrapper::DecodeRsaOaep(void *dst, size_t dst_size, const void *label_digest, size_t label_digest_size, const void *src, size_t src_size) {
    /* Very basic validation. */
    if (dst_size == 0 || src_size != 0x100 || label_digest_size != SHA256_HASH_SIZE) {
        return 0;
    }

    u8 block[0x100];
    std::memcpy(block, src, sizeof(block));

    /* First, validate byte 0 == 0, and unmask DB. */
    int invalid = block[0];
    u8 *salt = block + 1;
    u8 *db = salt + SHA256_HASH_SIZE;
    CalcMgf1AndXor(salt, SHA256_HASH_SIZE, db, src_size - (1 + SHA256_HASH_SIZE));
    CalcMgf1AndXor(db, src_size - (1 + SHA256_HASH_SIZE), salt, SHA256_HASH_SIZE);

    /* Validate label digest. */
    for (size_t i = 0; i < SHA256_HASH_SIZE; i++) {
        invalid |= db[i] ^ reinterpret_cast<const u8 *>(label_digest)[i];
    }

    /* Locate message after 00...0001 padding. */
    const u8 *padded_msg = db + SHA256_HASH_SIZE;
    size_t padded_msg_size = src_size - (1 + 2 * SHA256_HASH_SIZE);
    size_t msg_ind = 0;
    int not_found = 1;
    int wrong_padding = 0;
    size_t i = 0;
    while (i < padded_msg_size) {
        int zero = (padded_msg[i] == 0);
        int one = (padded_msg[i] == 1);
        msg_ind += static_cast<size_t>(not_found & one) * (++i);
        not_found &= ~one;
        wrong_padding |= (not_found & ~zero);
    }

    if (invalid | not_found | wrong_padding) {
        return 0;
    }

    /* Copy message out. */
    size_t msg_size = padded_msg_size - msg_ind;
    if (msg_size > dst_size) {
        return 0;
    }
    std::memcpy(dst, padded_msg + msg_ind, msg_size);
    return msg_size;
}

void SecureMonitorWrapper::WaitSeOperationComplete() {
    eventWait(&g_se_event, U64_MAX);
}

Result SecureMonitorWrapper::ConvertToSplResult(SmcResult result) {
    if (result == SmcResult_Success) {
        return ResultSuccess;
    }
    if (result < SmcResult_Max) {
        return MAKERESULT(Module_Spl, static_cast<u32>(result));
    }
    return ResultSplUnknownSmcResult;
}

SmcResult SecureMonitorWrapper::WaitCheckStatus(AsyncOperationKey op_key) {
    WaitSeOperationComplete();

    SmcResult op_res;
    SmcResult res = SmcWrapper::CheckStatus(&op_res, op_key);
    if (res != SmcResult_Success) {
        return res;
    }

    return op_res;
}

SmcResult SecureMonitorWrapper::WaitGetResult(void *out_buf, size_t out_buf_size, AsyncOperationKey op_key) {
    WaitSeOperationComplete();

    SmcResult op_res;
    SmcResult res = SmcWrapper::GetResult(&op_res, out_buf, out_buf_size, op_key);
    if (res != SmcResult_Success) {
        return res;
    }

    return op_res;
}

SmcResult SecureMonitorWrapper::DecryptAesBlock(u32 keyslot, void *dst, const void *src) {
    struct DecryptAesBlockLayout {
        SeCryptContext crypt_ctx;
        u8 in_block[AES_BLOCK_SIZE] __attribute__((aligned(AES_BLOCK_SIZE)));
        u8 out_block[AES_BLOCK_SIZE] __attribute__((aligned(AES_BLOCK_SIZE)));
    };
    DecryptAesBlockLayout *layout = reinterpret_cast<DecryptAesBlockLayout *>(g_work_buffer);

    layout->crypt_ctx.in.num_entries = 0;
    layout->crypt_ctx.in.address = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, in_block);
    layout->crypt_ctx.in.size = sizeof(layout->in_block);
    layout->crypt_ctx.out.num_entries = 0;
    layout->crypt_ctx.out.address = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, out_block);
    layout->crypt_ctx.out.size = sizeof(layout->out_block);

    std::memcpy(layout->in_block, src, sizeof(layout->in_block));

    armDCacheFlush(layout, sizeof(*layout));
    {
        std::scoped_lock<HosMutex> lk(g_async_op_lock);
        AsyncOperationKey op_key;
        const IvCtr iv_ctr = {};
        const u32 mode = SmcWrapper::GetCryptAesMode(SmcCipherMode_CbcDecrypt, keyslot);
        const u32 dst_ll_addr = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, crypt_ctx.out);
        const u32 src_ll_addr = g_se_mapped_work_buffer_addr + offsetof(DecryptAesBlockLayout, crypt_ctx.in);

        SmcResult res = SmcWrapper::CryptAes(&op_key, mode, iv_ctr, dst_ll_addr, src_ll_addr, sizeof(layout->in_block));
        if (res != SmcResult_Success) {
            return res;
        }

        if ((res = WaitCheckStatus(op_key)) != SmcResult_Success) {
            return res;
        }
    }
    armDCacheFlush(layout, sizeof(*layout));

    std::memcpy(dst, layout->out_block, sizeof(layout->out_block));
    return SmcResult_Success;
}

Result SecureMonitorWrapper::GetConfig(u64 *out, SplConfigItem which) {
    /* Nintendo explicitly blacklists package2 hash here, amusingly. */
    /* This is not blacklisted in safemode, but we're never in safe mode... */
    if (which == SplConfigItem_Package2Hash) {
        return ResultSplInvalidArgument;
    }

    SmcResult res = SmcWrapper::GetConfig(out, 1, which);

    /* Nintendo has some special handling here for hardware type/is_retail. */
    if (which == SplConfigItem_HardwareType && res == SmcResult_InvalidArgument) {
        *out = 0;
        res = SmcResult_Success;
    }
    if (which == SplConfigItem_IsRetail && res == SmcResult_InvalidArgument) {
        *out = 0;
        res = SmcResult_Success;
    }

    return ConvertToSplResult(res);
}

Result SecureMonitorWrapper::ExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size) {
    struct ExpModLayout {
        u8 base[0x100];
        u8 exp[0x100];
        u8 mod[0x100];
    };
    ExpModLayout *layout = reinterpret_cast<ExpModLayout *>(g_work_buffer);

    /* Validate sizes. */
    if (base_size > sizeof(layout->base)) {
        return ResultSplInvalidSize;
    }
    if (exp_size > sizeof(layout->exp)) {
        return ResultSplInvalidSize;
    }
    if (mod_size > sizeof(layout->mod)) {
        return ResultSplInvalidSize;
    }
    if (out_size > MaxWorkBufferSize) {
        return ResultSplInvalidSize;
    }

    /* Copy data into work buffer. */
    const size_t base_ofs = sizeof(layout->base) - base_size;
    const size_t mod_ofs = sizeof(layout->mod) - mod_size;
    std::memset(layout, 0, sizeof(*layout));
    std::memcpy(layout->base + base_ofs, base, base_size);
    std::memcpy(layout->exp, exp, exp_size);
    std::memcpy(layout->mod + mod_ofs, mod, mod_size);

    /* Do exp mod operation. */
    armDCacheFlush(layout, sizeof(*layout));
    {
        std::scoped_lock<HosMutex> lk(g_async_op_lock);
        AsyncOperationKey op_key;

        SmcResult res = SmcWrapper::ExpMod(&op_key, layout->base, layout->exp, exp_size, layout->mod);
        if (res != SmcResult_Success) {
            return ConvertToSplResult(res);
        }

        if ((res = WaitGetResult(g_work_buffer, out_size, op_key)) != SmcResult_Success) {
            return ConvertToSplResult(res);
        }
    }
    armDCacheFlush(g_work_buffer, sizeof(out_size));

    std::memcpy(out, g_work_buffer, out_size);
    return ResultSuccess;
}

Result SecureMonitorWrapper::SetConfig(SplConfigItem which, u64 value) {
    return ConvertToSplResult(SmcWrapper::SetConfig(which, &value, 1));
}

Result SecureMonitorWrapper::GenerateRandomBytesInternal(void *out, size_t size) {
    if (!g_drbg.GenerateRandomBytes(out, size)) {
        /* We need to reseed. */
        {
            u8 seed[CtrDrbg::SeedSize];

            SmcResult res = SmcWrapper::GenerateRandomBytes(seed, sizeof(seed));
            if (res != SmcResult_Success) {
                return ConvertToSplResult(res);
            }

            g_drbg.Reseed(seed);
            g_drbg.GenerateRandomBytes(out, size);
        }
    }

    return ResultSuccess;
}

Result SecureMonitorWrapper::GenerateRandomBytes(void *out, size_t size) {
    u8 *cur_dst = reinterpret_cast<u8 *>(out);

    for (size_t ofs = 0; ofs < size; ofs += CtrDrbg::MaxRequestSize) {
        const size_t cur_size = std::min(size - ofs, CtrDrbg::MaxRequestSize);

        Result rc = GenerateRandomBytesInternal(cur_dst, size);
        if (R_FAILED(rc)) {
            return rc;
        }
        cur_dst += cur_size;
    }

    return ResultSuccess;
}

Result SecureMonitorWrapper::IsDevelopment(bool *out) {
    u64 is_retail;
    Result rc = this->GetConfig(&is_retail, SplConfigItem_IsRetail);
    if (R_FAILED(rc)) {
        return rc;
    }

    *out = (is_retail == 0);
    return ResultSuccess;
}

Result SecureMonitorWrapper::SetBootReason(BootReasonValue boot_reason) {
    if (this->IsBootReasonSet()) {
        return ResultSplBootReasonAlreadySet;
    }

    this->boot_reason = boot_reason;
    this->boot_reason_set = true;
    return ResultSuccess;
}

Result SecureMonitorWrapper::GetBootReason(BootReasonValue *out) {
    if (!this->IsBootReasonSet()) {
        return ResultSplBootReasonNotSet;
    }

    *out = GetBootReason();
    return ResultSuccess;
}

Result SecureMonitorWrapper::GenerateAesKek(AccessKey *out_access_key, const KeySource &key_source, u32 generation, u32 option) {
    return ConvertToSplResult(SmcWrapper::GenerateAesKek(out_access_key, key_source, generation, option));
}

Result SecureMonitorWrapper::LoadAesKey(u32 keyslot, const void *owner, const AccessKey &access_key, const KeySource &key_source) {
    Result rc = ValidateAesKeyslot(keyslot, owner);
    if (R_FAILED(rc)) {
        return rc;
    }
    return ConvertToSplResult(SmcWrapper::LoadAesKey(keyslot, access_key, key_source));
}

Result SecureMonitorWrapper::GenerateAesKey(AesKey *out_key, const AccessKey &access_key, const KeySource &key_source) {
    Result rc;
    SmcResult smc_rc;

    static const KeySource s_generate_aes_key_source = {
        .data = {0x89, 0x61, 0x5E, 0xE0, 0x5C, 0x31, 0xB6, 0x80, 0x5F, 0xE5, 0x8F, 0x3D, 0xA2, 0x4F, 0x7A, 0xA8}
    };

    ScopedAesKeyslot keyslot_holder(this);
    if (R_FAILED((rc = keyslot_holder.Allocate()))) {
        return rc;
    }

    smc_rc = SmcWrapper::LoadAesKey(keyslot_holder.GetKeyslot(), access_key, s_generate_aes_key_source);
    if (smc_rc == SmcResult_Success) {
        smc_rc = DecryptAesBlock(keyslot_holder.GetKeyslot(), out_key, &key_source);
    }

    return ConvertToSplResult(smc_rc);
}

Result SecureMonitorWrapper::DecryptAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 option) {
    Result rc;

    static const KeySource s_decrypt_aes_key_source = {
        .data = {0x11, 0x70, 0x24, 0x2B, 0x48, 0x69, 0x11, 0xF1, 0x11, 0xB0, 0x0C, 0x47, 0x7C, 0xC3, 0xEF, 0x7E}
    };

    AccessKey access_key;
    if (R_FAILED((rc = GenerateAesKek(&access_key, s_decrypt_aes_key_source, generation, option)))) {
        return rc;
    }

    return GenerateAesKey(out_key, access_key, key_source);
}

Result SecureMonitorWrapper::CryptAesCtr(void *dst, size_t dst_size, u32 keyslot, const void *owner, const void *src, size_t src_size, const IvCtr &iv_ctr) {
    Result rc = ValidateAesKeyslot(keyslot, owner);
    if (R_FAILED(rc)) {
        return rc;
    }

    /* Succeed immediately if there's nothing to crypt. */
    if (src_size == 0) {
        return ResultSuccess;
    }

    /* Validate sizes. */
    if (src_size > dst_size || src_size % AES_BLOCK_SIZE != 0) {
        return ResultSplInvalidSize;
    }

    /* We can only map 0x400000 aligned buffers for the SE. With that in mind, we have some math to do. */
    const uintptr_t src_addr = reinterpret_cast<uintptr_t>(src);
    const uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);
    const uintptr_t src_addr_page_aligned = src_addr & ~0xFFFul;
    const uintptr_t dst_addr_page_aligned = dst_addr & ~0xFFFul;
    const size_t src_size_page_aligned = ((src_addr + src_size + 0xFFFul) & ~0xFFFul) - src_addr_page_aligned;
    const size_t dst_size_page_aligned = ((dst_addr + dst_size + 0xFFFul) & ~0xFFFul) - dst_addr_page_aligned;
    const u32 src_se_map_addr = CryptAesInMapBase + (src_addr_page_aligned & DeviceAddressSpaceAlignMask);
    const u32 dst_se_map_addr = CryptAesOutMapBase + (dst_addr_page_aligned & DeviceAddressSpaceAlignMask);
    const u32 src_se_addr = CryptAesInMapBase + (src_addr & DeviceAddressSpaceAlignMask);
    const u32 dst_se_addr = CryptAesOutMapBase + (dst_addr & DeviceAddressSpaceAlignMask);

    /* Validate aligned sizes. */
    if (src_size_page_aligned > CryptAesSizeMax || dst_size_page_aligned > CryptAesSizeMax) {
        return ResultSplInvalidSize;
    }

    /* Helpers for mapping/unmapping. */
    DeviceAddressSpaceMapHelper in_mapper(g_se_das_hnd,  src_se_map_addr, src_addr_page_aligned, src_size_page_aligned, 1);
    DeviceAddressSpaceMapHelper out_mapper(g_se_das_hnd, dst_se_map_addr, dst_addr_page_aligned, dst_size_page_aligned, 2);

    /* Setup SE linked list entries. */
    SeCryptContext *crypt_ctx = reinterpret_cast<SeCryptContext *>(g_work_buffer);
    crypt_ctx->in.num_entries = 0;
    crypt_ctx->in.address = src_se_addr;
    crypt_ctx->in.size = src_size;
    crypt_ctx->out.num_entries = 0;
    crypt_ctx->out.address = dst_se_addr;
    crypt_ctx->out.size = dst_size;

    armDCacheFlush(crypt_ctx, sizeof(*crypt_ctx));
    armDCacheFlush(const_cast<void *>(src), src_size);
    armDCacheFlush(dst, dst_size);
    {
        std::scoped_lock<HosMutex> lk(g_async_op_lock);
        AsyncOperationKey op_key;
        const u32 mode = SmcWrapper::GetCryptAesMode(SmcCipherMode_Ctr, keyslot);
        const u32 dst_ll_addr = g_se_mapped_work_buffer_addr + offsetof(SeCryptContext, out);
        const u32 src_ll_addr = g_se_mapped_work_buffer_addr + offsetof(SeCryptContext, in);

        SmcResult res = SmcWrapper::CryptAes(&op_key, mode, iv_ctr, dst_ll_addr, src_ll_addr, src_size);
        if (res != SmcResult_Success) {
            return ConvertToSplResult(res);
        }

        if ((res = WaitCheckStatus(op_key)) != SmcResult_Success) {
            return ConvertToSplResult(res);
        }
    }
    armDCacheFlush(dst, dst_size);

    return ResultSuccess;
}

Result SecureMonitorWrapper::ComputeCmac(Cmac *out_cmac, u32 keyslot, const void *owner, const void *data, size_t size) {
    Result rc = ValidateAesKeyslot(keyslot, owner);
    if (R_FAILED(rc)) {
        return rc;
    }

    if (size > MaxWorkBufferSize) {
        return ResultSplInvalidSize;
    }

    std::memcpy(g_work_buffer, data, size);
    return ConvertToSplResult(SmcWrapper::ComputeCmac(out_cmac, keyslot, g_work_buffer, size));
}

Result SecureMonitorWrapper::AllocateAesKeyslot(u32 *out_keyslot, const void *owner) {
    if (GetRuntimeFirmwareVersion() <= FirmwareVersion_100) {
        /* On 1.0.0, keyslots were kind of a wild west. */
        *out_keyslot = 0;
        return ResultSuccess;
    }

    for (size_t i = 0; i < GetMaxKeyslots(); i++) {
        if (this->keyslot_owners[i] == 0) {
            this->keyslot_owners[i] = owner;
            *out_keyslot = static_cast<u32>(i);
            return ResultSuccess;
        }
    }

    g_se_keyslot_available_event->Clear();
    return ResultSplOutOfKeyslots;
}

Result SecureMonitorWrapper::ValidateAesKeyslot(u32 keyslot, const void *owner) {
    if (keyslot >= GetMaxKeyslots()) {
        return ResultSplInvalidKeyslot;
    }
    if (this->keyslot_owners[keyslot] != owner && GetRuntimeFirmwareVersion() > FirmwareVersion_100) {
        return ResultSplInvalidKeyslot;
    }
    return ResultSuccess;
}

Result SecureMonitorWrapper::FreeAesKeyslot(u32 keyslot, const void *owner) {
    if (GetRuntimeFirmwareVersion() <= FirmwareVersion_100) {
        /* On 1.0.0, keyslots were kind of a wild west. */
        return ResultSuccess;
    }

    Result rc = ValidateAesKeyslot(keyslot, owner);
    if (R_FAILED(rc)) {
        return rc;
    }

    /* Clear the keyslot. */
    {
        AccessKey access_key = {};
        KeySource key_source = {};

        SmcWrapper::LoadAesKey(keyslot, access_key, key_source);
    }
    this->keyslot_owners[keyslot] = nullptr;
    g_se_keyslot_available_event->Signal();
    return ResultSuccess;
}

Result SecureMonitorWrapper::DecryptRsaPrivateKey(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
    struct DecryptRsaPrivateKeyLayout {
        u8 data[RsaPrivateKeySize + RsaPrivateKeyMetaSize];
    };
    DecryptRsaPrivateKeyLayout *layout = reinterpret_cast<DecryptRsaPrivateKeyLayout *>(g_work_buffer);

    /* Validate size. */
    if (src_size < RsaPrivateKeyMetaSize || src_size > sizeof(DecryptRsaPrivateKeyLayout)) {
        return ResultSplInvalidSize;
    }

    std::memcpy(layout->data, src, src_size);
    armDCacheFlush(layout, sizeof(*layout));

    SmcResult smc_res;
    size_t copy_size = 0;
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
        copy_size = std::min(dst_size, src_size - RsaPrivateKeyMetaSize);
        smc_res = SmcWrapper::DecryptOrImportRsaPrivateKey(layout->data, src_size, access_key, key_source, SmcDecryptOrImportMode_DecryptRsaPrivateKey);
    } else {
        smc_res = SmcWrapper::DecryptRsaPrivateKey(&copy_size, layout->data, src_size, access_key, key_source, option);
        copy_size = std::min(dst_size, copy_size);
    }

    armDCacheFlush(layout, sizeof(*layout));
    if (smc_res == SmcResult_Success) {
        std::memcpy(dst, layout->data, copy_size);
    }

    return ConvertToSplResult(smc_res);
}

Result SecureMonitorWrapper::ImportSecureExpModKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
    struct ImportSecureExpModKeyLayout {
        u8 data[RsaPrivateKeyMetaSize + 2 * RsaPrivateKeySize + 0x10];
    };
    ImportSecureExpModKeyLayout *layout = reinterpret_cast<ImportSecureExpModKeyLayout *>(g_work_buffer);

    /* Validate size. */
    if (src_size > sizeof(ImportSecureExpModKeyLayout)) {
        return ResultSplInvalidSize;
    }

    std::memcpy(layout, src, src_size);

    armDCacheFlush(layout, sizeof(*layout));
    SmcResult smc_res;
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
        smc_res = SmcWrapper::DecryptOrImportRsaPrivateKey(layout->data, src_size, access_key, key_source, option);
    } else {
        smc_res = SmcWrapper::ImportSecureExpModKey(layout->data, src_size, access_key, key_source, option);
    }

    return ConvertToSplResult(smc_res);
}

Result SecureMonitorWrapper::SecureExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size, u32 option) {
    struct SecureExpModLayout {
        u8 base[0x100];
        u8 mod[0x100];
    };
    SecureExpModLayout *layout = reinterpret_cast<SecureExpModLayout *>(g_work_buffer);

    /* Validate sizes. */
    if (base_size > sizeof(layout->base)) {
        return ResultSplInvalidSize;
    }
    if (mod_size > sizeof(layout->mod)) {
        return ResultSplInvalidSize;
    }
    if (out_size > MaxWorkBufferSize) {
        return ResultSplInvalidSize;
    }

    /* Copy data into work buffer. */
    const size_t base_ofs = sizeof(layout->base) - base_size;
    const size_t mod_ofs = sizeof(layout->mod) - mod_size;
    std::memset(layout, 0, sizeof(*layout));
    std::memcpy(layout->base + base_ofs, base, base_size);
    std::memcpy(layout->mod + mod_ofs, mod, mod_size);

    /* Do exp mod operation. */
    armDCacheFlush(layout, sizeof(*layout));
    {
        std::scoped_lock<HosMutex> lk(g_async_op_lock);
        AsyncOperationKey op_key;

        SmcResult res = SmcWrapper::SecureExpMod(&op_key, layout->base, layout->mod, option);
        if (res != SmcResult_Success) {
            return ConvertToSplResult(res);
        }

        if ((res = WaitGetResult(g_work_buffer, out_size, op_key)) != SmcResult_Success) {
            return ConvertToSplResult(res);
        }
    }
    armDCacheFlush(g_work_buffer, sizeof(out_size));

    std::memcpy(out, g_work_buffer, out_size);
    return ResultSuccess;
}

Result SecureMonitorWrapper::ImportSslKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source) {
    return ImportSecureExpModKey(src, src_size, access_key, key_source, SmcDecryptOrImportMode_ImportSslKey);
}

Result SecureMonitorWrapper::SslExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size) {
    return SecureExpMod(out, out_size, base, base_size, mod, mod_size, SmcSecureExpModMode_Ssl);
}

Result SecureMonitorWrapper::ImportEsKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
        return ImportSecureExpModKey(src, src_size, access_key, key_source, SmcDecryptOrImportMode_ImportEsKey);
    } else {
        struct ImportEsKeyLayout {
            u8 data[RsaPrivateKeyMetaSize + 2 * RsaPrivateKeySize + 0x10];
        };
        ImportEsKeyLayout *layout = reinterpret_cast<ImportEsKeyLayout *>(g_work_buffer);

        /* Validate size. */
        if (src_size > sizeof(ImportEsKeyLayout)) {
            return ResultSplInvalidSize;
        }

        std::memcpy(layout, src, src_size);

        armDCacheFlush(layout, sizeof(*layout));
        return ConvertToSplResult(SmcWrapper::ImportEsKey(layout->data, src_size, access_key, key_source, option));
    }
}

Result SecureMonitorWrapper::UnwrapEsRsaOaepWrappedKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation, EsKeyType type) {
    struct UnwrapEsKeyLayout {
        u8 base[0x100];
        u8 mod[0x100];
    };
    UnwrapEsKeyLayout *layout = reinterpret_cast<UnwrapEsKeyLayout *>(g_work_buffer);

    /* Validate sizes. */
    if (base_size > sizeof(layout->base)) {
        return ResultSplInvalidSize;
    }
    if (mod_size > sizeof(layout->mod)) {
        return ResultSplInvalidSize;
    }
    if (label_digest_size > LabelDigestSizeMax) {
        return ResultSplInvalidSize;
    }

    /* Copy data into work buffer. */
    const size_t base_ofs = sizeof(layout->base) - base_size;
    const size_t mod_ofs = sizeof(layout->mod) - mod_size;
    std::memset(layout, 0, sizeof(*layout));
    std::memcpy(layout->base + base_ofs, base, base_size);
    std::memcpy(layout->mod + mod_ofs, mod, mod_size);

    /* Do exp mod operation. */
    armDCacheFlush(layout, sizeof(*layout));
    {
        std::scoped_lock<HosMutex> lk(g_async_op_lock);
        AsyncOperationKey op_key;

        SmcResult res = SmcWrapper::UnwrapTitleKey(&op_key, layout->base, layout->mod, label_digest, label_digest_size, SmcWrapper::GetUnwrapEsKeyOption(type, generation));
        if (res != SmcResult_Success) {
            return ConvertToSplResult(res);
        }

        if ((res = WaitGetResult(g_work_buffer, sizeof(*out_access_key), op_key)) != SmcResult_Success) {
            return ConvertToSplResult(res);
        }
    }
    armDCacheFlush(g_work_buffer, sizeof(*out_access_key));

    std::memcpy(out_access_key, g_work_buffer, sizeof(*out_access_key));
    return ResultSuccess;
}

Result SecureMonitorWrapper::UnwrapTitleKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation) {
    return UnwrapEsRsaOaepWrappedKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, generation, EsKeyType_TitleKey);
}

Result SecureMonitorWrapper::UnwrapCommonTitleKey(AccessKey *out_access_key, const KeySource &key_source, u32 generation) {
    return ConvertToSplResult(SmcWrapper::UnwrapCommonTitleKey(out_access_key, key_source, generation));
}

Result SecureMonitorWrapper::ImportDrmKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source) {
    return ImportSecureExpModKey(src, src_size, access_key, key_source, SmcDecryptOrImportMode_ImportDrmKey);
}

Result SecureMonitorWrapper::DrmExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size) {
    return SecureExpMod(out, out_size, base, base_size, mod, mod_size, SmcSecureExpModMode_Drm);
}

Result SecureMonitorWrapper::UnwrapElicenseKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation) {
    return UnwrapEsRsaOaepWrappedKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, generation, EsKeyType_ElicenseKey);
}

Result SecureMonitorWrapper::LoadElicenseKey(u32 keyslot, const void *owner, const AccessKey &access_key) {
    /* Right now, this is just literally the same function as LoadTitleKey in N's impl. */
    return LoadTitleKey(keyslot, owner, access_key);
}

Result SecureMonitorWrapper::ImportLotusKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
        option = SmcDecryptOrImportMode_ImportLotusKey;
    }
    return ImportSecureExpModKey(src, src_size, access_key, key_source, option);
}

Result SecureMonitorWrapper::DecryptLotusMessage(u32 *out_size, void *dst, size_t dst_size, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size) {
    /* Validate sizes. */
    if (dst_size > MaxWorkBufferSize || label_digest_size != LabelDigestSizeMax) {
        return ResultSplInvalidSize;
    }

    /* Nintendo doesn't check this result code, but we will. */
    Result rc = SecureExpMod(g_work_buffer, 0x100, base, base_size, mod, mod_size, SmcSecureExpModMode_Lotus);
    if (R_FAILED(rc)) {
        return rc;
    }

    size_t data_size = DecodeRsaOaep(dst, dst_size, label_digest, label_digest_size, g_work_buffer, 0x100);
    if (data_size == 0) {
        return ResultSplDecryptionFailed;
    }

    *out_size = static_cast<u32>(data_size);
    return ResultSuccess;
}

Result SecureMonitorWrapper::GenerateSpecificAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 which) {
    return ConvertToSplResult(SmcWrapper::GenerateSpecificAesKey(out_key, key_source, generation, which));
}

Result SecureMonitorWrapper::LoadTitleKey(u32 keyslot, const void *owner, const AccessKey &access_key) {
    Result rc = ValidateAesKeyslot(keyslot, owner);
    if (R_FAILED(rc)) {
        return rc;
    }
    return ConvertToSplResult(SmcWrapper::LoadTitleKey(keyslot, access_key));
}

Result SecureMonitorWrapper::GetPackage2Hash(void *dst, const size_t size) {
    u64 hash[4];

    if (size < sizeof(hash)) {
        return ResultSplInvalidSize;
    }

    SmcResult smc_res;
    if ((smc_res = SmcWrapper::GetConfig(hash, 4, SplConfigItem_Package2Hash)) != SmcResult_Success) {
        return ConvertToSplResult(smc_res);
    }

    std::memcpy(dst, hash, sizeof(hash));
    return ResultSuccess;
}

Result SecureMonitorWrapper::ReEncryptRsaPrivateKey(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
    struct ReEncryptRsaPrivateKeyLayout {
        u8 data[RsaPrivateKeyMetaSize + 2 * RsaPrivateKeySize + 0x10];
        AccessKey access_key_dec;
        KeySource source_dec;
        AccessKey access_key_enc;
        KeySource source_enc;
    };
    ReEncryptRsaPrivateKeyLayout *layout = reinterpret_cast<ReEncryptRsaPrivateKeyLayout *>(g_work_buffer);

    /* Validate size. */
    if (src_size < RsaPrivateKeyMetaSize || src_size > sizeof(ReEncryptRsaPrivateKeyLayout)) {
        return ResultSplInvalidSize;
    }

    std::memcpy(layout, src, src_size);
    layout->access_key_dec = access_key_dec;
    layout->source_dec = source_dec;
    layout->access_key_enc = access_key_enc;
    layout->source_enc = source_enc;

    armDCacheFlush(layout, sizeof(*layout));

    SmcResult smc_res = SmcWrapper::ReEncryptRsaPrivateKey(layout->data, src_size, layout->access_key_dec, layout->source_dec, layout->access_key_enc, layout->source_enc, option);
    if (smc_res == SmcResult_Success) {
        size_t copy_size = std::min(dst_size, src_size);
        armDCacheFlush(layout, copy_size);
        std::memcpy(dst, layout->data, copy_size);
    }

    return ConvertToSplResult(smc_res);

}

Result SecureMonitorWrapper::FreeAesKeyslots(const void *owner) {
    for (size_t i = 0; i < GetMaxKeyslots(); i++) {
        if (this->keyslot_owners[i] == owner) {
            FreeAesKeyslot(i, owner);
        }
    }
    return ResultSuccess;
}

Handle SecureMonitorWrapper::GetAesKeyslotAvailableEventHandle() {
    return g_se_keyslot_available_event->GetHandle();
}