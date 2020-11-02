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
#include <stratosphere.hpp>

namespace ams::pwm::driver::impl {

    class ChannelSessionImpl : public ::ams::ddsf::ISession {
        NON_COPYABLE(ChannelSessionImpl);
        NON_MOVEABLE(ChannelSessionImpl);
        AMS_DDSF_CASTABLE_TRAITS(ams::pwm::driver::impl::ChannelSessionImpl, ::ams::ddsf::ISession);
        public:
            ChannelSessionImpl() { /* ... */ }

            ~ChannelSessionImpl() {
                this->Close();
            }

            Result Open(IPwmDevice *device, ddsf::AccessMode access_mode);
            void Close();

            Result SetPeriod(TimeSpan period);
            Result GetPeriod(TimeSpan *out);

            Result SetDuty(int duty);
            Result GetDuty(int *out);

            Result SetEnabled(bool en);
            Result GetEnabled(bool *out);

            Result SetScale(double scale);
            Result GetScale(double *out);
    };
    static_assert( sizeof(ChannelSessionImpl) <= ChannelSessionSize);
    static_assert(alignof(ChannelSessionImpl) <= ChannelSessionAlign);

    struct alignas(ChannelSessionAlign) ChannelSessionImplPadded {
        ChannelSessionImpl _impl;
        u8 _padding[ChannelSessionSize - sizeof(ChannelSessionImpl)];
    };
    static_assert( sizeof(ChannelSessionImplPadded) == ChannelSessionSize);
    static_assert(alignof(ChannelSessionImplPadded) == ChannelSessionAlign);

    ALWAYS_INLINE ChannelSessionImpl &GetChannelSessionImpl(ChannelSession &session) {
        return GetReference(session._impl)._impl;
    }

    ALWAYS_INLINE const ChannelSessionImpl &GetChannelSessionImpl(const ChannelSession &session) {
        return GetReference(session._impl)._impl;
    }

    ALWAYS_INLINE ChannelSessionImpl &GetOpenChannelSessionImpl(ChannelSession &session) {
        auto &ref = GetReference(session._impl)._impl;
        AMS_ASSERT(ref.IsOpen());
        return ref;
    }

    ALWAYS_INLINE const ChannelSessionImpl &GetOpenChannelSessionImpl(const ChannelSession &session) {
        const auto &ref = GetReference(session._impl)._impl;
        AMS_ASSERT(ref.IsOpen());
        return ref;
    }

}
