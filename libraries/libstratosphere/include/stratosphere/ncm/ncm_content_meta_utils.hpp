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
#include <stratosphere/ncm/ncm_auto_buffer.hpp>
#include <stratosphere/ncm/ncm_storage_id.hpp>
#include <stratosphere/ncm/ncm_content_storage.hpp>
#include <stratosphere/ncm/ncm_content_meta_key.hpp>
#include <stratosphere/ncm/ncm_content_meta_database.hpp>
#include <stratosphere/ncm/ncm_firmware_variation.hpp>

namespace ams::ncm {

    using MountContentMetaFunction = Result (*)(const char *mount_name, const char *path, fs::ContentAttributes attr);

    bool IsContentMetaFileName(const char *name);

    Result ReadContentMetaPathAlongWithExtendedDataAndDigest(AutoBuffer *out, const char *path, fs::ContentAttributes attr);
    Result ReadContentMetaPathAlongWithExtendedDataAndDigestSuppressingFsAbort(AutoBuffer *out, const char *path, fs::ContentAttributes attr);

    Result ReadContentMetaPathWithoutExtendedDataOrDigest(AutoBuffer *out, const char *path, fs::ContentAttributes attr);
    Result ReadContentMetaPathWithoutExtendedDataOrDigestSuppressingFsAbort(AutoBuffer *out, const char *path, fs::ContentAttributes attr);

    using ReadContentMetaPathFunction = Result (*)(AutoBuffer *out, const char *path, fs::ContentAttributes attr);

    Result TryReadContentMetaPath(fs::ContentAttributes *out_attr, AutoBuffer *out, const char *path, ReadContentMetaPathFunction func);
    Result TryReadContentMetaPath(AutoBuffer *out, const char *path, ReadContentMetaPathFunction func);

    Result ReadVariationContentMetaInfoList(s32 *out_count, std::unique_ptr<ContentMetaInfo[]> *out_meta_infos, const Path &path, fs::ContentAttributes attr, FirmwareVariationId firmware_variation_id);

    void SetMountContentMetaFunction(MountContentMetaFunction func);

}
