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
#include <exosphere.hpp>
#include "secmon_boot.hpp"

namespace ams::secmon::boot {

    bool VerifyBootConfigSignature(pkg1::BootConfig &bc, const void *mod, size_t mod_size) {
        return VerifySignature(std::addressof(bc.signature), sizeof(bc.signature), mod, mod_size, std::addressof(bc.signed_data), sizeof(bc.signed_data));
    }

    bool VerifyBootConfigEcid(const pkg1::BootConfig &bc) {
        /* Get the ecid. */
        br::BootEcid ecid;
        fuse::GetEcid(std::addressof(ecid));

        /* Verify it matches. */
        return crypto::IsSameBytes(std::addressof(ecid), bc.signed_data.ecid, sizeof(ecid));
    }

}
