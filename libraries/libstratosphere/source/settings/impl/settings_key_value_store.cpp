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
#include <stratosphere.hpp>
#include "settings_key_value_store.hpp"
#include "settings_spl.hpp"
#include "settings_system_data.hpp"

namespace ams::settings::impl {

    namespace {
        constexpr fs::SystemSaveDataId SystemSaveDataId          = 0x8000000000000051;
        constexpr                  u32 SystemSaveDataFlags       = fs::SaveDataFlags_KeepAfterRefurbishment | fs::SaveDataFlags_KeepAfterResettingSystemSaveData;
        constexpr                  s64 SystemSaveDataSize        = 544_KB;
        constexpr                  s64 SystemSaveDataJournalSize = 544_KB;

        constexpr inline const char FwdbgSystemDataMountName[]     = "FwdbgSettingsD";
        constexpr inline const char PfCfgSystemDataMountName[]     = "PfcfgSettingsD";
        constexpr inline const char SystemSaveDataMountName[]      = "FwdbgSettingsS";

        constexpr inline const char SettingsNameSeparator = '!';

        /* Type forward declarations. */
        class MapKey;
        struct MapValue;
        template<typename T>
        class Allocator;
        using Map = std::map<MapKey, MapValue, std::less<MapKey>, Allocator<std::pair<const MapKey, MapValue>>>;

        /* Function forward declarations. */
        void FreeMapValueToHeap(const MapValue &value);
        void *AllocateFromHeap(size_t size);
        void FreeToHeap(void *block, size_t size);
        lmem::HeapHandle &GetHeapHandle();
        Result GetKeyValueStoreMap(Map **out);
        Result GetKeyValueStoreMap(Map **out, bool force_load);
        Result GetKeyValueStoreMapForciblyForDebug(Map **out);
        Result LoadKeyValueStoreMap(Map *out);
        Result LoadKeyValueStoreMap(Map *out, SplHardwareType hardware_type);
        template<typename T>
        Result LoadKeyValueStoreMapCurrent(Map *out, T &data);
        template<typename T>
        Result LoadKeyValueStoreMapDefault(Map *out, T &data);
        template<typename T, typename F>
        Result LoadKeyValueStoreMapEntries(Map *out, T &data, F load);
        template<typename T, typename F>
        Result LoadKeyValueStoreMapEntry(Map *out, T &data, s64 &offset, F load);
        Result LoadKeyValueStoreMapForDebug(Map *out, SystemSaveData *system_save_data, SystemSaveData *fwdbg_system_data, SystemSaveData *pfcfg_system_data);
        template<typename T>
        Result ReadData(T &data, s64 &offset, void *buffer, size_t size);
        template<typename T>
        Result ReadDataToHeap(T &data, s64 &offset, void **buffer, size_t size);
        template<typename T>
        Result ReadAllBytes(T *data, u64 *out_count, char * const out_buffer, size_t out_buffer_size);
        template<typename T>
        Result SaveKeyValueStoreMapCurrent(T &data, const Map &map);

        struct SystemDataTag {
            struct Fwdbg{};
            struct PfCfg{};
        };

        class MapKey {
            public:
                static constexpr size_t MaxKeySize = sizeof(SettingsName) + sizeof(SettingsItemKey);
            private:
                char *m_chars;
                size_t m_count;
            public:
                MapKey(const char * const chars) : m_chars(nullptr), m_count(0) {
                    AMS_ASSERT(chars != nullptr);
                    this->Assign(chars, util::Strnlen(chars, MaxKeySize));
                }

                MapKey(const char * const chars, s32 count) : m_chars(nullptr), m_count(0) {
                    AMS_ASSERT(chars != nullptr);
                    AMS_ASSERT(count >= 0);
                    this->Assign(chars, count);
                }

                MapKey(MapKey &&other) : m_chars(nullptr), m_count(0) {
                    std::swap(m_chars, other.m_chars);
                    std::swap(m_count, other.m_count);
                }

                MapKey(const MapKey &other) : m_chars(nullptr), m_count(0) {
                    this->Assign(other.GetString(), other.GetCount());
                }

                ~MapKey() {
                    this->Reset();
                }

                MapKey &Append(char c) {
                    const char chars[2] = { c, 0 };
                    return this->Append(chars, 1);
                }

                MapKey &Append(const char * const chars, s32 count) {
                    AMS_ASSERT(chars != nullptr);
                    AMS_ASSERT(count >= 0);

                    /* Allocate the new key. */
                    const size_t new_count = m_count + count;
                    char *new_heap = static_cast<char *>(AllocateFromHeap(new_count));

                    /* Copy the existing string to the new heap. */
                    std::memcpy(new_heap, this->GetString(), this->GetCount());

                    /* Copy the string to append to the new heap. */
                    std::memcpy(new_heap + this->GetCount(), chars, count);

                    /* Null-terminate the new string. */
                    new_heap[new_count - 1] = '\x00';

                    /* Reset and update the key. */
                    this->Reset();
                    m_count = new_count;
                    m_chars = new_heap;
                    return *this;
                }

                MapKey &Assign(const char * const chars, s32 count) {
                    AMS_ASSERT(chars != nullptr);
                    AMS_ASSERT(count >= 0);

                    /* Reset the key. */
                    this->Reset();

                    /* Update the count and allocate the buffer. */
                    m_count = count + 1;
                    m_chars = static_cast<char *>(AllocateFromHeap(m_count));

                    /* Copy the characters to the buffer. */
                    std::memcpy(m_chars, chars, count);
                    m_chars[count] = '\x00';
                    return *this;
                }

                size_t Find(const MapKey &other) const {
                    return std::search(this->GetString(), this->GetString() + this->GetCount(), other.GetString(), other.GetString() + other.GetCount()) - this->GetString();
                }

                s32 GetCount() const {
                    return static_cast<s32>(m_count) - 1;
                }

                const char *GetString() const {
                    return m_chars;
                }
            private:
                void Reset() {
                    if (m_chars != nullptr) {
                        FreeToHeap(m_chars, m_count);
                        m_chars = nullptr;
                        m_count = 0;
                    }
                }
        };

        inline bool operator<(const MapKey &lhs, const MapKey &rhs) {
            return std::strncmp(lhs.GetString(), rhs.GetString(), std::max(lhs.GetCount(), rhs.GetCount())) < 0;
        }

        MapKey MakeMapKey(const SettingsName &name, const SettingsItemKey &item_key) {
            /* Create a map key. */
            MapKey key(name.value, util::Strnlen(name.value, util::size(name.value)));

            /* Append the settings name separator followed by the item key. */
            key.Append(SettingsNameSeparator);
            key.Append(item_key.value, util::Strnlen(item_key.value, util::size(item_key.value)));

            /* Output the map key. */
            return key;
        }

