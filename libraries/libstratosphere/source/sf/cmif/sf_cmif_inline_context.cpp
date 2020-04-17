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
#include <stratosphere.hpp>

namespace ams::sf {

    namespace cmif {

        namespace {

            thread_local InlineContext g_inline_context;

            ALWAYS_INLINE void OnSetInlineContext() {
                /* Ensure that libnx receives the priority value. */
                ::fsSetPriority(static_cast<::FsPriority>(::ams::sf::GetFsInlineContext()));
            }

        }

        InlineContext GetInlineContext() {
            InlineContext ctx;
            std::memcpy(std::addressof(ctx), std::addressof(::ams::sf::cmif::g_inline_context), sizeof(ctx));
            return ctx;
        }

        InlineContext SetInlineContext(InlineContext ctx) {
            ON_SCOPE_EXIT { OnSetInlineContext(); };
            static_assert(sizeof(ctx) <= sizeof(g_inline_context));

            InlineContext old_ctx;
            std::memcpy(std::addressof(old_ctx), std::addressof(g_inline_context), sizeof(old_ctx));
            std::memcpy(std::addressof(g_inline_context), std::addressof(ctx), sizeof(ctx));
            return old_ctx;
        }

    }

    u8 GetFsInlineContext() {
        u8 ctx;
        std::memcpy(std::addressof(ctx), std::addressof(cmif::g_inline_context), sizeof(ctx));
        return ctx;
    }

    u8 SetFsInlineContext(u8 ctx) {
        ON_SCOPE_EXIT { cmif::OnSetInlineContext(); };
        static_assert(sizeof(ctx) <= sizeof(cmif::g_inline_context));

        u8 old_ctx;
        std::memcpy(std::addressof(old_ctx), std::addressof(cmif::g_inline_context), sizeof(old_ctx));
        std::memcpy(std::addressof(cmif::g_inline_context), std::addressof(ctx), sizeof(ctx));
        return old_ctx;
    }

}
