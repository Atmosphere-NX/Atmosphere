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
#include <vapours.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_i_file_system_proxy.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_i_program_registry.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_i_file_system_proxy_for_loader.hpp>

namespace ams::fssrv {

    namespace impl {

        class FileSystemProxyCoreImpl;

    }

    class FileSystemProxyImpl {
        NON_COPYABLE(FileSystemProxyImpl);
        NON_MOVEABLE(FileSystemProxyImpl);
        private:
            impl::FileSystemProxyCoreImpl *m_impl;
            /* TODO: service pointers. */
            u64 m_process_id;
        public:
            FileSystemProxyImpl();
            ~FileSystemProxyImpl();

            /* TODO */

        public:
            /* fsp-ldr */
            Result OpenCodeFileSystemDeprecated(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id);
            Result OpenCodeFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id);
            Result IsArchivedProgram(ams::sf::Out<bool> out, u64 process_id);
            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid);
    };
    static_assert(sf::IsIFileSystemProxy<FileSystemProxyImpl>);
    static_assert(sf::IsIFileSystemProxyForLoader<FileSystemProxyImpl>);

    class InvalidFileSystemProxyImplForLoader {
        public:
            Result OpenCodeFileSystemDeprecated(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
                AMS_UNUSED(out_fs, path, program_id);
                return fs::ResultPortAcceptableCountLimited();
            }

            Result OpenCodeFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
                AMS_UNUSED(out_fs, out_verif, path, program_id);
                return fs::ResultPortAcceptableCountLimited();
            }

            Result IsArchivedProgram(ams::sf::Out<bool> out, u64 process_id) {
                AMS_UNUSED(out, process_id);
                return fs::ResultPortAcceptableCountLimited();
            }

            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
                AMS_UNUSED(client_pid);
                return fs::ResultPortAcceptableCountLimited();
            }
    };
    static_assert(sf::IsIFileSystemProxyForLoader<FileSystemProxyImpl>);

}