        struct MapValue {
            public:
                u8 type;
                size_t current_value_size;
                size_t default_value_size;
                void *current_value;
                void *default_value;
        };
        static_assert(sizeof(MapValue) == 0x28);

        template<typename T>
        class Allocator {
            public:
                using value_type      = T;
                using size_type       = size_t;
                using difference_type = ptrdiff_t;
            public:
                Allocator() noexcept = default;
                ~Allocator() noexcept = default;

                Allocator(const Allocator &) noexcept = default;
                Allocator(Allocator &&) noexcept = default;

                T *allocate(size_t n) noexcept {
                    return static_cast<T *>(AllocateFromHeap(sizeof(T) * n));
                }

                void deallocate(T *p, size_t n) noexcept {
                    FreeToHeap(p, sizeof(T) * n);
                }
            private:
                Allocator &operator=(const Allocator &) noexcept = default;
                Allocator &operator=(Allocator &&) noexcept = default;
        };

        template<class T, class U>
        constexpr inline bool operator==(const Allocator<T> &, const Allocator<U> &) {
            return true;
        }

        constexpr inline size_t MapKeyBufferSize   = MapKey::MaxKeySize * 2;
        constexpr inline size_t MapEntryBufferSize = 0x40 + sizeof(Map::value_type);

        constexpr inline size_t HeapMemorySize = 512_KB;

        constinit os::SdkMutex g_key_value_store_mutex;

        void ClearKeyValueStoreMap(Map &map) {
            /* Free all values to the heap. */
            for (const auto &kv_pair : map) {
                FreeMapValueToHeap(kv_pair.second);
            }

            /* Clear the map. */
            map.clear();
        }

        bool CompareValue(const void *lhs, size_t lhs_size, const void *rhs, size_t rhs_size) {
            /* Check if both buffers are the same. */
            if (lhs == rhs) {
                return true;
            }

            /* If the value sizes don't match, return false. */
            if (lhs_size != rhs_size) {
                return false;
            }

            /* If the value sizes are 0, they are considered to match. */
            if (lhs_size == 0) {
                return true;
            }

            /* Compare the two values if they are non-null. */
            return lhs != nullptr && rhs != nullptr && std::memcmp(lhs, rhs, lhs_size) == 0;
        }

        void FreeMapValueToHeap(const MapValue &map_value) {
            /* Free the current value. */
            if (map_value.current_value != nullptr && map_value.current_value != map_value.default_value) {
                FreeToHeap(map_value.current_value, map_value.current_value_size);
            }

            /* Free the default value. */
            if (map_value.default_value != nullptr) {
                FreeToHeap(map_value.default_value, map_value.default_value_size);
            }
        }

        void FreeToHeap(void *block, size_t size) {
            AMS_UNUSED(size);
            lmem::FreeToExpHeap(GetHeapHandle(), block);
        }

        void *AllocateFromHeap(size_t size) {
            return lmem::AllocateFromExpHeap(GetHeapHandle(), size);
        }

        size_t GetHeapAllocatableSize() {
            return lmem::GetExpHeapAllocatableSize(GetHeapHandle(), sizeof(void *));
        }

        lmem::HeapHandle &GetHeapHandle() {
            AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(u8, s_heap_memory[HeapMemorySize]);
            AMS_FUNCTION_LOCAL_STATIC(lmem::HeapHandle, s_heap_handle, lmem::CreateExpHeap(s_heap_memory, sizeof(s_heap_memory), lmem::CreateOption_ThreadSafe));

            return s_heap_handle;
        }

        Result GetKeyValueStoreMap(Map **out) {
            /* Check preconditions. */
            AMS_ASSERT(out != nullptr);

            /* Get the map. */
            return GetKeyValueStoreMap(out, false);
        }

        Result GetKeyValueStoreMap(Map **out, bool force_load) {
            /* Check preconditions. */
            AMS_ASSERT(out != nullptr);

            /* Declare static instance variables. */
            AMS_FUNCTION_LOCAL_STATIC(Map, s_map);
            AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(bool, s_is_map_loaded, false);

            /* Get pointer to the map. */
            Map * const map = std::addressof(s_map);

            /* TODO: Mutex? */
            /* Load the map, if we haven't already. */
            if (AMS_UNLIKELY(!s_is_map_loaded)) {
                /* Attempt to load the map, allowing for failure if acceptable. */
                const auto result = LoadKeyValueStoreMap(map);

                if (!force_load) {
                    R_TRY(result);
                }

                /* Note that the map is loaded. */
                s_is_map_loaded = true;
            }

            /* Set the output pointer. */
            *out = map;
            return ResultSuccess();
        }

        Result GetKeyValueStoreMapForciblyForDebug(Map **out) {
            /* Check preconditions. */
            AMS_ASSERT(out != nullptr);

            /* Get the map. */
            return GetKeyValueStoreMap(out, true);
        }

        Result GetMapValueOfKeyValueStoreItemForDebug(MapValue *out, const KeyValueStoreItemForDebug &item) {
            /* Check preconditions. */
            AMS_ASSERT(out != nullptr);

            /* Create the map value. */
            MapValue map_value = {
                .type = item.type,
                .current_value_size = item.current_value_size,
                .default_value_size = item.default_value_size,
                .current_value      = nullptr,
                .default_value      = nullptr,
            };

            /* Ensure we free any buffers we allocate, if we fail. */
            ON_SCOPE_EXIT { FreeMapValueToHeap(map_value); };

            /* If the default value size is > 0, copy it to the map value. */
            if (map_value.default_value_size > 0) {
                /* Allocate the default value if there is sufficient memory available. */
                R_UNLESS(GetHeapAllocatableSize() >= map_value.default_value_size, settings::ResultSettingsItemValueAllocationFailed());
                map_value.default_value = AllocateFromHeap(map_value.default_value_size);
                AMS_ASSERT(map_value.default_value != nullptr);

                /* Copy the default value from the item. */
                std::memcpy(map_value.default_value, item.default_value, map_value.default_value_size);
            }

            /* If the current value and the default values are identical, set the map value to the default value. */
            if (CompareValue(item.current_value, item.current_value_size, item.default_value, item.default_value_size)) {
                map_value.current_value_size = map_value.default_value_size;
                map_value.current_value      = map_value.default_value;
            } else if (map_value.current_value_size > 0) {
                /* Allocate the current value if there is sufficient memory available. */
                R_UNLESS(GetHeapAllocatableSize() >= map_value.current_value_size, settings::ResultSettingsItemValueAllocationFailed());
                map_value.current_value = AllocateFromHeap(map_value.current_value_size);
                AMS_ASSERT(map_value.current_value != nullptr);

                /* Copy the current value from the item. */
                std::memcpy(map_value.current_value, item.current_value, map_value.current_value_size);
            }

            /* Set the output map value. */
            *out = map_value;

            /* Ensure we don't free the value buffers we returned. */
            map_value.current_value = nullptr;
            map_value.default_value = nullptr;
            return ResultSuccess();
        }

