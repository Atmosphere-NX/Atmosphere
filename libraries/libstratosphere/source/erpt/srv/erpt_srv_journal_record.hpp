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
#include <stratosphere.hpp>
#include "erpt_srv_allocator.hpp"
#include "erpt_srv_ref_count.hpp"

namespace ams::erpt::srv {

    template<typename Info>
    class JournalRecord : public Allocator, public RefCount, public util::IntrusiveListBaseNode<JournalRecord<Info>> {
        public:
            Info m_info;

            JournalRecord() {
                std::memset(std::addressof(m_info), 0, sizeof(m_info));
            }

            explicit JournalRecord(Info info) : m_info(info) { /* ... */ }

    };

}
