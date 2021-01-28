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
#include <stratosphere.hpp>

namespace ams::sm {

    /* Service definition. */
    class DebugMonitorService {
        public:
            Result AtmosphereGetRecord(sf::Out<ServiceRecord> record, ServiceName service);
            void AtmosphereListRecords(const sf::OutArray<ServiceRecord> &records, sf::Out<u64> out_count, u64 offset);
            void AtmosphereGetRecordSize(sf::Out<u64> record_size);
    };
    static_assert(sm::impl::IsIDebugMonitorInterface<DebugMonitorService>);

}
