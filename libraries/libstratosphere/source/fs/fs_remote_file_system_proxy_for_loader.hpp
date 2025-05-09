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
#include <stratosphere.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include "../fssrv/impl/fssrv_allocator_for_service_framework.hpp"

namespace ams::fs {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteFileSystemProxyForLoader {
        NON_COPYABLE(RemoteFileSystemProxyForLoader);
        NON_MOVEABLE(RemoteFileSystemProxyForLoader);
        private:
            using ObjectFactory = fssrv::impl::FileSystemObjectFactory;
        public:
            RemoteFileSystemProxyForLoader();
            ~RemoteFileSystemProxyForLoader();
        public:
            Result OpenCodeFileSystemDeprecated(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
                ::FsCodeInfo dummy;
                ::FsFileSystem fs;
                R_TRY(fsldrOpenCodeFileSystem(std::addressof(dummy), program_id.value, ::NcmStorageId_None, path.str, static_cast<::FsContentAttributes>(static_cast<u8>(fs::ContentAttributes_None)), std::addressof(fs)));

                out_fs.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result OpenCodeFileSystemDeprecated2(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
                ::FsFileSystem fs;
                R_TRY(fsldrOpenCodeFileSystem(reinterpret_cast<::FsCodeInfo *>(out_verif.GetPointer()), program_id.value, ::NcmStorageId_None, path.str, static_cast<::FsContentAttributes>(static_cast<u8>(fs::ContentAttributes_None)), std::addressof(fs)));

                out_fs.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result OpenCodeFileSystemDeprecated3(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id) {
                ::FsFileSystem fs;
                R_TRY(fsldrOpenCodeFileSystem(reinterpret_cast<::FsCodeInfo *>(out_verif.GetPointer()), program_id.value, ::NcmStorageId_None, path.str, static_cast<::FsContentAttributes>(static_cast<u8>(attr)), std::addressof(fs)));

                out_fs.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result OpenCodeFileSystemDeprecated4(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id) {
                ::FsFileSystem fs;
                R_TRY(fsldrOpenCodeFileSystem(reinterpret_cast<::FsCodeInfo *>(out_verif.GetPointer()), program_id.value, ::NcmStorageId_None, path.str, static_cast<::FsContentAttributes>(static_cast<u8>(attr)), std::addressof(fs)));

                out_fs.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result OpenCodeFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id) {
                ::FsFileSystem fs;
                R_TRY(fsldrOpenCodeFileSystem(reinterpret_cast<::FsCodeInfo *>(out_verif.GetPointer()), program_id.value, static_cast<::NcmStorageId>(static_cast<u8>(storage_id)), nullptr, static_cast<::FsContentAttributes>(static_cast<u8>(attr)), std::addressof(fs)));

                out_fs.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result IsArchivedProgram(ams::sf::Out<bool> out, u64 process_id) {
                R_RETURN(fsldrIsArchivedProgram(process_id, out.GetPointer()));
            }

            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
                /* Libnx does this for us automatically. */
                AMS_UNUSED(client_pid);
                R_SUCCEED();
            }
    };
    static_assert(fssrv::sf::IsIFileSystemProxyForLoader<RemoteFileSystemProxyForLoader>);
    #endif

}