        template<typename T>
        const char *GetSystemDataMountName();

        template<>
        const char *GetSystemDataMountName<SystemDataTag::Fwdbg>() {
            return FwdbgSystemDataMountName;
        }

        template<>
        const char *GetSystemDataMountName<SystemDataTag::PfCfg>() {
            return PfCfgSystemDataMountName;
        }

        template<typename T>
        Result GetSystemData(SystemData **out_data, ncm::SystemDataId id) {
            /* Check pre-conditions. */
            AMS_ASSERT(out_data != nullptr);

            /* Declare static instance variables. */
            AMS_FUNCTION_LOCAL_STATIC(SystemData, s_data, id, GetSystemDataMountName<T>());
            AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(bool, s_mounted, false);

            /* Get pointer to the system data. */
            SystemData *data = std::addressof(s_data);

            /* TODO: Mutex? */
            /* Mount the system data, if we haven't already. */
            if (AMS_UNLIKELY(!s_mounted)) {
                /* Mount the system data. */
                R_TRY(data->Mount());

                /* Note that we mounted. */
                s_mounted = true;
            }

            /* Set the output pointer. */
            *out_data = data;
            return ResultSuccess();
        }

        Result GetSystemSaveData(SystemSaveData **out_data, bool create_save) {
            /* Check pre-conditions. */
            AMS_ASSERT(out_data != nullptr);

            /* Declare static instance variables. */
            AMS_FUNCTION_LOCAL_STATIC(SystemSaveData, s_data, SystemSaveDataId, SystemSaveDataSize, SystemSaveDataJournalSize, SystemSaveDataFlags, SystemSaveDataMountName);
            AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(bool, s_mounted, false);

            /* Get pointer to the system save data. */
            SystemSaveData *data = std::addressof(s_data);

            /* Mount the system data, if we haven't already. */
            if (AMS_UNLIKELY(!s_mounted)) {
                /* Mount the system data. */
                R_TRY(data->Mount(create_save));

                /* Note that we mounted. */
                s_mounted = true;
            }

            /* Set the output pointer. */
            *out_data = data;
            return ResultSuccess();
        }

        Result LoadKeyValueStoreMap(Map *out) {
            /* Check pre-conditions. */
            AMS_ASSERT(out != nullptr);

            /* Clear the key value store map. */
            ClearKeyValueStoreMap(*out);

            /* Get the firmware debug system data. */
            SystemData *system_data = nullptr;
            R_TRY(GetSystemData<SystemDataTag::Fwdbg>(std::addressof(system_data), ncm::SystemDataId::FirmwareDebugSettings));
            AMS_ASSERT(system_data != nullptr);

            /* Load the default keys/values for the firmware debug system data. */
            R_TRY(LoadKeyValueStoreMapDefault(out, *system_data));

            /* Load the keys/values based on the hardware type. */
            R_TRY(LoadKeyValueStoreMap(out, GetSplHardwareType()));

            if (IsSplDevelopment()) {
                /* Get the system save data. */
                SystemSaveData *system_save_data = nullptr;
                R_SUCCEED_IF(R_FAILED(GetSystemSaveData(std::addressof(system_save_data), false)));
                AMS_ASSERT(system_save_data != nullptr);

                /* Attempt to load the current keys/values from the system save data. */
                if (const auto result = LoadKeyValueStoreMapCurrent(out, *system_save_data); R_FAILED(result)) {
                    /* Reset all values to their defaults. */
                    for (auto &kv_pair : *out) {
                        MapValue &map_value = kv_pair.second;

                        /* Free the current value. */
                        if (map_value.current_value != nullptr && map_value.current_value != map_value.default_value) {
                            FreeToHeap(map_value.current_value, map_value.current_value_size);
                        }

                        /* Reset the current value to the default value. */
                        map_value.current_value_size = map_value.default_value_size;
                        map_value.current_value      = map_value.default_value;
                    }

                    /* Log failure to load system save data. TODO: Make this a warning. */
                    AMS_LOG("[firmware debug settings] Warning: Failed to load the system save data. (%08x, %d%03d-%04d)\n", result.GetInnerValue(), 2, result.GetModule(), result.GetDescription());
                }
            }

            return ResultSuccess();
        }

