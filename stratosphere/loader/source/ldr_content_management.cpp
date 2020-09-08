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
#include "ldr_content_management.hpp"

namespace ams::ldr {

    namespace {

        os::Mutex g_scoped_code_mount_lock(false);

    }

    /* ScopedCodeMount functionality. */
    ScopedCodeMount::ScopedCodeMount(const ncm::ProgramLocation &loc) : lk(g_scoped_code_mount_lock), has_status(false), mounted_ams(false), mounted_sd_or_code(false), mounted_code(false) {
        this->result = this->Initialize(loc);
    }

    ScopedCodeMount::ScopedCodeMount(const ncm::ProgramLocation &loc, const cfg::OverrideStatus &o) : lk(g_scoped_code_mount_lock), override_status(o), has_status(true), mounted_ams(false), mounted_sd_or_code(false), mounted_code(false) {
        this->result = this->Initialize(loc);
    }

    ScopedCodeMount::~ScopedCodeMount() {
        /* Unmount filesystems. */
        if (this->mounted_ams) {
            fs::Unmount(AtmosphereCodeMountName);
        }
        if (this->mounted_sd_or_code) {
            fs::Unmount(SdOrCodeMountName);
        }
        if (this->mounted_code) {
            fs::Unmount(CodeMountName);
        }
    }

    Result ScopedCodeMount::Initialize(const ncm::ProgramLocation &loc) {
        /* Capture override status, if necessary. */
        this->EnsureOverrideStatus(loc);
        AMS_ABORT_UNLESS(this->has_status);

        /* Get the content path. */
        char content_path[fs::EntryNameLengthMax + 1] = "/";
        if (static_cast<ncm::StorageId>(loc.storage_id) != ncm::StorageId::None) {
            R_TRY(ResolveContentPath(content_path, loc));
        }

        /* Mount the atmosphere code file system. */
        R_TRY(fs::MountCodeForAtmosphereWithRedirection(std::addressof(this->ams_code_verification_data), AtmosphereCodeMountName, content_path, loc.program_id, this->override_status.IsHbl(), this->override_status.IsProgramSpecific()));
        this->mounted_ams = true;

        /* Mount the sd or base code file system. */
        R_TRY(fs::MountCodeForAtmosphere(std::addressof(this->sd_or_base_code_verification_data), SdOrCodeMountName, content_path, loc.program_id));
        this->mounted_sd_or_code = true;

        /* Mount the base code file system. */
        if (R_SUCCEEDED(fs::MountCode(std::addressof(this->base_code_verification_data), CodeMountName, content_path, loc.program_id))) {
            this->mounted_code = true;
        }

        return ResultSuccess();
    }

    void ScopedCodeMount::EnsureOverrideStatus(const ncm::ProgramLocation &loc) {
        if (this->has_status) {
            return;
        }
        this->override_status = cfg::CaptureOverrideStatus(loc.program_id);
        this->has_status = true;
    }

    /* Redirection API. */
    Result ResolveContentPath(char *out_path, const ncm::ProgramLocation &loc) {
        lr::Path path;

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

        std::strncpy(out_path, path.str, fs::EntryNameLengthMax);
        out_path[fs::EntryNameLengthMax - 1] = '\0';

        fs::Replace(out_path, fs::EntryNameLengthMax + 1, fs::StringTraits::AlternateDirectorySeparator, fs::StringTraits::DirectorySeparator);

        return ResultSuccess();
    }

    Result RedirectContentPath(const char *path, const ncm::ProgramLocation &loc) {
        /* Copy in path. */
        lr::Path lr_path;
        std::strncpy(lr_path.str, path, sizeof(lr_path.str));
        lr_path.str[sizeof(lr_path.str) - 1] = '\0';

        /* Redirect the path. */
        lr::LocationResolver lr;
        R_TRY(lr::OpenLocationResolver(std::addressof(lr),  static_cast<ncm::StorageId>(loc.storage_id)));
        lr.RedirectProgramPath(lr_path, loc.program_id);

        return ResultSuccess();
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

        return ResultSuccess();
    }

}
