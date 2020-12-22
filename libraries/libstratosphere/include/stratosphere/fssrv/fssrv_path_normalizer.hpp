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
#pragma once
#include "../fs/fs_common.hpp"

namespace ams::fssrv {

    /* This is in fssrv::detail in official code. */
    /* TODO: Consider moving to ::impl? */

    class PathNormalizer {
        public:
            enum Option : u32 {
                Option_None                  = BIT(0),
                Option_PreserveUnc           = BIT(1),
                Option_PreserveTailSeparator = BIT(2),
                Option_HasMountName          = BIT(3),
                Option_AcceptEmpty           = BIT(4),
            };
        private:
            using Buffer = std::unique_ptr<char[], fs::impl::Deleter>;
        private:
            Buffer buffer;
            const char *path;
            Result result;
        private:
            static Result Normalize(const char **out_path, Buffer *out_buf, const char *path, bool preserve_unc, bool preserve_tail_sep, bool has_mount_name);
        public:
            /* TODO: Remove non-option constructor. */
            explicit PathNormalizer(const char *p) : buffer(), path(nullptr), result(ResultSuccess()) {
                this->result = Normalize(std::addressof(this->path), std::addressof(this->buffer), p, false, false, false);
            }

            PathNormalizer(const char *p, u32 option) : buffer(), path(nullptr), result(ResultSuccess()) {
                if ((option & Option_AcceptEmpty) && p[0] == '\x00') {
                    this->path = path;
                } else {
                    const bool preserve_unc      = (option & Option_PreserveUnc);
                    const bool preserve_tail_sep = (option & Option_PreserveTailSeparator);
                    const bool has_mount_name    = (option & Option_HasMountName);
                    this->result = Normalize(std::addressof(this->path), std::addressof(this->buffer), p, preserve_unc, preserve_tail_sep, has_mount_name);
                }
            }

            Result GetResult() const {
                return this->result;
            }

            const char *GetPath() const {
                return this->path;
            }
    };

}
