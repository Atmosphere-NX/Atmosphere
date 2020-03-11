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
#include <vapours.hpp>

namespace ams::crypto::impl {

#ifdef ATMOSPHERE_IS_STRATOSPHERE

    void Sha256Impl::Initialize() {
        static_assert(sizeof(this->state) == sizeof(::Sha256Context));
        ::sha256ContextCreate(reinterpret_cast<::Sha256Context *>(std::addressof(this->state)));
    }

    void Sha256Impl::Update(const void *data, size_t size) {
        static_assert(sizeof(this->state) == sizeof(::Sha256Context));
        ::sha256ContextUpdate(reinterpret_cast<::Sha256Context *>(std::addressof(this->state)), data, size);
    }

    void Sha256Impl::GetHash(void *dst, size_t size) {
        static_assert(sizeof(this->state) == sizeof(::Sha256Context));
        AMS_ASSERT(size >= HashSize);
        ::sha256ContextGetHash(reinterpret_cast<::Sha256Context *>(std::addressof(this->state)), dst);
    }

    void Sha256Impl::InitializeWithContext(const Sha256Context *context) {
        static_assert(sizeof(this->state) == sizeof(::Sha256Context));

        /* Copy state in from the context. */
        std::memcpy(this->state.intermediate_hash, context->intermediate_hash, sizeof(this->state.intermediate_hash));
        this->state.bits_consumed = context->bits_consumed;

        /* Clear the rest of state. */
        std::memset(this->state.buffer, 0, sizeof(this->state.buffer));
        this->state.num_buffered = 0;
        this->state.finalized = false;
    }

    size_t Sha256Impl::GetContext(Sha256Context *context) const {
        static_assert(sizeof(this->state) == sizeof(::Sha256Context));
        std::memcpy(context->intermediate_hash, this->state.intermediate_hash, sizeof(context->intermediate_hash));
        context->bits_consumed = this->state.bits_consumed;

        return this->state.num_buffered;
    }

#else

    /* TODO: Non-EL0 implementation. */

#endif

}