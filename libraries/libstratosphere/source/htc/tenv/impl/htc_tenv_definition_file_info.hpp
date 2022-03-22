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
#include "htc_tenv_allocator.hpp"

namespace ams::htc::tenv::impl {

    struct DefinitionFileInfo : public util::IntrusiveListBaseNode<DefinitionFileInfo> {
        u64 process_id;
        Path path;

        DefinitionFileInfo(u64 pid, Path *p) : process_id(pid) {
            AMS_ASSERT(p != nullptr);
            util::Strlcpy(this->path.str, p->str, PathLengthMax);
        }

        static void *operator new(size_t size) {
            return Allocate(size);
        }

        static void operator delete(void *p, size_t size) {
            Deallocate(p, size);
        }
    };

}
