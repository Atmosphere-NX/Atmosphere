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
#pragma once
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/fs/fs_code_verification_data.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: 16.2.0.0 */
    Result MountCode(CodeVerificationData *out, const char *name, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id);

    Result MountCodeForAtmosphereWithRedirection(CodeVerificationData *out, const char *name, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id, bool is_hbl, bool is_specific);
    Result MountCodeForAtmosphere(CodeVerificationData *out, const char *name, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id);

}
