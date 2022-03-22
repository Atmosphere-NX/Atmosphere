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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util::impl {

    template<bool Copy, bool CopyAssign, bool Move, bool MoveAssign, typename Tag = void>
    struct EnableCopyMove{};

    template<typename Tag>
    struct EnableCopyMove<false, true, true, true, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = delete;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = default;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = default;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = default;
    };

    template<typename Tag>
    struct EnableCopyMove<true, false, true, true, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = default;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = default;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = delete;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = default;
    };

    template<typename Tag>
    struct EnableCopyMove<false, false, true, true, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = delete;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = default;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = delete;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = default;
    };

    template<typename Tag>
    struct EnableCopyMove<true, true, false, true, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = default;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = default;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = default;
    };

    template<typename Tag>
    struct EnableCopyMove<false, true, false, true, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = delete;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = default;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = default;
    };

    template<typename Tag>
    struct EnableCopyMove<true, false, false, true, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = default;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = delete;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = default;
    };

    template<typename Tag>
    struct EnableCopyMove<false, false, false, true, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = delete;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = delete;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = default;
    };

    template<typename Tag>
    struct EnableCopyMove<true, true, true, false, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = default;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = default;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = default;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = delete;
    };

    template<typename Tag>
    struct EnableCopyMove<true, true, false, false, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = default;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = default;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = delete;
    };

    template<typename Tag>
    struct EnableCopyMove<false, true, false, false, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = delete;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = default;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = delete;
    };

    template<typename Tag>
    struct EnableCopyMove<true, false, false, false, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = default;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = delete;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = delete;
    };

    template<typename Tag>
    struct EnableCopyMove<false, false, false, false, Tag> {
        constexpr EnableCopyMove() noexcept                                  = default;

        constexpr EnableCopyMove(const EnableCopyMove &) noexcept            = delete;
        constexpr EnableCopyMove(EnableCopyMove &&) noexcept                 = delete;
        constexpr EnableCopyMove &operator=(const EnableCopyMove &) noexcept = delete;
        constexpr EnableCopyMove &operator=(EnableCopyMove &&) noexcept      = delete;
    };


}
