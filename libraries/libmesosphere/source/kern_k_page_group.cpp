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
#include <mesosphere.hpp>

namespace ams::kern {

    void KPageGroup::Finalize() {
        KBlockInfo *cur = m_first_block;
        while (cur != nullptr) {
            KBlockInfo *next = cur->GetNext();
            m_manager->Free(cur);
            cur = next;
        }

        m_first_block = nullptr;
        m_last_block  = nullptr;
    }

    void KPageGroup::CloseAndReset() {
        auto &mm = Kernel::GetMemoryManager();

        KBlockInfo *cur = m_first_block;
        while (cur != nullptr) {
            KBlockInfo *next = cur->GetNext();
            mm.Close(cur->GetAddress(), cur->GetNumPages());
            m_manager->Free(cur);
            cur = next;
        }

        m_first_block = nullptr;
        m_last_block  = nullptr;
    }

    size_t KPageGroup::GetNumPages() const {
        size_t num_pages = 0;

        for (const auto &it : *this) {
            num_pages += it.GetNumPages();
        }

        return num_pages;
    }

    Result KPageGroup::AddBlock(KPhysicalAddress addr, size_t num_pages) {
        /* Succeed immediately if we're adding no pages. */
        R_SUCCEED_IF(num_pages == 0);

        /* Check for overflow. */
        MESOSPHERE_ASSERT(addr < addr + num_pages * PageSize);

        /* Try to just append to the last block. */
        if (m_last_block != nullptr) {
            R_SUCCEED_IF(m_last_block->TryConcatenate(addr, num_pages));
        }

        /* Allocate a new block. */
        KBlockInfo *new_block = m_manager->Allocate();
        R_UNLESS(new_block != nullptr, svc::ResultOutOfResource());

        /* Initialize the block. */
        new_block->Initialize(addr, num_pages);

        /* Add the block to our list. */
        if (m_last_block != nullptr) {
            m_last_block->SetNext(new_block);
        } else {
            m_first_block = new_block;
        }
        m_last_block = new_block;

        R_SUCCEED();
    }

    Result KPageGroup::CopyRangeTo(KPageGroup &out, size_t range_offset, size_t range_size) const {
        /* Get the previous last block for the group. */
        KBlockInfo * const out_last = out.m_last_block;
        const auto out_last_addr = out_last != nullptr ? out_last->GetAddress()  : Null<KPhysicalAddress>;
        const auto out_last_np   = out_last != nullptr ? out_last->GetNumPages() : 0;

        /* Ensure we cleanup the group on failure. */
        ON_RESULT_FAILURE {
            KBlockInfo *cur = out_last != nullptr ? out_last->GetNext() : out.m_first_block;
            while (cur != nullptr) {
                KBlockInfo *next = cur->GetNext();
                out.m_manager->Free(cur);
                cur = next;
            }

            if (out_last != nullptr) {
                out_last->Initialize(out_last_addr, out_last_np);
                out_last->SetNext(nullptr);
            } else {
                out.m_first_block = nullptr;
            }
            out.m_last_block = out_last;
        };

        /* Find the pages within the requested range. */
        size_t cur_offset = 0, remaining_size = range_size;
        for (auto it = this->begin(); it != this->end() && remaining_size > 0; ++it) {
            /* Get the current size. */
            const size_t cur_size = it->GetSize();

            /* Determine if the offset is in range. */
            const size_t rel_diff = range_offset - cur_offset;
            const bool is_before  = cur_offset <= range_offset;
            cur_offset += cur_size;
            if (is_before && range_offset < cur_offset) {
                /* It is, so add the block. */
                const size_t block_size = std::min<size_t>(cur_size - rel_diff, remaining_size);
                R_TRY(out.AddBlock(it->GetAddress() + rel_diff, block_size / PageSize));

                /* Advance. */
                cur_offset      = range_offset + block_size;
                remaining_size -= block_size;
                range_offset   += block_size;
            }
        }

        /* Check that we successfully copied the range. */
        MESOSPHERE_ABORT_UNLESS(remaining_size == 0);

        R_SUCCEED();
    }

    void KPageGroup::Open() const {
        auto &mm = Kernel::GetMemoryManager();

        for (const auto &it : *this) {
            mm.Open(it.GetAddress(), it.GetNumPages());
        }
    }

    void KPageGroup::OpenFirst() const {
        auto &mm = Kernel::GetMemoryManager();

        for (const auto &it : *this) {
            mm.OpenFirst(it.GetAddress(), it.GetNumPages());
        }
    }

    void KPageGroup::Close() const {
        auto &mm = Kernel::GetMemoryManager();

        for (const auto &it : *this) {
            mm.Close(it.GetAddress(), it.GetNumPages());
        }
    }

    bool KPageGroup::IsEquivalentTo(const KPageGroup &rhs) const {
        auto lit  = this->begin();
        auto rit  = rhs.begin();
        auto lend = this->end();
        auto rend = rhs.end();

        while (lit != lend && rit != rend) {
            if (*lit != *rit) {
                return false;
            }

            ++lit;
            ++rit;
        }

        return lit == lend && rit == rend;
    }

}
