/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>

#include "ro_nrr.hpp"
#include "ro_registration.hpp"

Result NrrUtils::ValidateNrrSignature(const NrrHeader *header) {
    /* TODO: Implement RSA-2048 PSS..... */
    
    /* TODO: Check PSS fixed-key signature. */
    if (false) {
        return ResultRoNotAuthorized;
    }
    
    /* Check TitleID pattern is valid. */
    if ((header->title_id & header->title_id_mask) != header->title_id_pattern) {
        return ResultRoNotAuthorized;
    }
    
    /* TODO: Check PSS signature over hashes. */
    if (false) {
        return ResultRoNotAuthorized;
    }
    
    return ResultSuccess;
}

Result NrrUtils::ValidateNrr(const NrrHeader *header, u64 size, u64 title_id, RoModuleType expected_type, bool enforce_type) {
    if (header->magic != MagicNrr0) {
        return ResultRoInvalidNrr;
    }
    if (header->nrr_size != size) {
        return ResultRoInvalidSize;
    }
    
    bool ease_nro_restriction = Registration::ShouldEaseNroRestriction();
    
    /* Check signature. */
    Result rc = ValidateNrrSignature(header);
    if (R_FAILED(rc)) {
        if (!ease_nro_restriction) {
            return rc;
        }
    }
    
    /* Check title id. */
    if (title_id != header->title_id) {
        if (!ease_nro_restriction) {
            return ResultRoInvalidNrr;
        }
    }
    
    /* Check type. */
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
        if (!enforce_type || expected_type != static_cast<RoModuleType>(header->nrr_type)) {
            if (!ease_nro_restriction) {
                return ResultRoInvalidNrrType;
            }
        }
    }
    
    return ResultSuccess;
}