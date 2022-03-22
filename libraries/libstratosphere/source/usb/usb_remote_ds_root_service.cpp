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
#include "usb_remote_ds_root_service.hpp"
#include "usb_remote_ds_service.hpp"

namespace ams::usb {

    Result RemoteDsRootService::GetService(sf::Out<sf::SharedPointer<usb::ds::IDsService>> out) {
        Service srv;

        serviceAssumeDomain(std::addressof(m_srv));
        R_TRY(serviceDispatch(std::addressof(m_srv), 0, .out_num_objects = 1, .out_objects = std::addressof(srv)));

        *out = ObjectFactory::CreateSharedEmplaced<ds::IDsService, RemoteDsService>(m_allocator, srv, m_allocator);

        return ResultSuccess();
    }

}
