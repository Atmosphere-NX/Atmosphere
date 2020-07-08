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
#include "sm_debug_monitor_service.hpp"
#include "impl/sm_service_manager.hpp"

namespace ams::sm {

    Result DebugMonitorService::AtmosphereGetRecord(sf::Out<ServiceRecord> record, ServiceName service) {
        return impl::GetServiceRecord(record.GetPointer(), service);
    }

    void DebugMonitorService::AtmosphereListRecords(const sf::OutArray<ServiceRecord> &records, sf::Out<u64> out_count, u64 offset) {
        R_ABORT_UNLESS(impl::ListServiceRecords(records.GetPointer(), out_count.GetPointer(), offset, records.GetSize()));
    }

    void DebugMonitorService::AtmosphereGetRecordSize(sf::Out<u64> record_size) {
        record_size.SetValue(sizeof(ServiceRecord));
    }

}
