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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams {

    namespace impl {

        using DeviceCodeType = u32;

    }

    /* TODO: Better understand device code components. */
    class DeviceCode {
        private:
            impl::DeviceCodeType inner_value;
        public:
            constexpr DeviceCode(impl::DeviceCodeType v) : inner_value(v) { /* ... */ }

            constexpr impl::DeviceCodeType GetInternalValue() const { return this->inner_value; }

            constexpr bool operator==(const DeviceCode &rhs) const {
                return this->GetInternalValue() == rhs.GetInternalValue();
            }

            constexpr bool operator!=(const DeviceCode &rhs) const {
                return !(*this == rhs);
            }
    };

    constexpr inline const DeviceCode InvalidDeviceCode(0);

}
