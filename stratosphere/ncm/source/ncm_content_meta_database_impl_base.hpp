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

namespace ams::ncm {

    class ContentMetaDatabaseImplBase : public IContentMetaDatabase {
        NON_COPYABLE(ContentMetaDatabaseImplBase);
        NON_MOVEABLE(ContentMetaDatabaseImplBase);
        protected:
            ams::kvdb::MemoryKeyValueStore<ContentMetaKey> *kvs;
            char mount_name[16];
            bool disabled;
        protected:
            ContentMetaDatabaseImplBase(ams::kvdb::MemoryKeyValueStore<ContentMetaKey> *kvs) :
                kvs(kvs), disabled(false)
            {
                /* ... */
            }

            ContentMetaDatabaseImplBase(ams::kvdb::MemoryKeyValueStore<ContentMetaKey> *kvs, const char *mount_name) :
                ContentMetaDatabaseImplBase(kvs)
            {
                std::strcpy(this->mount_name, mount_name);
            }
        protected:
            Result EnsureEnabled() {
                R_UNLESS(!this->disabled, ncm::ResultInvalidContentMetaDatabase());
                return ResultSuccess();
            }
    };

}