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
#include "ldr_content_management.hpp"

namespace ams::ldr {

    namespace {

        constinit os::SdkMutex g_scoped_code_mount_lock;

    }

    /* ScopedCodeMount functionality. */
    ScopedCodeMount::ScopedCodeMount(const ncm::ProgramLocation &loc, const ldr::ProgramAttributes &attrs) : m_lk(g_scoped_code_mount_lock), m_has_status(false), m_mounted_ams(false), m_mounted_sd_or_code(false), m_mounted_code(false) {
        m_result = this->Initialize(loc, attrs);
    }

    ScopedCodeMount::ScopedCodeMount(const ncm::ProgramLocation &loc, const cfg::OverrideStatus &o, const ldr::ProgramAttributes &attrs) : m_lk(g_scoped_code_mount_lock), m_override_status(o), m_has_status(true), m_mounted_ams(false), m_mounted_sd_or_code(false), m_mounted_code(false) {
        m_result = this->Initialize(loc, attrs);
    }

    ScopedCodeMount::~ScopedCodeMount() {
        /* Unmount filesystems. */
        if (m_mounted_ams) {
            fs::Unmount(AtmosphereCodeMountName);
        }
        if (m_mounted_sd_or_code) {
            fs::Unmount(SdOrCodeMountName);
        }
        if (m_mounted_code) {
            fs::Unmount(CodeMountName);
        }
    }

    Result ScopedCodeMount::Initialize(const ncm::ProgramLocation &loc, const ldr::ProgramAttributes &attrs) {
        /* Capture override status, if necessary. */
        this->EnsureOverrideStatus(loc);
        AMS_ABORT_UNLESS(m_has_status);

        /* Get the content path. */
        char content_path[fs::EntryNameLengthMax + 1];
        R_TRY(GetProgramPath(content_path, sizeof(content_path), loc, attrs));

        /* Get the content attributes. */
        const auto content_attributes = attrs.content_attributes;

        /* Mount the atmosphere code file system. */
        R_TRY(fs::MountCodeForAtmosphereWithRedirection(std::addressof(m_ams_code_verification_data), AtmosphereCodeMountName, content_path, content_attributes, loc.program_id, static_cast<ncm::StorageId>(loc.storage_id), m_override_status.IsHbl(), m_override_status.IsProgramSpecific()));
        m_mounted_ams = true;

        /* Mount the sd or base code file system. */
        R_TRY(fs::MountCodeForAtmosphere(std::addressof(m_sd_or_base_code_verification_data), SdOrCodeMountName, content_path, content_attributes, loc.program_id, static_cast<ncm::StorageId>(loc.storage_id)));
        m_mounted_sd_or_code = true;

        /* Mount the base code file system. */
        if (R_SUCCEEDED(fs::MountCode(std::addressof(m_base_code_verification_data), CodeMountName, content_path, content_attributes, loc.program_id, static_cast<ncm::StorageId>(loc.storage_id)))) {
            m_mounted_code = true;
        }

        R_SUCCEED();
    }

    void ScopedCodeMount::EnsureOverrideStatus(const ncm::ProgramLocation &loc) {
        if (m_has_status) {
            return;
        }
        m_override_status = cfg::CaptureOverrideStatus(loc.program_id);
        m_has_status = true;
    }

    /* Redirection API. */
    Result GetProgramPath(char *out_path, size_t out_size, const ncm::ProgramLocation &loc, const ldr::ProgramAttributes &attrs) {
        /* Check for storage id none. */
        if (static_cast<ncm::StorageId>(loc.storage_id) == ncm::StorageId::None) {
            std::memset(out_path, 0, out_size);
            std::memcpy(out_path, "/", std::min<size_t>(out_size, 2));
            R_SUCCEED();
        }

        /* Get the content attributes. */
        const auto content_attributes = attrs.content_attributes;
        AMS_UNUSED(content_attributes);

        lr::Path path;

        /* Check that path registration is allowable. */
        if (static_cast<ncm::StorageId>(loc.storage_id) == ncm::StorageId::Host) {
            AMS_ABORT_UNLESS(spl::IsDevelopment());
        }

        /* Try to get the path from the registered resolver. */
        lr::RegisteredLocationResolver reg;
        R_TRY(lr::OpenRegisteredLocationResolver(std::addressof(reg)));

        R_TRY_CATCH(reg.ResolveProgramPath(std::addressof(path), loc.program_id)) {
            R_CATCH(lr::ResultProgramNotFound) {
                /* Program wasn't found via registered resolver, fall back to the normal resolver. */
                lr::LocationResolver lr;
                R_TRY(lr::OpenLocationResolver(std::addressof(lr), static_cast<ncm::StorageId>(loc.storage_id)));
                R_TRY(lr.ResolveProgramPath(std::addressof(path), loc.program_id));
            }
        } R_END_TRY_CATCH;

        /* Fix directory separators in path. */
        fs::Replace(path.str, sizeof(path.str), fs::StringTraits::AlternateDirectorySeparator, fs::StringTraits::DirectorySeparator);

        /* Check that the path is valid. */
        AMS_ABORT_UNLESS(path.IsValid());

        /* Copy the output path. */
        std::memset(out_path, 0, out_size);
        std::memcpy(out_path, path.str, std::min(out_size, sizeof(path)));

        R_SUCCEED();
    }

    Result RedirectProgramPath(const char *path, size_t size, const ncm::ProgramLocation &loc) {
        /* Check for storage id none. */
        if (static_cast<ncm::StorageId>(loc.storage_id) == ncm::StorageId::None) {
            R_SUCCEED();
        }

        /* Open location resolver. */
        lr::LocationResolver lr;
        R_TRY(lr::OpenLocationResolver(std::addressof(lr),  static_cast<ncm::StorageId>(loc.storage_id)));

        /* Copy in path. */
        lr::Path lr_path;
        std::memcpy(lr_path.str, path, std::min(size, sizeof(lr_path.str)));
        lr_path.str[sizeof(lr_path.str) - 1] = '\x00';

        /* Redirect the path. */
        lr.RedirectProgramPath(lr_path, loc.program_id);

        R_SUCCEED();
    }

    Result RedirectHtmlDocumentPathForHbl(const ncm::ProgramLocation &loc) {
        lr::Path path;

        /* Open a location resolver. */
        lr::LocationResolver lr;
        R_TRY(lr::OpenLocationResolver(std::addressof(lr),  static_cast<ncm::StorageId>(loc.storage_id)));

        /* If there's already a Html Document path, we don't need to set one. */
        R_SUCCEED_IF(R_SUCCEEDED(lr.ResolveApplicationHtmlDocumentPath(std::addressof(path), loc.program_id)));

        /* We just need to set this to any valid NCA path. Let's use the executable path. */
        R_TRY(lr.ResolveProgramPath(std::addressof(path), loc.program_id));
        lr.RedirectApplicationHtmlDocumentPath(path, loc.program_id, loc.program_id);

        R_SUCCEED();
    }

}