        Result LoadKeyValueStoreMap(Map *out, SplHardwareType hardware_type) {
            SystemData *data = nullptr;

            /* Get the platform configuration system data for the hardware type. */
            switch (hardware_type) {
                case SplHardwareType_None:
                    return ResultSuccess();
                case SplHardwareType_Icosa:
                    R_TRY(GetSystemData<SystemDataTag::PfCfg>(std::addressof(data), ncm::SystemDataId::PlatformConfigIcosa));
                    break;
                case SplHardwareType_IcosaMariko:
                    R_TRY(GetSystemData<SystemDataTag::PfCfg>(std::addressof(data), ncm::SystemDataId::PlatformConfigIcosaMariko));
                    break;
                case SplHardwareType_Copper:
                    R_TRY(GetSystemData<SystemDataTag::PfCfg>(std::addressof(data), ncm::SystemDataId::PlatformConfigCopper));
                    break;
                case SplHardwareType_Hoag:
                    R_TRY(GetSystemData<SystemDataTag::PfCfg>(std::addressof(data), ncm::SystemDataId::PlatformConfigHoag));
                    break;
                case SplHardwareType_Calcio:
                    R_TRY(GetSystemData<SystemDataTag::PfCfg>(std::addressof(data), ncm::SystemDataId::PlatformConfigCalcio));
                    break;
                case SplHardwareType_Aula:
                    R_TRY(GetSystemData<SystemDataTag::PfCfg>(std::addressof(data), ncm::SystemDataId::PlatformConfigAula));
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure data is not null. */
            AMS_ASSERT(data != nullptr);

            /* Load the key value store map. */
            return LoadKeyValueStoreMapDefault(out, *data);
        }

        template<typename T>
        Result LoadKeyValueStoreMapCurrent(Map *out, T &data) {
            /* Check pre-conditions. */
            AMS_ASSERT(out != nullptr);

            /* Open the data for reading. */
            R_TRY(data.OpenToRead());
            ON_SCOPE_EXIT { data.Close(); };

            /* Load the map entries. */
            R_TRY(LoadKeyValueStoreMapEntries(out, data, [](Map &map, const MapKey &key, u8 type, const void *value_buffer, u32 value_size) -> Result {
                AMS_UNUSED(type);
                /* Find the key in the map. */
                if (auto it = map.find(key); it != map.end()) {
                    MapValue &map_value = it->second;
                    size_t current_value_size = value_size;
                    void *current_value_buffer = nullptr;

                    if (current_value_size > 0) {
                        /* Ensure there is sufficient memory for the value. */
                        R_UNLESS(GetHeapAllocatableSize() >= current_value_size, settings::ResultSettingsItemValueAllocationFailed());

                        /* Allocate the value buffer. */
                        current_value_buffer = AllocateFromHeap(current_value_size);
                        AMS_ASSERT(current_value_buffer != nullptr);

                        /* Copy the value to the current value buffer. */
                        std::memcpy(current_value_buffer, value_buffer, current_value_size);
                    }

                    /* Replace the current value buffer with a new one. */
                    std::swap(map_value.current_value_size, current_value_size);
                    std::swap(map_value.current_value, current_value_buffer);

                    /* Free the old buffer if it is no longer in use. */
                    if (current_value_buffer != nullptr && current_value_buffer != map_value.default_value) {
                        FreeToHeap(current_value_buffer, current_value_size);
                    }
                }

                return ResultSuccess();
            }));

            return ResultSuccess();
        }

        template<typename T>
        Result LoadKeyValueStoreMapDefault(Map *out, T &data) {
            /* Check pre-conditions. */
            AMS_ASSERT(out != nullptr);

            /* Open the data for reading. */
            R_TRY(data.OpenToRead());
            ON_SCOPE_EXIT { data.Close(); };

            /* Load the map entries. */
            R_TRY(LoadKeyValueStoreMapEntries(out, data, [](Map &map, const MapKey &key, u8 type, const void *value_buffer, u32 value_size) -> Result {
                /* Ensure there is sufficient memory for two keys. */
                R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

                /* Copy the map key. */
                MapKey default_key = key;
                void *default_value_buffer = nullptr;

                ON_SCOPE_EXIT {
                    /* Free the value buffer if allocated. */
                    if (default_value_buffer != nullptr) {
                        FreeToHeap(default_value_buffer, value_size);
                    }
                };

                if (value_size > 0) {
                    /* Ensure there is sufficient memory for the value. */
                    R_UNLESS(GetHeapAllocatableSize() >= value_size, settings::ResultSettingsItemValueAllocationFailed());

                    /* Allocate the value buffer. */
                    default_value_buffer = AllocateFromHeap(value_size);
                    AMS_ASSERT(default_value_buffer != nullptr);

                    /* Copy the value to the new value buffer. */
                    std::memcpy(default_value_buffer, value_buffer, value_size);
                }

                /* Create the map value. */
                MapValue default_value {
                    .type               = type,
                    .current_value_size = value_size,
                    .default_value_size = value_size,
                    .current_value      = default_value_buffer,
                    .default_value      = default_value_buffer,
                };

                /* Ensure there is sufficient memory for the value. */
                R_UNLESS(GetHeapAllocatableSize() >= MapEntryBufferSize, settings::ResultSettingsItemValueAllocationFailed());

                /* Insert the value into the map. */
                map[std::move(default_key)] = default_value;
                return ResultSuccess();
            }));

            return ResultSuccess();
        }

        template<typename T, typename F>
        Result LoadKeyValueStoreMapEntries(Map *out, T &data, F load) {
            /* Check pre-conditions. */
            AMS_ASSERT(out != nullptr);
            AMS_ASSERT(load != nullptr);

            /* Read the number of entries. */
            s64 offset = 0;
            u32 total_size = 0;
            R_TRY(ReadData(data, offset, std::addressof(total_size), sizeof(total_size)));

            /* Iterate through all entries. NOTE: The offset is updated within LoadKeyValueStoreMapEntry. */
            while (offset < total_size) {
                R_TRY(LoadKeyValueStoreMapEntry(out, data, offset, load));
            }

            return ResultSuccess();
        }

        template<typename T, typename F>
        Result LoadKeyValueStoreMapEntry(Map *out, T &data, s64 &offset, F load) {
            /* Check pre-conditions. */
            AMS_ASSERT(out != nullptr);
            AMS_ASSERT(load != nullptr);

            /* Read the size of the key. */
            u32 key_size = 0;
            R_TRY(ReadData(data, offset, std::addressof(key_size), sizeof(key_size)));
            AMS_ASSERT(key_size > 1);

            /* Ensure there is sufficient memory for this key. */
            R_UNLESS(GetHeapAllocatableSize() >= key_size, settings::ResultSettingsItemKeyAllocationFailed());

            /* Read the key. */
            void *key_buffer = nullptr;
            R_TRY(ReadDataToHeap(data, offset, std::addressof(key_buffer), key_size));
            AMS_ASSERT(key_buffer != nullptr);
            ON_SCOPE_EXIT { FreeToHeap(key_buffer, key_size); };

            /* Ensure there is sufficient memory for two keys. */
            R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

            const MapKey key(static_cast<const char *>(key_buffer), key_size - 1);

            /* Read the type from the data. */
            u8 type = 0;
            R_TRY(ReadData(data, offset, std::addressof(type), sizeof(type)));

            /* Read the size of the value. */
            u32 value_size = 0;
            R_TRY(ReadData(data, offset, std::addressof(value_size), sizeof(value_size)));

            void *value_buffer = nullptr;
            ON_SCOPE_EXIT {
                if (value_buffer != nullptr) {
                    FreeToHeap(value_buffer, value_size);
                }
            };

            if (value_size > 0) {
                /* Ensure there is sufficient memory for the value. */
                R_UNLESS(GetHeapAllocatableSize() >= value_size, settings::ResultSettingsItemValueAllocationFailed());

                /* Read the value to the buffer. */
                R_TRY(ReadDataToHeap(data, offset, std::addressof(value_buffer), value_size));
            }

            /* Load the value. */
            return load(*out, key, type, value_buffer, value_size);
        }

        Result LoadKeyValueStoreMapForDebug(Map *out, SystemSaveData *system_save_data, SystemSaveData *fwdbg_system_save_data, SystemSaveData *pfcfg_system_save_data) {
            /* Check pre-conditions. */
            AMS_ASSERT(out != nullptr);
            AMS_ASSERT(system_save_data != nullptr);
            AMS_ASSERT(fwdbg_system_save_data != nullptr);
            AMS_ASSERT(pfcfg_system_save_data != nullptr);

            /* Clear the map. */
            ClearKeyValueStoreMap(*out);

            /* Load the default keys/values for the firmware debug system save data. */
            R_TRY(LoadKeyValueStoreMapDefault(out, *fwdbg_system_save_data));

            /* Load the default keys/values for the platform configuration system save data. */
            R_TRY(LoadKeyValueStoreMapDefault(out, *pfcfg_system_save_data));

            /* Load the current values for the system save data. */
            R_TRY(LoadKeyValueStoreMapCurrent(out, *system_save_data));
            return ResultSuccess();
        }

        template<typename T>
        Result ReadData(T &data, s64 &offset, void *buffer, size_t size) {
            AMS_ASSERT(buffer != nullptr);

            /* Read the data. */
            R_TRY(data.Read(offset, buffer, size));

            /* Increment the offset. */
            offset += static_cast<s64>(size);
            return ResultSuccess();
        }

        template<typename T>
        Result ReadDataToHeap(T &data, s64 &offset, void **buffer, size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(buffer != nullptr);
            AMS_ASSERT(size > 0);

            /* Allocate a buffer from the heap. */
            *buffer = AllocateFromHeap(size);
            AMS_ASSERT(*buffer != nullptr);

            /* Ensure we free the buffer if we fail. */
            auto alloc_guard = SCOPE_GUARD { FreeToHeap(*buffer, size); *buffer = nullptr; };

            /* Read data to the buffer. */
            R_TRY(ReadData(data, offset, *buffer, size));

            /* We succeeded. */
            alloc_guard.Cancel();
            return ResultSuccess();
        }

        template<typename T>
        Result ReadAllBytes(T *data, u64 *out_count, char * const out_buffer, size_t out_buffer_size) {
            /* Check preconditions. */
            AMS_ASSERT(data != nullptr);
            AMS_ASSERT(out_count != nullptr);
            AMS_ASSERT(out_buffer != nullptr);

            /* Open data for reading. */
            R_TRY(data->OpenToRead());
            ON_SCOPE_EXIT { data->Close(); };

            /* Read the data size. */
            u32 size = 0;
            R_TRY(data->Read(0, std::addressof(size), sizeof(size)));

            /* Ensure the data size does not exceed the buffer size. */
            size = std::min(size, static_cast<u32>(out_buffer_size));

            /* Read the data. */
            R_TRY(data->Read(0, out_buffer, size));

            /* Set the count. */
            *out_count = size;
            return ResultSuccess();
        }

        Result ReadSystemDataFirmwareDebug(u64 *out_count, char * const out_buffer, size_t out_buffer_size) {
            /* Check preconditions. */
            AMS_ASSERT(out_count != nullptr);
            AMS_ASSERT(out_buffer);

            /* Attempt to get the firmware debug system data. */
            SystemData *system_data = nullptr;
            if (R_SUCCEEDED(GetSystemData<SystemDataTag::Fwdbg>(std::addressof(system_data), ncm::SystemDataId::FirmwareDebugSettings))) {
                AMS_ASSERT(system_data != nullptr);

                /* Read the data. */
                R_TRY(ReadAllBytes(system_data, out_count, out_buffer, out_buffer_size));
            } else {
                /* Set the output count to 0. */
                *out_count = 0;
            }

            return ResultSuccess();
        }

        Result ReadSystemDataPlatformConfiguration(u64 *out_count, char * const out_buffer, size_t out_buffer_size) {
            /* Check preconditions. */
            AMS_ASSERT(out_count != nullptr);
            AMS_ASSERT(out_buffer);

            ncm::SystemDataId system_data_id;

            switch (GetSplHardwareType()) {
                case SplHardwareType_None:
                    *out_count = 0;
                    return ResultSuccess();
                case SplHardwareType_Icosa:
                    system_data_id = ncm::SystemDataId::PlatformConfigIcosa;
                    break;
                case SplHardwareType_IcosaMariko:
                    system_data_id = ncm::SystemDataId::PlatformConfigIcosaMariko;
                    break;
                case SplHardwareType_Copper:
                    system_data_id = ncm::SystemDataId::PlatformConfigCopper;
                    break;
                case SplHardwareType_Hoag:
                    system_data_id = ncm::SystemDataId::PlatformConfigHoag;
                    break;
                case SplHardwareType_Calcio:
                    system_data_id = ncm::SystemDataId::PlatformConfigCalcio;
                    break;
                case SplHardwareType_Aula:
                    system_data_id = ncm::SystemDataId::PlatformConfigAula;
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Attempt to get the platform configuration system data. */
            SystemData *system_data = nullptr;
            if (R_SUCCEEDED(GetSystemData<SystemDataTag::PfCfg>(std::addressof(system_data), system_data_id))) {
                AMS_ASSERT(system_data != nullptr);

                /* Read the data. */
                R_TRY(ReadAllBytes(system_data, out_count, out_buffer, out_buffer_size));
            } else {
                /* Set the output count to 0. */
                *out_count = 0;
            }

            return ResultSuccess();
        }

        Result ReadSystemSaveData(u64 *out_count, char * const out_buffer, size_t out_buffer_size) {
            /* Check preconditions. */
            AMS_ASSERT(out_count != nullptr);
            AMS_ASSERT(out_buffer);

            /* Attempt to get the system save data. */
            SystemSaveData *system_save_data = nullptr;
            if (R_SUCCEEDED(GetSystemSaveData(std::addressof(system_save_data), false))) {
                AMS_ASSERT(system_save_data != nullptr);

                /* Read the data. */
                R_TRY(ReadAllBytes(system_save_data, out_count, out_buffer, out_buffer_size));
            } else {
                /* Set the output count to 0. */
                *out_count = 0;
            }

            return ResultSuccess();
        }

        Result SaveKeyValueStoreMap(const Map &map) {
            /* Get the system save data. */
            SystemSaveData *system_save_data = nullptr;
            R_TRY(GetSystemSaveData(std::addressof(system_save_data), false));
            AMS_ASSERT(system_save_data != nullptr);

            /* Save the current values of the key value store map. */
            return SaveKeyValueStoreMapCurrent(*system_save_data, map);
        }

        template<typename T, typename F>
        Result SaveKeyValueStoreMap(T &data, const Map &map, F test) {
            /* Check preconditions. */
            AMS_ASSERT(test != nullptr);

            /* Create the save data if necessary. */
            R_TRY_CATCH(data.Create(HeapMemorySize)) {
                R_CATCH(fs::ResultPathAlreadyExists) { /* It's okay if the save data already exists. */ }
            } R_END_TRY_CATCH;

            {
                /* Open the save data for writing. */
                R_TRY(data.OpenToWrite());
                ON_SCOPE_EXIT {
                    /* Flush and close the save data. NOTE: Nintendo only does this if SetFileSize succeeds. */
                    R_ABORT_UNLESS(data.Flush());
                    data.Close();
                };

                /* Set the file size of the save data. */
                R_TRY(data.SetFileSize(HeapMemorySize));

                /* Write the data size, which includes itself. */
                u32 data_size = sizeof(data_size);
                R_TRY(data.Write(0, std::addressof(data_size), sizeof(data_size)));

                /* Set the current offset to after the data size. */
                s64 current_offset = sizeof(data_size);

                /* Iterate through map entries. */
                for (const auto &kv_pair : map) {
                    /* Declare variables for test. */
                    u8 type                  = 0;
                    const void *value_buffer = nullptr;
                    u32 value_size           = 0;

                    /* Test if the map value varies from the default. */
                    if (test(std::addressof(type), std::addressof(value_buffer), std::addressof(value_size), kv_pair.second)) {
                        R_TRY(SaveKeyValueStoreMapEntry(data, current_offset, kv_pair.first, type, value_buffer, value_size));
                    }
                }

                /* Write the updated save data size. */
                data_size = static_cast<u32>(current_offset);
                R_TRY(data.Write(0, std::addressof(data_size), sizeof(data_size)));
            }

            /* Commit the save data. */
            return data.Commit(false);
        }

        template<typename T>
        Result SaveKeyValueStoreMapCurrent(T &data, const Map &map) {
            /* Save the current values in the map to the data. */
            return SaveKeyValueStoreMap(data, map, [](u8 *out_type, const void **out_value_buffer, u32 *out_value_size, const MapValue &map_value) -> bool {
                /* Check preconditions. */
                AMS_ASSERT(out_type != nullptr);
                AMS_ASSERT(out_value_buffer != nullptr);
                AMS_ASSERT(out_value_size != nullptr);

                /* Check if the current value matches the default value. */
                if (CompareValue(map_value.current_value, map_value.current_value_size, map_value.default_value, map_value.default_value_size)) {
                    return false;
                }

                /* Output the map value type, current value and current value size. */
                *out_type         = map_value.type;
                *out_value_buffer = map_value.current_value;
                *out_value_size   = map_value.current_value_size;
                return true;
            });
        }

        template<typename T>
        Result SaveKeyValueStoreMapDefault(T &data, const Map &map) {
            /* Save the default values in the map to the data. */
            return SaveKeyValueStoreMap(data, map, [](u8 *out_type, const void **out_value_buffer, u32 *out_value_size, const MapValue &map_value) -> bool {
                /* Check preconditions. */
                AMS_ASSERT(out_type != nullptr);
                AMS_ASSERT(out_value_buffer != nullptr);
                AMS_ASSERT(out_value_size != nullptr);

                /* Output the map value type, default value and default value size. */
                *out_type         = map_value.type;
                *out_value_buffer = map_value.default_value;
                *out_value_size   = map_value.default_value_size;
                return true;
            });
        }

        Result SaveKeyValueStoreMapDefaultForDebug(SystemSaveData &data, const Map &map) {
            return SaveKeyValueStoreMapDefault(data, map);
        }

        template<typename T>
        Result SaveKeyValueStoreMapEntry(T &data, s64 &offset, const MapKey &key, u8 type, const void *value_buffer, u32 value_size) {
            /* Write the key size and increment the offset. */
            const u32 key_size = key.GetCount() + 1;
            R_TRY(data.Write(offset, std::addressof(key_size), sizeof(key_size)));
            offset += static_cast<s64>(sizeof(key_size));

            /* Write the key string and increment the offset. */
            R_TRY(data.Write(offset, key.GetString(), key_size));
            offset += static_cast<s64>(key_size);

            /* Write the type and increment the offset. */
            R_TRY(data.Write(offset, std::addressof(type), sizeof(type)));
            offset += static_cast<s64>(sizeof(type));

            /* Write the value size and increment the offset. */
            R_TRY(data.Write(offset, std::addressof(value_size), sizeof(value_size)));
            offset += static_cast<s64>(sizeof(value_size));

            /* If the value is larger than 0, write it to the data. */
            if (value_size > 0) {
                /* Check preconditions. */
                AMS_ASSERT(value_buffer != nullptr);

                R_TRY(data.Write(offset, value_buffer, value_size));
                offset += static_cast<s64>(value_size);
            }

            return ResultSuccess();
        }

    }

    Result KeyValueStore::CreateKeyIterator(KeyValueStoreKeyIterator *out) {
        /* Check preconditions. */
        AMS_ASSERT(out != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Ensure there is sufficient memory for two keys. */
        R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

        /* Create a map key from the key value store's name. */
        MapKey map_key_header(m_name.value);

        /* Append the settings name separator. */
        map_key_header.Append(SettingsNameSeparator);

        /* Define the item map key. */
        const MapKey *item_map_key = nullptr;

        /* Find an item map key with the name as a prefix. */
        for (const auto &kv_pair : *map) {
            const MapKey &map_key = kv_pair.first;

            /* Check if the name map key is smaller than the current map key, and the current map key contains the name map key. */
            if (map_key_header < map_key && map_key.Find(map_key_header)) {
                item_map_key = std::addressof(map_key);
                break;
            }
        }

        /* Ensure we have located an item map key. */
        R_UNLESS(item_map_key != nullptr, settings::ResultSettingsItemNotFound());

        /* Ensure there is sufficient memory for the item map key. */
        const size_t item_map_key_size = item_map_key->GetCount() + 1;
        R_UNLESS(GetHeapAllocatableSize() >= item_map_key_size, settings::ResultSettingsItemKeyIteratorAllocationFailed());

        /* Allocate the key buffer. */
        char *buffer = static_cast<char *>(AllocateFromHeap(item_map_key_size));
        AMS_ASSERT(buffer != nullptr);

        /* Copy the item map key's string to the buffer. */
        std::memcpy(buffer, item_map_key->GetString(), item_map_key_size);

        /* Output the iterator. */
        *out = {
            .header_size = static_cast<size_t>(map_key_header.GetCount()),
            .entire_size = item_map_key_size,
            .map_key     = buffer,
        };
        return ResultSuccess();
    }

    Result KeyValueStore::GetValue(u64 *out_count, char *out_buffer, size_t out_buffer_size, const SettingsItemKey &item_key) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);
        AMS_ASSERT(out_buffer != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Ensure there is sufficient memory for two keys. */
        R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

        /* Find the key in the map. */
        const Map::const_iterator it = map->find(MakeMapKey(m_name, item_key));
        R_UNLESS(it != map->end(), settings::ResultSettingsItemNotFound());

        /* Get the map value from the iterator. */
        const MapValue &map_value = it->second;

        /* Calculate the current value size. */
        const size_t current_value_size = std::min(map_value.current_value_size, out_buffer_size);

        /* If the current value size is > 0, copy to the output buffer. */
        if (current_value_size > 0) {
            AMS_ASSERT(map_value.current_value != nullptr);
            std::memcpy(out_buffer, map_value.current_value, current_value_size);
        }

        /* Set the output count. */
        *out_count = current_value_size;
        return ResultSuccess();
    }

    Result KeyValueStore::GetValueSize(u64 *out_value_size, const SettingsItemKey &item_key) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Ensure there is sufficient memory for two keys. */
        R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

        /* Find the key in the map. */
        const Map::const_iterator it = map->find(MakeMapKey(m_name, item_key));
        R_UNLESS(it != map->end(), settings::ResultSettingsItemNotFound());

        /* Output the value size. */
        *out_value_size = it->second.current_value_size;
        return ResultSuccess();
    }

