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
    class DmntService final : public sf::IServiceObject {
        protected:
            /* Command IDs. */
            enum class CommandId {
                AtmosphereGetRecord     = 65000,
                AtmosphereListRecords   = 65001,
                AtmosphereGetRecordSize = 65002,
            };
        private:
            /* Actual commands. */
            virtual Result AtmosphereGetRecord(sf::Out<ServiceRecord> record, ServiceName service);
            virtual void AtmosphereListRecords(const sf::OutArray<ServiceRecord> &records, sf::Out<u64> out_count, u64 offset);
            virtual void AtmosphereGetRecordSize(sf::Out<u64> record_size);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(AtmosphereGetRecord),
                MAKE_SERVICE_COMMAND_META(AtmosphereListRecords),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetRecordSize),
            };
    };

}
