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

namespace ams::fssystem {

    u8 NcaHeader::GetProperKeyGeneration() const {
        return std::max(this->key_generation, this->key_generation_2);
    }

    bool NcaPatchInfo::HasIndirectTable() const {
        return this->indirect_size != 0;
    }

    bool NcaPatchInfo::HasAesCtrExTable() const {
        return this->aes_ctr_ex_size != 0;
    }

}