    Result KeyValueStore::ResetValue(const SettingsItemKey &item_key) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Ensure there is sufficient memory for two keys. */
        R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

        /* Find the key in the map. */
        const Map::iterator it = map->find(MakeMapKey(m_name, item_key));
        R_UNLESS(it != map->end(), settings::ResultSettingsItemNotFound());

        /* Get the map value from the iterator. */
        MapValue &map_value = it->second;

        /* Succeed if the map value has already been reset. */
        R_SUCCEED_IF(map_value.current_value == map_value.default_value);

        /* Store the previous value and its size. */
        size_t prev_value_size = map_value.current_value_size;
        void *prev_value       = map_value.current_value;

        /* Reset the current value to default. */
        map_value.current_value_size = map_value.default_value_size;
        map_value.current_value      = map_value.default_value;

        /* Attempt to save the key value store map. */
        if (const auto result = SaveKeyValueStoreMap(*map); R_FAILED(result)) {
            /* Revert to the previous value. */
            map_value.current_value_size = prev_value_size;
            map_value.current_value      = prev_value;

            /* Attempt to save the map again. Nintendo does not check the result of this. */
            SaveKeyValueStoreMap(*map);
            return result;
        }

        /* If present, free the previous value. */
        if (prev_value != nullptr && prev_value != map_value.default_value) {
            FreeToHeap(prev_value, prev_value_size);
        }

