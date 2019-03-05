/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>


class SettingsItemManager {
    public:
        static constexpr size_t MaxNameLength = 64; 
        static constexpr size_t MaxKeyLength = 64;
    public:
        static Result ValidateName(const char *name, size_t max_size);
        static Result ValidateName(const char *name);
        
        static Result ValidateKey(const char *key, size_t max_size);
        static Result ValidateKey(const char *key);

        static void LoadConfiguration();
        static Result GetValueSize(const char *name, const char *key, u64 *out_size);
        static Result GetValue(const char *name, const char *key, void *out, size_t max_size, u64 *out_size);
};
