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
#include <exosphere.hpp>

namespace ams::secmon::boot {

    void MakePageTable();

    void InitializeColdBoot();

    bool VerifySignature(void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *msg, size_t msg_size);
    bool VerifyHash(const void *hash, uintptr_t msg, size_t msg_size);

    bool VerifyBootConfigSignature(pkg1::BootConfig &bc, const void *mod, size_t mod_size);
    bool VerifyBootConfigEcid(const pkg1::BootConfig &bc);

}