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
#include "impl/fssrv_program_info.hpp"

namespace ams::fssrv {

    FileSystemProxyImpl::FileSystemProxyImpl() {
        /* TODO: Set core impl. */
        m_process_id = os::InvalidProcessId.value;
    }

    FileSystemProxyImpl::~FileSystemProxyImpl() {
        /* ... */
    }

    Result FileSystemProxyImpl::OpenCodeFileSystemDeprecated(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out_fs, path, program_id);
    }

    Result FileSystemProxyImpl::OpenCodeFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out_fs, out_verif, path, program_id);
    }

    Result FileSystemProxyImpl::IsArchivedProgram(ams::sf::Out<bool> out, u64 process_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out, process_id);
    }

    Result FileSystemProxyImpl::SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
        /* Set current process. */
        m_process_id = client_pid.GetValue().value;

        /* TODO: Allocate NcaFileSystemService. */
        AMS_ABORT("TODO");
    }

}
