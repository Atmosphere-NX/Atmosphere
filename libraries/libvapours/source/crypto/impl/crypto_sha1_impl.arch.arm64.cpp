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
#include <vapours.hpp>

namespace ams::crypto::impl {

#ifdef ATMOSPHERE_IS_STRATOSPHERE

    void Sha1Impl::Initialize() {
        static_assert(sizeof(m_state) == sizeof(::Sha1Context));
        ::sha1ContextCreate(reinterpret_cast<::Sha1Context *>(std::addressof(m_state)));
    }

    void Sha1Impl::Update(const void *data, size_t size) {
        static_assert(sizeof(m_state) == sizeof(::Sha1Context));
        ::sha1ContextUpdate(reinterpret_cast<::Sha1Context *>(std::addressof(m_state)), data, size);
    }

    void Sha1Impl::GetHash(void *dst, size_t size) {
        static_assert(sizeof(m_state) == sizeof(::Sha1Context));
        AMS_ASSERT(size >= HashSize);
        AMS_UNUSED(size);
        ::sha1ContextGetHash(reinterpret_cast<::Sha1Context *>(std::addressof(m_state)), dst);
    }

#else

    /* TODO: Non-EL0 implementation. */

#endif

}