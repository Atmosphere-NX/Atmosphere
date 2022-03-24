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
#include <stratosphere/fssrv/fssrv_file_system_proxy_impl.hpp>

namespace ams::fssrv::impl {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    ams::sf::EmplacedRef<fssrv::sf::IFileSystemProxy, fssrv::FileSystemProxyImpl> GetFileSystemProxyServiceObject();

    ams::sf::SharedPointer<fssrv::sf::IProgramRegistry> GetProgramRegistryServiceObject();
    ams::sf::SharedPointer<fssrv::sf::IProgramRegistry> GetInvalidProgramRegistryServiceObject();

    ams::sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader> GetFileSystemProxyForLoaderServiceObject();
    ams::sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader> GetInvalidFileSystemProxyForLoaderServiceObject();

}
