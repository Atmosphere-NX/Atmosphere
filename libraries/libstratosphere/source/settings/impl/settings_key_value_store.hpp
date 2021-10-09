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
#include <stratosphere.hpp>
#include "settings_system_save_data.hpp"

namespace ams::settings::impl {

    struct KeyValueStoreItemForDebug {
        const char *key;
        u8 type;
        size_t current_value_size;
        size_t default_value_size;
        void *current_value;
        void *default_value;
    };
    static_assert(sizeof(KeyValueStoreItemForDebug) == 0x30);

    struct KeyValueStoreKeyIterator {
        size_t header_size;
        size_t entire_size;
        char *map_key;
    };
    static_assert(sizeof(KeyValueStoreKeyIterator) == 0x18);

    class KeyValueStore {
        private:
            const SettingsName &m_name;
        public:
            explicit KeyValueStore(const SettingsName &name) : m_name(name) { /* ... */ }

            Result CreateKeyIterator(KeyValueStoreKeyIterator *out);
            Result GetValue(u64 *out_count, char *out_buffer, size_t out_buffer_size, const SettingsItemKey &item_key);
            Result GetValueSize(u64 *out_value_size, const SettingsItemKey &item_key);
            Result ResetValue(const SettingsItemKey &item_key);
            Result SetValue(const SettingsItemKey &item_key, const void *buffer, size_t buffer_size);
    };

    Result AddKeyValueStoreItemForDebug(const KeyValueStoreItemForDebug * const items, size_t items_count);
    Result AdvanceKeyValueStoreKeyIterator(KeyValueStoreKeyIterator *out);
    Result DestroyKeyValueStoreKeyIterator(KeyValueStoreKeyIterator *out);
    Result GetKeyValueStoreItemCountForDebug(u64 *out_count);
    Result GetKeyValueStoreItemForDebug(u64 *out_count, KeyValueStoreItemForDebug * const out_items, size_t out_items_count);
    Result GetKeyValueStoreKeyIteratorKey(u64 *out_count, char *out_buffer, size_t out_buffer_size, const KeyValueStoreKeyIterator &iterator);
    Result GetKeyValueStoreKeyIteratorKeySize(u64 *out_count, const KeyValueStoreKeyIterator &iterator);
    Result ReadKeyValueStoreFirmwareDebug(u64 *out_count, char * const out_buffer, size_t out_buffer_size);
    Result ReadKeyValueStorePlatformConfiguration(u64 *out_count, char * const out_buffer, size_t out_buffer_size);
    Result ReadKeyValueStoreSaveData(u64 *out_count, char * const out_buffer, size_t out_buffer_size);
    Result ReloadKeyValueStoreForDebug(SystemSaveData *system_save_data, SystemSaveData *fwdbg_system_data, SystemSaveData *pfcfg_system_data);
    Result ReloadKeyValueStoreForDebug();
    Result ResetKeyValueStoreSaveData();
    Result SaveKeyValueStoreAllForDebug(SystemSaveData *data);

}
