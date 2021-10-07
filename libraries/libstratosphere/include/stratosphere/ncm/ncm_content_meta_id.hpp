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
#include <stratosphere/ncm/ncm_data_id.hpp>
#include <stratosphere/ncm/ncm_program_id.hpp>

namespace ams::ncm {

    struct ApplicationId {
        u64 value;

        constexpr operator ProgramId() const {
            return { this->value };
        }

        static const ApplicationId Start;
        static const ApplicationId End;
    };

    constexpr inline const ApplicationId InvalidApplicationId = {};

    inline constexpr const ApplicationId ApplicationId::Start   = { 0x0100000000010000ul };
    inline constexpr const ApplicationId ApplicationId::End     = { 0x01FFFFFFFFFFFFFFul };

    inline constexpr bool IsApplicationId(const ProgramId &program_id) {
        return ApplicationId::Start <= program_id && program_id <= ApplicationId::End;
    }

    inline constexpr bool IsApplicationId(const ApplicationId &) {
        return true;
    }

    struct ApplicationGroupId {
        u64 value;
    };

    struct PatchId {
        u64 value;

        constexpr operator ProgramId() const {
            return { this->value };
        }
    };

    struct PatchGroupId {
        u64 value;
    };

    struct AddOnContentId {
        u64 value;

        constexpr operator DataId() const {
            return { this->value };
        }
    };

    struct DeltaId {
        u64 value;

        constexpr operator ProgramId() const {
            return { this->value };
        }
    };

}
