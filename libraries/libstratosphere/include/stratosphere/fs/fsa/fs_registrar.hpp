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
#include "../fs_common.hpp"

namespace ams::fs::fsa {

    class ICommonMountNameGenerator {
        public:
            virtual ~ICommonMountNameGenerator() { /* ... */ }
            virtual Result GenerateCommonMountName(char *name, size_t name_size) = 0;
    };

    class IFileSystem;

    Result Register(const char *name, std::unique_ptr<IFileSystem> &&fs);
    Result Register(const char *name, std::unique_ptr<IFileSystem> &&fs, std::unique_ptr<ICommonMountNameGenerator> &&generator);
    Result Register(const char *name, std::unique_ptr<IFileSystem> &&fs, std::unique_ptr<ICommonMountNameGenerator> &&generator, bool use_data_cache, bool use_path_cache, bool multi_commit_supported);

    void Unregister(const char *name);
}
