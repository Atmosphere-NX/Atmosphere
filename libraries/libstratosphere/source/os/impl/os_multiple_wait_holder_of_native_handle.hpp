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
#include "os_multiple_wait_holder_base.hpp"

namespace ams::os::impl {

    class MultiWaitHolderOfNativeHandle : public MultiWaitHolderOfNativeWaitObject {
        private:
            NativeHandle m_handle;
        public:
            explicit MultiWaitHolderOfNativeHandle(NativeHandle h) : m_handle(h) { /* ... */ }

            /* IsSignaled, GetHandle both implemented. */
            virtual TriBool IsSignaled() const override {
                return TriBool::Undefined;
            }

            virtual bool GetNativeHandle(os::NativeHandle *out) const override {
                *out = m_handle;
                return true;
            }
    };

}
