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
#include <stratosphere.hpp>
#include "htc_tenv_impl.hpp"
#include "htc_tenv_definition_file_info.hpp"

namespace ams::htc::tenv::impl {

    namespace {

        class DefinitionFileInfoManager {
            private:
                using DefinitionFileInfoList = util::IntrusiveListBaseTraits<DefinitionFileInfo>::ListType;
            private:
                DefinitionFileInfoList m_list;
                os::SdkMutex m_mutex;
            public:
                constexpr DefinitionFileInfoManager() = default;

                ~DefinitionFileInfoManager() {
                    while (!m_list.empty()) {
                        auto *p = std::addressof(*m_list.rbegin());
                        m_list.erase(m_list.iterator_to(*p));
                        delete p;
                    }
                }

                void Remove(DefinitionFileInfo *info) {
                    std::scoped_lock lk(m_mutex);

                    m_list.erase(m_list.iterator_to(*info));
                    delete info;
                }

                DefinitionFileInfo *GetInfo(u64 process_id) {
                    std::scoped_lock lk(m_mutex);

                    for (auto &info : m_list) {
                        if (info.process_id == process_id) {
                            return std::addressof(info);
                        }
                    }

                    return nullptr;
                }
        };

        constinit DefinitionFileInfoManager g_definition_file_info_manager;

        ALWAYS_INLINE DefinitionFileInfoManager &GetDefinitionFileInfoManager() {
            return g_definition_file_info_manager;
        }

    }

    void UnregisterDefinitionFilePath(u64 process_id) {
        /* Require the process id to be valid. */
        if (process_id == 0) {
            return;
        }

        /* Remove the definition file info, if we have one. */
        if (auto *info = GetDefinitionFileInfoManager().GetInfo(process_id); info != nullptr) {
            GetDefinitionFileInfoManager().Remove(info);
        }
    }

}