        return ResultSuccess();
    }

    Result KeyValueStore::SetValue(const SettingsItemKey &item_key, const void *buffer, size_t buffer_size) {
        /* Check preconditions. */
        AMS_ASSERT(buffer != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Ensure there is sufficient memory for two keys. */
        R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

        /* Find the key in the map. */
        const Map::iterator it = map->find(MakeMapKey(m_name, item_key));
        R_UNLESS(it != map->end(), settings::ResultSettingsItemNotFound());

        /* Get the map value from the iterator. */
        MapValue &map_value = it->second;

        /* Succeed if the map value is already set to the new value. */
        R_SUCCEED_IF(CompareValue(map_value.current_value, map_value.current_value_size, buffer, buffer_size));

        /* Define the value buffer and size variables. */
        size_t value_size = buffer_size;
        void *value_buffer = nullptr;

        /* Set the value buffer to the default value if the new value is the same. */
        if (CompareValue(map_value.default_value, map_value.default_value_size, buffer, buffer_size)) {
            value_buffer = map_value.default_value;
        } else if (buffer_size > 0) {
            /* Allocate the new value if there is sufficient memory available. */
            R_UNLESS(GetHeapAllocatableSize() >= value_size, settings::ResultSettingsItemValueAllocationFailed());
            value_buffer = AllocateFromHeap(value_size);
            AMS_ASSERT(value_buffer != nullptr);

            /* Copy the value to the value buffer. */
            std::memcpy(value_buffer, buffer, value_size);
        }

        /* Swap the current value with the new value. */
        std::swap(map_value.current_value_size, value_size);
        std::swap(map_value.current_value, value_buffer);

        /* Attempt to save the key value store map. */
        const auto result = SaveKeyValueStoreMap(*map);

        /* If we failed, revert to the previous value. */
        if (R_FAILED(result)) {
            std::swap(map_value.current_value_size, value_size);
            std::swap(map_value.current_value, value_buffer);
        }

        /* Free the now unused value. */
        if (value_buffer != nullptr && value_buffer != map_value.default_value) {
            FreeToHeap(value_buffer, value_size);
        }

        /* If we failed, attempt to save the map again. Note that Nintendo does not check the result of this. */
        if (R_FAILED(result)) {
            SaveKeyValueStoreMap(*map);
            return result;
        }

       return ResultSuccess();
    }

    Result AddKeyValueStoreItemForDebug(const KeyValueStoreItemForDebug * const items, size_t items_count) {
        /* Check preconditions. */
        AMS_ASSERT(items != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMapForciblyForDebug(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Iterate through each item. */
        for (size_t i = 0; i < items_count; i++) {
            const KeyValueStoreItemForDebug &item = items[i];

            /* Create a map value for our scope. */
            MapValue map_value = {};
            ON_SCOPE_EXIT { FreeMapValueToHeap(map_value); };

            /* Get the map value for the item. */
            R_TRY(GetMapValueOfKeyValueStoreItemForDebug(std::addressof(map_value), item));

            /* Ensure there is sufficient memory for two keys. */
            R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

            /* Create the map key. */
            MapKey map_key(item.key);

            /* Replace the existing value in the map if it already exists. */
            if (const Map::iterator it = map->find(map_key); it != map->end()) {
                /* Free the existing map value. */
                FreeMapValueToHeap(it->second);

                /* Replace the existing map value. */
                it->second = map_value;
            } else {
                /* Ensure there is sufficient memory for the value. */
                R_UNLESS(GetHeapAllocatableSize() >= MapEntryBufferSize, settings::ResultSettingsItemValueAllocationFailed());

                /* Assign the map value to the map key in the map. */
                (*map)[std::move(map_key)] = map_value;
            }

            /* Ensure we don't free the value buffers we added. */
            map_value.current_value = nullptr;
            map_value.default_value = nullptr;
        }

        return ResultSuccess();
    }

    Result AdvanceKeyValueStoreKeyIterator(KeyValueStoreKeyIterator *out) {
        /* Check preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out->header_size > 0);
        AMS_ASSERT(out->header_size < out->entire_size);
        AMS_ASSERT(out->map_key != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Ensure there is sufficient memory for two keys. */
        R_UNLESS(GetHeapAllocatableSize() >= MapKeyBufferSize, settings::ResultSettingsItemKeyAllocationFailed());

        /* Locate the iterator's current key. */
        Map::const_iterator it = map->find(MapKey(out->map_key, static_cast<s32>(out->entire_size) - 1));
        R_UNLESS(it != map->end(), settings::ResultNotFoundSettingsItemKeyIterator());

        /* Increment the iterator, ensuring we aren't at the end of the map. */
        R_UNLESS((++it) != map->end(), settings::ResultStopIteration());

        /* Get the map key. */
        const MapKey &map_key = it->first;

        /* Ensure the advanced iterator retains the required name. */
        R_UNLESS(std::strncmp(map_key.GetString(), out->map_key, out->header_size) == 0, settings::ResultStopIteration());

        /* Ensure there is sufficient memory for the map key. */
        const size_t map_key_size = map_key.GetCount() + 1;
        R_UNLESS(GetHeapAllocatableSize() >= map_key_size, settings::ResultSettingsItemKeyIteratorAllocationFailed());

        /* Free the iterator's old map key. */
        FreeToHeap(out->map_key, out->entire_size);

        /* Allocate the new map key. */
        char *buffer = static_cast<char *>(AllocateFromHeap(map_key_size));
        AMS_ASSERT(buffer != nullptr);

        /* Copy the new map key to the buffer. */
        std::memcpy(buffer, map_key.GetString(), map_key_size);

        /* Set the output map key. */
        out->entire_size = map_key_size;
        out->map_key     = buffer;

        return ResultSuccess();
    }

    Result DestroyKeyValueStoreKeyIterator(KeyValueStoreKeyIterator *out) {
        /* Check preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out->header_size > 0);
        AMS_ASSERT(out->header_size < out->entire_size);
        AMS_ASSERT(out->map_key != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Free the key to the heap. */
        FreeToHeap(out->map_key, out->entire_size);

        /* Reset the name and key. */
        out->header_size = 0;
        out->entire_size = 0;
        out->map_key     = nullptr;
        return ResultSuccess();
    }

    Result GetKeyValueStoreItemCountForDebug(u64 *out_count) {
        /* Check preconditions. */
        AMS_ASSERT(out != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Output the item count. */
        *out_count = map->size();
        return ResultSuccess();
    }

    Result GetKeyValueStoreItemForDebug(u64 *out_count, KeyValueStoreItemForDebug * const out_items, size_t out_items_count) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);
        AMS_ASSERT(out_items != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Define the count variable. */
        size_t count = 0;

        /* Iterate through each value in the map and output kvs items. */
        for (const auto &kv_pair : *map) {
            /* Get the current map value. */
            const MapValue &map_value = kv_pair.second;

            /* Break if the count exceeds the output items count. */
            if (count >= out_items_count) {
                break;
            }

            /* Get the current item. */
            KeyValueStoreItemForDebug &item = out_items[count++];

            /* Copy the map key and value to the item. */
            item.key                = kv_pair.first.GetString();
            item.type               = map_value.type;
            item.current_value_size = map_value.current_value_size;
            item.default_value_size = map_value.default_value_size;
            item.current_value      = map_value.current_value;
            item.default_value      = map_value.default_value;
        }

        /* Set the output count. */
        *out_count = count;
        return ResultSuccess();
    }

    Result GetKeyValueStoreKeyIteratorKey(u64 *out_count, char *out_buffer, size_t out_buffer_size, const KeyValueStoreKeyIterator &iterator) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);
        AMS_ASSERT(out_buffer != nullptr);
        AMS_ASSERT(iterator.header_size > 0);
        AMS_ASSERT(iterator.header_size < iterator.entire_size);
        AMS_ASSERT(iterator.map_key != nullptr);

        /* Copy the key from the iterator to the output buffer. */
        const size_t key_size = std::min(out_buffer_size, std::min(iterator.entire_size - iterator.header_size, SettingsItemKeyLengthMax + 1));
        std::strncpy(out_buffer, iterator.map_key + iterator.header_size, key_size);

        /* Set the end of the key to null. */
        if (key_size > 0) {
            out_buffer[key_size - 1] = '\x00';
        }

        /* Output the key size. */
        *out_count = key_size;
        return ResultSuccess();
    }

    Result GetKeyValueStoreKeyIteratorKeySize(u64 *out_count, const KeyValueStoreKeyIterator &iterator) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);
        AMS_ASSERT(iterator.header_size > 0);
        AMS_ASSERT(iterator.header_size < iterator.entire_size);
        AMS_ASSERT(iterator.map_key != nullptr);

