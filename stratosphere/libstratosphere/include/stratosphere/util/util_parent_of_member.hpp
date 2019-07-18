/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include "../defines.hpp"

namespace sts::util {

    namespace impl {

        template<typename Parent, typename Member>
        union OffsetOfImpl {
            Member Parent::* ptr;
            intptr_t offset;
        };

        template<typename Parent, typename Member>
        constexpr inline Parent *GetParentOfMemberImpl(Member *member, Member Parent::* ptr) {
            return reinterpret_cast<Parent *>(reinterpret_cast<uintptr_t>(member) - OffsetOfImpl<Parent, Member>{ ptr }.offset);
        }

        template<typename Parent, typename Member>
        constexpr inline Parent const *GetParentOfMemberImpl(Member const *member, Member Parent::* ptr) {
            return reinterpret_cast<Parent *>(reinterpret_cast<uintptr_t>(member) - OffsetOfImpl<Parent, Member>{ ptr }.offset);
        }

        template<typename T>
        struct GetMemberPointerTraits;

        template<typename P, typename M>
        struct GetMemberPointerTraits<M P::*> {
            using Parent = P;
            using Member = M;
        };

        template<auto MemberPtr>
        using GetParentType = typename GetMemberPointerTraits<decltype(MemberPtr)>::Parent;

        template<auto MemberPtr>
        using GetMemberType = typename GetMemberPointerTraits<decltype(MemberPtr)>::Member;

    }

    template<auto MemberPtr>
    constexpr inline impl::GetParentType<MemberPtr> *GetParentPointer(impl::GetMemberType<MemberPtr> *member) {
        return impl::GetParentOfMemberImpl<impl::GetParentType<MemberPtr>, impl::GetMemberType<MemberPtr>>(member, MemberPtr);
    }

    template<auto MemberPtr>
    constexpr inline impl::GetParentType<MemberPtr> const *GetParentPointer(impl::GetMemberType<MemberPtr> const *member) {
        return impl::GetParentOfMemberImpl<impl::GetParentType<MemberPtr>, impl::GetMemberType<MemberPtr>>(member, MemberPtr);
    }

    template<auto MemberPtr>
    constexpr inline impl::GetParentType<MemberPtr> &GetParentReference(impl::GetMemberType<MemberPtr> *member) {
        return *impl::GetParentOfMemberImpl<impl::GetParentType<MemberPtr>, impl::GetMemberType<MemberPtr>>(member, MemberPtr);
    }

    template<auto MemberPtr>
    constexpr inline impl::GetParentType<MemberPtr> const &GetParentReference(impl::GetMemberType<MemberPtr> const *member) {
        return *impl::GetParentOfMemberImpl<impl::GetParentType<MemberPtr>, impl::GetMemberType<MemberPtr>>(member, MemberPtr);
    }

}