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
#include "gpio_remote_manager_impl.hpp"

namespace ams::gpio {

    namespace {

        struct GpioRemoteManagerTag;
        using RemoteAllocator     = ams::sf::ExpHeapStaticAllocator<3_KB, GpioRemoteManagerTag>;
        using RemoteObjectFactory = ams::sf::ObjectFactory<typename RemoteAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    RemoteAllocator::Initialize(lmem::CreateOption_None);
                }
        } g_static_allocator_initializer;

    }

    Result RemoteManagerImpl::OpenSession(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name) {
        ::GpioPadSession p;
        R_TRY(::gpioOpenSession(std::addressof(p), static_cast<::GpioPadName>(static_cast<u32>(pad_name))));

        out.SetValue(RemoteObjectFactory::CreateSharedEmplaced<gpio::sf::IPadSession, RemotePadSessionImpl>(p));
        return ResultSuccess();
    }

    Result RemoteManagerImpl::OpenSession2(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, DeviceCode device_code, ddsf::AccessMode access_mode) {
        ::GpioPadSession p;
        R_TRY(::gpioOpenSession2(std::addressof(p), device_code.GetInternalValue(), access_mode));

        out.SetValue(RemoteObjectFactory::CreateSharedEmplaced<gpio::sf::IPadSession, RemotePadSessionImpl>(p));
        return ResultSuccess();
    }

}
