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
#include <mesosphere.hpp>

namespace ams::kern {

    void KPageGroup::Finalize() {
        auto it = this->block_list.begin();
        while (it != this->block_list.end()) {
            KBlockInfo *info = std::addressof(*it);
            it = this->block_list.erase(it);
            this->manager->Free(info);
        }
    }

    size_t KPageGroup::GetNumPages() const {
        size_t num_pages = 0;

        for (const auto &it : *this) {
            num_pages += it.GetNumPages();
        }

        return num_pages;
    }

    Result KPageGroup::AddBlock(KVirtualAddress addr, size_t num_pages) {
        /* Succeed immediately if we're adding no pages. */
        R_SUCCEED_IF(num_pages == 0);

        /* Check for overflow. */
        MESOSPHERE_ASSERT(addr < addr + num_pages * PageSize);

        /* Try to just append to the last block. */
        if (!this->block_list.empty()) {
            auto it = --(this->block_list.end());
            R_SUCCEED_IF(it->TryConcatenate(addr, num_pages));
        }

        /* Allocate a new block. */
        KBlockInfo *new_block = this->manager->Allocate();
        R_UNLESS(new_block != nullptr, svc::ResultOutOfResource());

        /* Initialize the block. */
        new_block->Initialize(addr, num_pages);
        this->block_list.push_back(*new_block);

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
        auto lit  = this->block_list.cbegin();
        auto rit  = rhs.block_list.cbegin();
        auto lend = this->block_list.cend();
        auto rend = rhs.block_list.cend();

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
