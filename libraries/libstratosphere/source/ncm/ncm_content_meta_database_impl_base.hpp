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
            using ContentMetaKeyValueStore = ams::kvdb::MemoryKeyValueStore<ContentMetaKey>;
        protected:
            ContentMetaKeyValueStore *kvs;
            char mount_name[fs::MountNameLengthMax + 1];
            bool disabled;
        protected:
            ContentMetaDatabaseImplBase(ContentMetaKeyValueStore *kvs) : kvs(kvs), disabled(false) { /* ... */ }

            ContentMetaDatabaseImplBase(ContentMetaKeyValueStore *kvs, const char *mount_name) : ContentMetaDatabaseImplBase(kvs) {
                std::strcpy(this->mount_name, mount_name);
            }
        protected:
            /* Helpers. */
            Result EnsureEnabled() const {
                R_UNLESS(!this->disabled, ncm::ResultInvalidContentMetaDatabase());
                return ResultSuccess();
            }

            Result GetContentMetaSize(size_t *out, const ContentMetaKey &key) const {
                R_TRY_CATCH(this->kvs->GetValueSize(out, key)) {
                    R_CONVERT(kvdb::ResultKeyNotFound, ncm::ResultContentMetaNotFound())
                } R_END_TRY_CATCH;

                return ResultSuccess();
            }

            Result GetContentMetaPointer(const void **out_value_ptr, size_t *out_size, const ContentMetaKey &key) const {
                R_TRY(this->GetContentMetaSize(out_size, key));
                return this->kvs->GetValuePointer(reinterpret_cast<const ContentMetaHeader **>(out_value_ptr), key);
            }
    };

}
