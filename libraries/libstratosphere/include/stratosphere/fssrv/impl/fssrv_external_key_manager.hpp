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
#include <stratosphere/fs/fs_rights_id.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/spl/spl_types.hpp>

namespace ams::fssrv::impl {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class ExternalKeyEntry : public util::IntrusiveListBaseNode<ExternalKeyEntry>, public ::ams::fs::impl::Newable {
        private:
            fs::RightsId m_rights_id;
            spl::AccessKey m_access_key;
        public:
            ExternalKeyEntry(const fs::RightsId &rights_id, const spl::AccessKey &access_key) : m_rights_id(rights_id), m_access_key(access_key) {
                /* ... */
            }

            bool Contains(const fs::RightsId &rights_id) const {
                return crypto::IsSameBytes(std::addressof(m_rights_id), std::addressof(rights_id), sizeof(m_rights_id));
            }

            bool Contains(const void *key, size_t key_size) const {
                AMS_ASSERT(key_size == sizeof(spl::AccessKey));
                AMS_UNUSED(key_size);

                return crypto::IsSameBytes(std::addressof(m_access_key), key, sizeof(m_access_key));
            }

            void CopyAccessKey(spl::AccessKey *out) const {
                AMS_ASSERT(out != nullptr);
                std::memcpy(out, std::addressof(m_access_key), sizeof(m_access_key));
            }
    };

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class ExternalKeyManager {
        NON_COPYABLE(ExternalKeyManager);
        NON_MOVEABLE(ExternalKeyManager);
        private:
            using ExternalKeyList = util::IntrusiveListBaseTraits<ExternalKeyEntry>::ListType;
        private:
            ExternalKeyList m_key_list;
            os::SdkMutex m_mutex;
        public:
            constexpr ExternalKeyManager() : m_key_list(), m_mutex() { /* ... */ }

            Result Register(const fs::RightsId &rights_id, const spl::AccessKey &access_key) {
                /* Acquire exclusive access to the key list */
                std::scoped_lock lk(m_mutex);

                /* Try to find an existing entry. */
                spl::AccessKey existing;
                if (R_SUCCEEDED(this->FindCore(std::addressof(existing), rights_id))) {
                    /* Check the key matches what was previously registered. */
                    R_UNLESS(crypto::IsSameBytes(std::addressof(existing), std::addressof(access_key), sizeof(access_key)), fs::ResultNcaExternalKeyInconsistent());
                } else {
                    /* Make a new entry. */
                    auto *entry = new ExternalKeyEntry(rights_id, access_key);
                    R_UNLESS(entry != nullptr, fs::ResultAllocationMemoryFailed());

                    /* Add the entry to our list. */
                    m_key_list.push_back(*entry);
                }

                R_SUCCEED();
            }

            Result Unregister(const fs::RightsId &rights_id) {
                /* Acquire exclusive access to the key list */
                std::scoped_lock lk(m_mutex);

                /* Find a matching entry. */
                for (auto it = m_key_list.begin(); it != m_key_list.end(); ++it) {
                    if (it->Contains(rights_id)) {
                        auto *entry = std::addressof(*it);
                        m_key_list.erase(it);
                        delete entry;
                        break;
                    }
                }

                /* Always succeed. */
                R_SUCCEED();
            }

            Result UnregisterAll() {
                /* Acquire exclusive access to the key list */
                std::scoped_lock lk(m_mutex);

                /* Remove all entries until our list is empty. */
                while (!m_key_list.empty()) {
                    auto *entry = std::addressof(*m_key_list.begin());
                    m_key_list.erase(m_key_list.iterator_to(*entry));
                    delete entry;
                }

                R_SUCCEED();
            }

            bool IsAvailableAccessKey(const void *key, size_t key_size) {
                /* Acquire exclusive access to the key list */
                std::scoped_lock lk(m_mutex);

                /* Check if any entry contains the key. */
                for (const auto &entry : m_key_list) {
                    if (entry.Contains(key, key_size)) {
                        return true;
                    }
                }

                return false;
            }

            Result Find(spl::AccessKey *out, const fs::RightsId &rights_id) {
                /* Acquire exclusive access to the key list */
                std::scoped_lock lk(m_mutex);

                /* Try to find an entry with the desired rights id. */
                R_RETURN(this->FindCore(out, rights_id));
            }
        private:
            Result FindCore(spl::AccessKey *out, const fs::RightsId &rights_id) {
                for (const auto &entry : m_key_list) {
                    if (entry.Contains(rights_id)) {
                        entry.CopyAccessKey(out);
                        R_SUCCEED();
                    }
                }

                R_THROW(fs::ResultNcaExternalKeyUnregistered());
            }
    };

}
