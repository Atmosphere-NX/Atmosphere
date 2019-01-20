/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include <stratosphere.hpp>
#include "sm_dmnt_service.hpp"
#include "sm_registration.hpp"

Result DmntService::AtmosphereGetRecord(Out<SmServiceRecord> record, SmServiceName service) {
    return Registration::GetServiceRecord(smEncodeName(service.name), record.GetPointer());
}

void DmntService::AtmosphereListRecords(OutBuffer<SmServiceRecord> records, Out<u64> out_count, u64 offset) {
    Registration::ListServiceRecords(offset, records.num_elements, records.buffer, out_count.GetPointer());
}

void DmntService::AtmosphereGetRecordSize(Out<u64> record_size) {
    record_size.SetValue(sizeof(SmServiceRecord));
}

