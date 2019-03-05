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
 
#pragma once
#include <switch.h>
#include <stratosphere.hpp>
#include "sm_types.hpp"

enum DmntServiceCmd {
    Dmnt_Cmd_AtmosphereGetRecord = 65000,
    Dmnt_Cmd_AtmosphereListRecords = 65001,
    Dmnt_Cmd_AtmosphereGetRecordSize = 65002,
};

class DmntService final : public IServiceObject {
    private:
        /* Actual commands. */
        virtual Result AtmosphereGetRecord(Out<SmServiceRecord> record, SmServiceName service);
        virtual void AtmosphereListRecords(OutBuffer<SmServiceRecord> records, Out<u64> out_count, u64 offset);
        virtual void AtmosphereGetRecordSize(Out<u64> record_size);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Dmnt_Cmd_AtmosphereGetRecord, &DmntService::AtmosphereGetRecord>(),
            MakeServiceCommandMeta<Dmnt_Cmd_AtmosphereListRecords, &DmntService::AtmosphereListRecords>(),
            MakeServiceCommandMeta<Dmnt_Cmd_AtmosphereGetRecordSize, &DmntService::AtmosphereGetRecordSize>(),
        };
};
