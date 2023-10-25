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
#include <vapours.hpp>
#include <stratosphere/gc/impl/gc_types.hpp>

namespace ams::gc {

    struct GameCardIdSet {
        gc::impl::CardId1 id1;
        gc::impl::CardId2 id2;
        gc::impl::CardId3 id3;
    };
    static_assert(util::is_pod<GameCardIdSet>::value);
    static_assert(sizeof(GameCardIdSet) == 0xC);

}
