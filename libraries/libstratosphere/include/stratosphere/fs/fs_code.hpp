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
#include "fs_common.hpp"
#include <stratosphere/ncm/ncm_ids.hpp>

namespace ams::fs {

    struct CodeVerificationData {
        u8 signature[crypto::Rsa2048PssSha256Verifier::SignatureSize];
        u8 target_hash[crypto::Rsa2048PssSha256Verifier::HashSize];
        bool has_data;
        u8 reserved[3];
    };
    static_assert(sizeof(CodeVerificationData) == crypto::Rsa2048PssSha256Verifier::SignatureSize + crypto::Rsa2048PssSha256Verifier::HashSize + 4);

    Result MountCode(CodeVerificationData *out, const char *name, const char *path, ncm::ProgramId program_id);

    Result MountCodeForAtmosphereWithRedirection(CodeVerificationData *out, const char *name, const char *path, ncm::ProgramId program_id, bool is_hbl, bool is_specific);
    Result MountCodeForAtmosphere(CodeVerificationData *out, const char *name, const char *path, ncm::ProgramId program_id);

}
