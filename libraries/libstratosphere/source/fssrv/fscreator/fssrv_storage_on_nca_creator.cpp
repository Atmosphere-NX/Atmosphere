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

namespace ams::fssrv::fscreator {

    Result StorageOnNcaCreator::VerifyAcid(fs::fsa::IFileSystem *fs, fssystem::NcaReader *nca_reader) {
        /* Open the npdm. */
        constexpr const char MetaFilePath[] = "/main.npdm";
        std::unique_ptr<fs::fsa::IFile> file;
        R_TRY(fs->OpenFile(std::addressof(file), MetaFilePath, fs::OpenMode_Read));

        size_t size;

        /* Read the Acid signature key generation. */
        constexpr s64 AcidSignatureKeyGenerationOffset = offsetof(ldr::Npdm, signature_key_generation);
        u32 acid_signature_key_generation;
        R_TRY(file->Read(std::addressof(size), AcidSignatureKeyGenerationOffset, std::addressof(acid_signature_key_generation), sizeof(acid_signature_key_generation), fs::ReadOption()));
        R_UNLESS(size == sizeof(acid_signature_key_generation), fs::ResultInvalidAcidFileSize());

        /* Read the Acid offset. */
        constexpr s64 AcidOffsetOffset = offsetof(ldr::Npdm, acid_offset);
        s32 acid_offset;
        R_TRY(file->Read(std::addressof(size), AcidOffsetOffset, std::addressof(acid_offset), sizeof(acid_offset), fs::ReadOption()));
        R_UNLESS(size == sizeof(acid_offset), fs::ResultInvalidAcidFileSize());

        /* Read the Acid size. */
        constexpr s64 AcidSizeOffset = offsetof(ldr::Npdm, acid_size);
        s32 acid_size;
        R_TRY(file->Read(std::addressof(size), AcidSizeOffset, std::addressof(acid_size), sizeof(acid_size), fs::ReadOption()));
        R_UNLESS(size == sizeof(acid_size), fs::ResultInvalidAcidFileSize());

        /* Allocate memory for the acid. */
        u8 *acid = static_cast<u8 *>(this->allocator->Allocate(acid_size));
        R_UNLESS(acid != nullptr, fs::ResultAllocationFailureInStorageOnNcaCreatorA());
        ON_SCOPE_EXIT { this->allocator->Deallocate(acid, acid_size); };

        /* Read the acid. */
        R_TRY(file->Read(std::addressof(size), acid_offset, acid, acid_size, fs::ReadOption()));
        R_UNLESS(size == static_cast<size_t>(acid_size), fs::ResultInvalidAcidSize());

        /* Define interesting extents. */
        constexpr s32 AcidSignOffset           = 0x000;
        constexpr s32 AcidSignSize             = 0x100;
        constexpr s32 HeaderSign2KeyOffset     = 0x100;
        constexpr s32 HeaderSign2KeySize       = 0x100;
        constexpr s32 AcidSignTargetOffset     = 0x100;
        constexpr s32 AcidSignTargetSizeOffset = 0x204;

        /* Read the sign target size. */
        R_UNLESS(acid_size >= static_cast<s32>(AcidSignTargetSizeOffset + sizeof(s32)), fs::ResultInvalidAcidSize());
        const s32 acid_sign_target_size = *reinterpret_cast<const s32 *>(acid + AcidSignTargetSizeOffset);

        /* Validate the sign target size. */
        R_UNLESS(acid_size >= static_cast<s32>(acid_sign_target_size + sizeof(s32)), fs::ResultInvalidAcidSize());
        R_UNLESS(acid_size >= AcidSignTargetOffset + acid_sign_target_size,          fs::ResultInvalidAcidSize());

        /* Verify the signature. */
        {
            const u8 *sig         = acid + AcidSignOffset;
            const size_t sig_size = static_cast<size_t>(AcidSignSize);
            const u8 *mod         = fssystem::GetAcidSignatureKeyModulus(this->is_prod, acid_signature_key_generation);
            const size_t mod_size = fssystem::AcidSignatureKeyModulusSize;
            const u8 *exp         = fssystem::GetAcidSignatureKeyPublicExponent();
            const size_t exp_size = fssystem::AcidSignatureKeyPublicExponentSize;
            const u8 *msg         = acid + AcidSignTargetOffset;
            const size_t msg_size = acid_sign_target_size;
            const bool is_signature_valid = crypto::VerifyRsa2048PssSha256(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
            if (!is_signature_valid) {
                /* If the signature is invalid, then unless program verification is disabled error out. */
                R_UNLESS(!this->is_enabled_program_verification, fs::ResultAcidVerificationFailed());

                /* If program verification is disabled, then we're fine. */
                return ResultSuccess();
            }
        }

        /* If we have an nca reader, verify the header signature using the key from the acid. */
        if (nca_reader) {
            /* Verify that the acid contains a key to validate the second signature with. */
            R_UNLESS(acid_size >= HeaderSign2KeyOffset + HeaderSign2KeySize, fs::ResultInvalidAcidSize());

            /* Validate that this key has its top byte set (and is thus approximately 2048 bits). */
            R_UNLESS(*(acid + HeaderSign2KeyOffset + HeaderSign2KeySize - 1) != 0x00, fs::ResultInvalidAcid());

            R_TRY(nca_reader->VerifyHeaderSign2(reinterpret_cast<char *>(acid) + HeaderSign2KeyOffset, HeaderSign2KeySize));
        }

        return ResultSuccess();
    }

    Result StorageOnNcaCreator::Create(std::shared_ptr<fs::IStorage> *out, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> nca_reader, s32 index, bool verify_header_sign_2) {
        /* Create a fs driver. */
        fssystem::NcaFileSystemDriver nca_fs_driver(nca_reader, this->allocator, this->buffer_manager);

        /* Open the storage. */
        std::shared_ptr<fs::IStorage> storage;
        R_TRY(nca_fs_driver.OpenStorage(std::addressof(storage), out_header_reader, index));

        /* If we should, verify the header signature. */
        if (verify_header_sign_2) {
            R_TRY(this->VerifyNcaHeaderSign2(nca_reader.get(), storage.get()));
        }

        /* Set the out storage. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result StorageOnNcaCreator::CreateWithPatch(std::shared_ptr<fs::IStorage> *out, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> original_nca_reader, std::shared_ptr<fssystem::NcaReader> current_nca_reader, s32 index, bool verify_header_sign_2) {
        /* Create a fs driver. */
        fssystem::NcaFileSystemDriver nca_fs_driver(original_nca_reader, current_nca_reader, this->allocator, this->buffer_manager);

        /* Open the storage. */
        std::shared_ptr<fs::IStorage> storage;
        R_TRY(nca_fs_driver.OpenStorage(std::addressof(storage), out_header_reader, index));

        /* If we should, verify the header signature. */
        if (verify_header_sign_2) {
            R_TRY(this->VerifyNcaHeaderSign2(current_nca_reader.get(), storage.get()));
        }

        /* Set the out storage. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result StorageOnNcaCreator::CreateNcaReader(std::shared_ptr<fssystem::NcaReader> *out, std::shared_ptr<fs::IStorage> storage) {
        /* Create a reader. */
        std::shared_ptr reader = fssystem::AllocateShared<fssystem::NcaReader>();
        R_UNLESS(reader != nullptr, fs::ResultAllocationFailureInStorageOnNcaCreatorB());

        /* Initialize the reader. */
        R_TRY(reader->Initialize(std::move(storage), this->nca_crypto_cfg));

        /* Set the output. */
        *out = std::move(reader);
        return ResultSuccess();
    }

    void StorageOnNcaCreator::SetEnabledProgramVerification(bool en) {
        if (!this->is_prod) {
            this->is_enabled_program_verification = en;
        }
    }

    Result StorageOnNcaCreator::VerifyNcaHeaderSign2(fssystem::NcaReader *nca_reader, fs::IStorage *storage) {
        fssystem::PartitionFileSystem part_fs;
        R_TRY(part_fs.Initialize(storage));
        return this->VerifyAcid(std::addressof(part_fs), nca_reader);
    }

}
