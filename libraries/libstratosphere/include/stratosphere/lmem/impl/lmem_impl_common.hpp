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
#include <vapours.hpp>
#include "../../os.hpp"

namespace ams::lmem::impl {

    /* NOTE: Nintendo does not use util::IntrusiveListNode. */
    /* They seem to manually manage linked list pointers. */
    /* This is pretty gross, so we're going to use util::IntrusiveListNode. */

    struct ExpHeapMemoryBlockHead {
        u16 magic;
        u32 attributes;
        size_t block_size;
        util::IntrusiveListNode list_node;
    };
    static_assert(std::is_trivially_destructible<ExpHeapMemoryBlockHead>::value);

    using ExpHeapMemoryBlockList = typename util::IntrusiveListMemberTraits<&ExpHeapMemoryBlockHead::list_node>::ListType;

    struct ExpHeapHead {
        ExpHeapMemoryBlockList free_list;
        ExpHeapMemoryBlockList used_list;
        u16 group_id;
        u16 mode;
        bool use_alignment_margins;
        char pad[3];
    };
    static_assert(sizeof(ExpHeapHead) == 0x28);
    static_assert(std::is_trivially_destructible<ExpHeapHead>::value);

    struct FrameHeapHead {
        void *next_block_head;
        void *next_block_tail;
    };
    static_assert(sizeof(FrameHeapHead) == 0x10);
    static_assert(std::is_trivially_destructible<FrameHeapHead>::value);

    struct UnitHead {
        UnitHead *next;
    };

    struct UnitHeapList {
        UnitHead *head;
    };

    struct UnitHeapHead {
        UnitHeapList free_list;
        size_t unit_size;
        s32 alignment;
        s32 num_units;
    };
    static_assert(sizeof(UnitHeapHead) == 0x18);
    static_assert(std::is_trivially_destructible<UnitHeapHead>::value);

    union ImplementationHeapHead {
        ExpHeapHead exp_heap_head;
        FrameHeapHead frame_heap_head;
        UnitHeapHead unit_heap_head;
    };

    struct HeapHead {
        u32 magic;
        util::IntrusiveListNode list_node;

        using ChildListTraits = util::IntrusiveListMemberTraitsDeferredAssert<&HeapHead::list_node>;
        using ChildList       = ChildListTraits::ListType;
        ChildList child_list;

        void *heap_start;
        void *heap_end;
        os::MutexType mutex;
        u8 option;
        ImplementationHeapHead impl_head;
    };
    static_assert(std::is_trivially_destructible<HeapHead>::value);
    static_assert(HeapHead::ChildListTraits::IsValid());

}
