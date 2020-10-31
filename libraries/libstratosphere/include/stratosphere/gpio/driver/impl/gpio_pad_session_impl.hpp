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
#include <vapours.hpp>
#include <stratosphere/gpio/gpio_types.hpp>
#include <stratosphere/gpio/driver/gpio_pad_accessor.hpp>
#include <stratosphere/gpio/driver/impl/gpio_event_holder.hpp>
#include <stratosphere/ddsf.hpp>

namespace ams::gpio::driver {

    class Pad;

}

namespace ams::gpio::driver::impl {

    class PadSessionImpl : public ::ams::ddsf::ISession {
        NON_COPYABLE(PadSessionImpl);
        NON_MOVEABLE(PadSessionImpl);
        AMS_DDSF_CASTABLE_TRAITS(ams::gpio::driver::impl::PadSessionImpl, ::ams::ddsf::ISession);
        private:
            EventHolder event_holder;
        private:
            Result UpdateDriverInterruptEnabled();
        public:
            PadSessionImpl() : event_holder() { /* ... */ }

            ~PadSessionImpl() {
                this->Close();
            }

            bool IsInterruptBound() const {
                return this->event_holder.IsBound();
            }

            Result Open(Pad *pad, ddsf::AccessMode access_mode);
            void Close();

            Result BindInterrupt(os::SystemEventType *event);
            void UnbindInterrupt();

            Result GetInterruptEnabled(bool *out) const;
            Result SetInterruptEnabled(bool en);
            void SignalInterruptBoundEvent();
    };
    static_assert( sizeof(PadSessionImpl) <= GpioPadSessionSize);
    static_assert(alignof(PadSessionImpl) <= GpioPadSessionAlign);

    struct alignas(GpioPadSessionAlign) GpioPadSessionImplPadded {
        PadSessionImpl _impl;
        u8 _padding[GpioPadSessionSize - sizeof(PadSessionImpl)];
    };
    static_assert( sizeof(GpioPadSessionImplPadded) == GpioPadSessionSize);
    static_assert(alignof(GpioPadSessionImplPadded) == GpioPadSessionAlign);

    ALWAYS_INLINE PadSessionImpl &GetPadSessionImpl(GpioPadSession &session) {
        return GetReference(session._impl)._impl;
    }

    ALWAYS_INLINE const PadSessionImpl &GetPadSessionImpl(const GpioPadSession &session) {
        return GetReference(session._impl)._impl;
    }

    ALWAYS_INLINE PadSessionImpl &GetOpenPadSessionImpl(GpioPadSession &session) {
        auto &ref = GetReference(session._impl)._impl;
        AMS_ASSERT(ref.IsOpen());
        return ref;
    }

    ALWAYS_INLINE const PadSessionImpl &GetOpenPadSessionImpl(const GpioPadSession &session) {
        const auto &ref = GetReference(session._impl)._impl;
        AMS_ASSERT(ref.IsOpen());
        return ref;
    }

}
