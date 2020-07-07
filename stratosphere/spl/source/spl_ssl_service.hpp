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
#pragma once
#include "spl_device_unique_data_service.hpp"

namespace ams::spl {

    class SslService : public DeviceUniqueDataService {
        public:
            /* Actual commands. */
            Result DecryptAndStoreSslClientCertKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source);
            Result ModularExponentiateWithSslClientCertKey(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod);
    };
    static_assert(spl::impl::IsISslInterface<SslService>);

}
