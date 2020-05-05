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

namespace ams::secmon {

    enum ImportRsaKey {
        ImportRsaKey_EsDrmCert    = 0,
        ImportRsaKey_Lotus        = 1,
        ImportRsaKey_Ssl          = 2,
        ImportRsaKey_EsClientCert = 3,

        ImportRsaKey_Count        = 4,
    };
    static_assert(util::size(secmon::ConfigurationContext{}.rsa_private_exponents) == ImportRsaKey_Count);

    void ImportRsaKeyExponent(ImportRsaKey which, const void *src, size_t size);
    void ImportRsaKeyModulusProvisionally(ImportRsaKey which, const void *src, size_t size);
    void CommitRsaKeyModulus(ImportRsaKey which);

    bool LoadRsaKey(int slot, ImportRsaKey which);
    void LoadProvisionalRsaKey(int slot, ImportRsaKey which);

    void SetMasterKey(int generation, const void *src, size_t size);
    void LoadMasterKey(int slot, int generation);

    void SetDeviceMasterKey(int generation, const void *src, size_t size);
    void LoadDeviceMasterKey(int slot, int generation);

}