        /* Output the key size. */
        *out_count = std::min(iterator.entire_size - iterator.header_size, SettingsItemKeyLengthMax + 1);
        return ResultSuccess();
    }

    Result ReadKeyValueStoreFirmwareDebug(u64 *out_count, char * const out_buffer, size_t out_buffer_size) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);
        AMS_ASSERT(out_buffer != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Read the firmware debug system data. */
        return ReadSystemDataFirmwareDebug(out_count, out_buffer, out_buffer_size);
    }

    Result ReadKeyValueStorePlatformConfiguration(u64 *out_count, char * const out_buffer, size_t out_buffer_size) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);
        AMS_ASSERT(out_buffer != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Read the platform configuration system data. */
        return ReadSystemDataPlatformConfiguration(out_count, out_buffer, out_buffer_size);
    }

    Result ReadKeyValueStoreSaveData(u64 *out_count, char * const out_buffer, size_t out_buffer_size) {
        /* Check preconditions. */
        AMS_ASSERT(out_count != nullptr);
        AMS_ASSERT(out_buffer != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Read the system save data. */
        return ReadSystemSaveData(out_count, out_buffer, out_buffer_size);
    }

    Result ReloadKeyValueStoreForDebug(SystemSaveData *system_save_data, SystemSaveData *fwdbg_system_data, SystemSaveData *pfcfg_system_data) {
        /* Check preconditions. */
        AMS_ASSERT(system_save_data != nullptr);
        AMS_ASSERT(fwdbg_system_data != nullptr);
        AMS_ASSERT(pfcfg_system_data != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMapForciblyForDebug(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Load the key value store map. */
        return LoadKeyValueStoreMapForDebug(map, system_save_data, fwdbg_system_data, pfcfg_system_data);
    }

    Result ReloadKeyValueStoreForDebug() {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Load the key value store map. */
        return LoadKeyValueStoreMap(map);
    }

    Result ResetKeyValueStoreSaveData() {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Reset all values in the map. */
        for (auto &kv_pair : *map) {
            /* Get the map value. */
            MapValue &map_value = kv_pair.second;

            /* If the current value isn't the default value, reset it. */
            if (map_value.current_value != map_value.default_value) {
                /* Store the previous value and size. */
                size_t prev_value_size = map_value.current_value_size;
                void *prev_value       = map_value.current_value;

                /* Reset the current value to the default value. */
                map_value.current_value_size = map_value.default_value_size;
                map_value.current_value      = map_value.default_value;

                /* Free the current value if present. */
                if (prev_value != nullptr) {
                    FreeToHeap(prev_value, prev_value_size);
                }
            }
        }

        /* Save the key value store map. */
        return SaveKeyValueStoreMap(*map);
    }

    Result SaveKeyValueStoreAllForDebug(SystemSaveData *data) {
        /* Check preconditions. */
        AMS_ASSERT(data != nullptr);

        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_key_value_store_mutex);

        /* Get the key value store map. */
        Map *map = nullptr;
        R_TRY(GetKeyValueStoreMap(std::addressof(map)));
        AMS_ASSERT(map != nullptr);

        /* Save the key value store map default values. */
        R_TRY(SaveKeyValueStoreMapDefaultForDebug(*data, *map));

        /* Save the key value store map. */
        return SaveKeyValueStoreMap(*map);
    }

}
