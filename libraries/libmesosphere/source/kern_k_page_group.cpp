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

        return ResultSuccess();
    }

    void KPageGroup::Open() const {
        auto &mm = Kernel::GetMemoryManager();

        for (const auto &it : *this) {
            mm.Open(it.GetAddress(), it.GetNumPages());
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
