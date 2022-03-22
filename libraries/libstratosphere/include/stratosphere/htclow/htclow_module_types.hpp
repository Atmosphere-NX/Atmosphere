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
#include <vapours.hpp>
#include <stratosphere/htclow/htclow_module_types.hpp>

namespace ams::htclow {

    enum class ModuleId : u8 {
        Unknown = 0,

        Htcfs   = 1,

        Htcmisc = 3,
        Htcs    = 4,
    };

    struct ModuleType {
        bool _is_initialized;
        ModuleId _id;
    };

    constexpr void InitializeModule(ModuleType *out, ModuleId id) {
        *out = {
            ._is_initialized = true,
            ._id             = id,
        };
    }

    constexpr void FinalizeModule(ModuleType *out) {
        *out = {
            ._is_initialized = false,
            ._id             = ModuleId::Unknown,
        };
    }

    class Module final {
        private:
            ModuleType m_impl;
        public:
            constexpr explicit Module(ModuleId id) : m_impl() {
                InitializeModule(std::addressof(m_impl), id);
            }

            constexpr ~Module() {
                FinalizeModule(std::addressof(m_impl));
            }

            ModuleType *GetBase() { return std::addressof(m_impl); }
            const ModuleType *GetBase() const { return std::addressof(m_impl); }
    };

}
