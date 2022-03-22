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
#include "fssrv_allocator_for_service_framework.hpp"

namespace ams::fssrv::impl {

    namespace {

        using FileSystemProxyServiceFactory          = ams::sf::ObjectFactory<AllocatorForServiceFramework::Policy>;
        using ProgramRegistryServiceFactory          = ams::sf::ObjectFactory<AllocatorForServiceFramework::Policy>;
        using FileSystemProxyForLoaderServiceFactory = ams::sf::ObjectFactory<AllocatorForServiceFramework::Policy>;

    }

    ams::sf::EmplacedRef<fssrv::sf::IFileSystemProxy, fssrv::FileSystemProxyImpl> GetFileSystemProxyServiceObject() {
        return FileSystemProxyServiceFactory::CreateSharedEmplaced<fssrv::sf::IFileSystemProxy, fssrv::FileSystemProxyImpl>();
    }

    ams::sf::SharedPointer<fssrv::sf::IProgramRegistry> GetProgramRegistryServiceObject() {
        return ProgramRegistryServiceFactory::CreateSharedEmplaced<fssrv::sf::IProgramRegistry, fssrv::ProgramRegistryImpl>();
    }

    ams::sf::SharedPointer<fssrv::sf::IProgramRegistry> GetInvalidProgramRegistryServiceObject() {
        return ProgramRegistryServiceFactory::CreateSharedEmplaced<fssrv::sf::IProgramRegistry, fssrv::InvalidProgramRegistryImpl>();
    }

    ams::sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader> GetFileSystemProxyForLoaderServiceObject() {
        return FileSystemProxyForLoaderServiceFactory ::CreateSharedEmplaced<fssrv::sf::IFileSystemProxyForLoader, fssrv::FileSystemProxyImpl>();
    }

    ams::sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader> GetInvalidFileSystemProxyForLoaderServiceObject() {
        return FileSystemProxyForLoaderServiceFactory ::CreateSharedEmplaced<fssrv::sf::IFileSystemProxyForLoader, fssrv::InvalidFileSystemProxyImplForLoader>();
    }

}
