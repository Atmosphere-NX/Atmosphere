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
#include <stratosphere/ncm/ncm_content_meta.hpp>
#include <stratosphere/ncm/ncm_content_meta_database.hpp>
#include <stratosphere/ncm/ncm_content_storage.hpp>

namespace ams::ncm {

    class ContentMetaDatabaseBuilder {
        private:
            ContentMetaDatabase *db;
        private:
            Result BuildFromPackageContentMeta(void *buf, size_t size, const ContentInfo &meta_info);
        public:
            explicit ContentMetaDatabaseBuilder(ContentMetaDatabase *d) : db(d) { /* ... */ }

            Result BuildFromStorage(ContentStorage *storage);

            Result Cleanup();
    };

}
