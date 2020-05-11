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
#include "ldr_launch_record.hpp"

namespace ams::ldr {

    namespace {

        bool g_development_for_acid_production_check = false;
        bool g_development_for_anti_downgrade_check  = false;
        bool g_development_for_acid_signature_check  = false;
        bool g_enabled_program_verification          = true;

    }

    void SetDevelopmentForAcidProductionCheck(bool development) {
        g_development_for_acid_production_check = development;
    }

    void SetDevelopmentForAntiDowngradeCheck(bool development) {
        g_development_for_anti_downgrade_check = development;
    }

    void SetDevelopmentForAcidSignatureCheck(bool development) {
        g_development_for_acid_signature_check = development;
    }

    void SetEnabledProgramVerification(bool enabled) {
        if (g_development_for_acid_signature_check) {
            g_enabled_program_verification = enabled;
        }
    }

    bool IsDevelopmentForAcidProductionCheck() {
        return g_development_for_acid_production_check;
    }

    bool IsDevelopmentForAntiDowngradeCheck() {
        return g_development_for_anti_downgrade_check;
    }

    bool IsDevelopmentForAcidSignatureCheck() {
        return g_development_for_acid_signature_check;
    }

    bool IsEnabledProgramVerification() {
        return g_enabled_program_verification;
    }

}
