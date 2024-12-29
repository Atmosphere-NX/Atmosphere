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
#pragma once
#include <stratosphere.hpp>

namespace ams::ldr {

    struct Meta {
        Npdm *npdm;
        Acid *acid;
        Aci *aci;

        void *acid_fac;
        void *acid_sac;
        void *acid_kac;

        void *aci_fah;
        void *aci_sac;
        void *aci_kac;

        void *modulus;
        bool check_verification_data;
    };

    /* Meta API. */
    Result LoadMeta(Meta *out_meta, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status, ncm::ContentMetaPlatform platform, bool unk_unused);
    Result LoadMetaFromCache(Meta *out_meta, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status, ncm::ContentMetaPlatform platform);
    void   InvalidateMetaCache();

}
