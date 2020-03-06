/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include "ncm_content_meta_database_impl.hpp"

namespace ams::ncm {

    class OnMemoryContentMetaDatabaseImpl : public ContentMetaDatabaseImpl {
        public:
            OnMemoryContentMetaDatabaseImpl(ams::kvdb::MemoryKeyValueStore<ContentMetaKey> *kvs) : ContentMetaDatabaseImpl(kvs) { /* ... */ }
        public:
            /* Actual commands. */
            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId id) override;
            virtual Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) override;
            virtual Result Commit() override;
    };

}
