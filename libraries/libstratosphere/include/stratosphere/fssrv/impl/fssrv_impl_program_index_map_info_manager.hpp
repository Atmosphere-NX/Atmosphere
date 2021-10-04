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
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fs_program_index_map_info.hpp>

namespace ams::fssrv::impl {

    struct ProgramIndexMapInfoEntry : public ::ams::util::IntrusiveListBaseNode<ProgramIndexMapInfoEntry>, public ::ams::fs::impl::Newable {
        ncm::ProgramId program_id;
        ncm::ProgramId base_program_id;
        u8 program_index;
    };

    class ProgramIndexMapInfoManager {
        NON_COPYABLE(ProgramIndexMapInfoManager);
        NON_MOVEABLE(ProgramIndexMapInfoManager);
        private:
            using ProgramIndexMapInfoList = util::IntrusiveListBaseTraits<ProgramIndexMapInfoEntry>::ListType;
        private:
            ProgramIndexMapInfoList m_list;
            mutable os::SdkMutex m_mutex;
        public:
            constexpr ProgramIndexMapInfoManager() : m_list(), m_mutex() { /* ... */ }

            void Clear() {
                /* Acquire exclusive access to the map. */
                std::scoped_lock lk(m_mutex);

                /* Actually clear. */
                this->ClearImpl();
            }

            size_t GetProgramCount() const {
                /* Acquire exclusive access to the map. */
                std::scoped_lock lk(m_mutex);

                /* Get the size. */
                return m_list.size();
            }

            util::optional<fs::ProgramIndexMapInfo> Get(const ncm::ProgramId &program_id) const {
                /* Acquire exclusive access to the map. */
                std::scoped_lock lk(m_mutex);

                /* Get the entry from the map. */
                return this->GetImpl([&] (const ProgramIndexMapInfoEntry &entry) {
                    return entry.program_id == program_id;
                });
            }

            ncm::ProgramId GetProgramId(const ncm::ProgramId &program_id, u8 program_index) const {
                /* Acquire exclusive access to the map. */
                std::scoped_lock lk(m_mutex);

                /* Get the program info for the desired program id. */
                const auto base_info = this->GetImpl([&] (const ProgramIndexMapInfoEntry &entry) {
                    return entry.program_id == program_id;
                });

                /* Check that an entry exists for the program id. */
                if (!base_info.has_value()) {
                    return ncm::InvalidProgramId;
                }

                /* Get a program info which matches the same base program with the desired index. */
                const auto target_info = this->GetImpl([&] (const ProgramIndexMapInfoEntry &entry) {
                    return entry.base_program_id == base_info->base_program_id && entry.program_index == program_index;
                });

                /* Return the desired program id. */
                if (target_info.has_value()) {
                    return target_info->program_id;
                } else {
                    return ncm::InvalidProgramId;
                }
            }

            Result Reset(const fs::ProgramIndexMapInfo *infos, int count) {
                /* Acquire exclusive access to the map. */
                std::scoped_lock lk(m_mutex);

                /* Clear the map, and ensure we remain clear if we fail after this point. */
                this->ClearImpl();
                auto clear_guard = SCOPE_GUARD { this->ClearImpl(); };

                /* Add each info to the list. */
                for (int i = 0; i < count; ++i) {
                    /* Allocate new entry. */
                    auto *entry = new ProgramIndexMapInfoEntry;
                    R_UNLESS(entry != nullptr, fs::ResultAllocationFailureInNew());

                    /* Copy over the info. */
                    entry->program_id      = infos[i].program_id;
                    entry->base_program_id = infos[i].base_program_id;
                    entry->program_index   = infos[i].program_index;

                    /* Add to the list. */
                    m_list.push_back(*entry);
                }

                /* We successfully imported the map. */
                clear_guard.Cancel();
                return ResultSuccess();
            }
        private:
            void ClearImpl() {
                /* Delete all entries. */
                while (!m_list.empty()) {
                    /* Get the first entry. */
                    ProgramIndexMapInfoEntry *front = std::addressof(*m_list.begin());

                    /* Erase it from the list. */
                    m_list.erase(m_list.iterator_to(*front));

                    /* Delete the entry. */
                    delete front;
                }
            }

            template<typename F>
            util::optional<fs::ProgramIndexMapInfo> GetImpl(F f) const {
                /* Try to find an entry matching the predicate. */
                util::optional<fs::ProgramIndexMapInfo> match = util::nullopt;

                for (const auto &entry : m_list) {
                    /* If the predicate matches, we want to return the relevant info. */
                    if (f(entry)) {
                        match.emplace();

                        match->program_id      = entry.program_id;
                        match->base_program_id = entry.base_program_id;
                        match->program_index   = entry.program_index;

                        break;
                    }
                }

                return match;
            }
    };


}
