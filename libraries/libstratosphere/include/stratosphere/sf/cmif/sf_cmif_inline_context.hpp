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
#include <stratosphere/sf/sf_common.hpp>

namespace ams::sf::cmif {

    using InlineContext = u32;

    InlineContext GetInlineContext();
    InlineContext SetInlineContext(InlineContext ctx);

    class ScopedInlineContextChanger {
        private:
            InlineContext m_prev_ctx;
        public:
            ALWAYS_INLINE explicit ScopedInlineContextChanger(InlineContext new_ctx) : m_prev_ctx(SetInlineContext(new_ctx)) { /* ... */ }
            ~ScopedInlineContextChanger() { SetInlineContext(m_prev_ctx); }
    };

}
