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

    class ContentStorageImplBase : public IContentStorage {
        NON_COPYABLE(ContentStorageImplBase);
        NON_MOVEABLE(ContentStorageImplBase);
        protected:
            PathString root_path;
            MakeContentPathFunction make_content_path_func;
            bool disabled;
        protected:
            ContentStorageImplBase() { /* ... */ }
        protected:
            /* Helpers. */
            Result EnsureEnabled() const {
                R_UNLESS(!this->disabled, ncm::ResultInvalidContentStorage());
                return ResultSuccess();
            }

            static Result GetRightsId(ncm::RightsId *out_rights_id, const Path &path) {
                if (hos::GetVersion() >= hos::Version_3_0_0) {
                    R_TRY(fs::GetRightsId(std::addressof(out_rights_id->id), std::addressof(out_rights_id->key_generation), path.str));
                } else {
                    R_TRY(fs::GetRightsId(std::addressof(out_rights_id->id), path.str));
                    out_rights_id->key_generation = 0;
                }
                return ResultSuccess();
            }
    };

}
