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
#include "ncm_file_mapper_file.hpp"
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace impl {

        template<typename T>
        concept IsMappedMemorySpan = std::same_as<T, Span<u8>>;

        constexpr inline u64 InitialIdCounterValue = 0x12345;

    }

    class SingleCacheMapperBase : public IMapper {
        private:
            bool m_is_mapped;
            MappedMemory m_mapped_memory;
            size_t m_accessible_size;
            bool m_is_using;
            bool m_is_dirty;
            u64 m_id_counter;
            u8 *m_buffer;
            size_t m_buffer_size;
        public:
            SingleCacheMapperBase(Span<u8> span) : m_is_mapped(false), m_mapped_memory{}, m_accessible_size(0), m_is_using(false), m_is_dirty(false), m_id_counter(impl::InitialIdCounterValue), m_buffer(span.data()), m_buffer_size(span.size_bytes()) {
                /* ... */
            }
        protected:
            void Finalize() {
                /* If we're unused and mapped, we should unmap. */
                if (!m_is_using && m_is_mapped) {
                    this->Unmap();
                }
            }
        private:
            Result Unmap() {
                /* Check pre-conditions. */
                AMS_ASSERT(m_is_mapped);

                /* If we're dirty, we'll need to flush the entry. */
                if (m_is_dirty) {
                    /* Unmap our memory. */
                    MappedMemory mem = m_mapped_memory;
                    if (mem.offset + mem.buffer_size > m_accessible_size) {
                        mem.buffer_size = m_accessible_size - mem.offset;
                    }

                    R_TRY(this->UnmapImpl(std::addressof(mem)));
                }

                /* Set as dirty/not mapped. */
                m_is_dirty  = false;
                m_is_mapped = false;
                R_SUCCEED();
            }

            Result GetMappedMemoryImpl(MappedMemory *out, size_t offset, size_t size) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_is_mapped);

                /* Ensure the accessible size works. */
                const bool can_update = this->IsAccessibleSizeUpdatable();
                R_UNLESS((offset + size <= m_accessible_size || can_update), ncm::ResultMapperInvalidArgument());

                /* Update our accessible size. */
                m_accessible_size = std::max<size_t>(m_accessible_size, size + offset);

                /* Set the output memory. */
                *out = m_mapped_memory;
                out->buffer_size = std::min<size_t>(out->buffer_size, m_accessible_size - out->offset);
                R_SUCCEED();
            }
        public:
            virtual Result GetMappedMemory(MappedMemory *out, size_t offset, size_t size) override final {
                /* Ensure our memory is valid, if it's already mapped. */
                if (m_is_mapped) {
                    /* If we can re-use the previous mapping, do so. */
                    if (m_mapped_memory.IsIncluded(offset, size)) {
                        /* If the memory is in use, we can't get a new mapping. */
                        R_UNLESS(!m_is_using, ncm::ResultMapperBusy());

                        /* Get the output memory. */
                        R_RETURN(this->GetMappedMemoryImpl(out, offset, size));
                    }

                    /* We don't have the correct data mapped, so we need to map. */
                    R_TRY(this->Unmap());
                }

                /* Map. */
                R_TRY(this->MapImpl(std::addressof(m_mapped_memory), Span<u8>(m_buffer, m_buffer_size), offset, size));

                /* Set our mapping id. */
                m_mapped_memory.id = m_id_counter++;

                /* Get the output memory. */
                R_RETURN(this->GetMappedMemoryImpl(out, offset, size));
            }

            virtual Result MarkUsing(u64 id) override final {
                /* Check that the mapping is correct. */
                R_UNLESS(m_is_mapped,              ncm::ResultMapperNotMapped());
                R_UNLESS(m_mapped_memory.id == id, ncm::ResultMapperNotMapped());

                /* Mark as using. */
                m_is_using = true;
                R_SUCCEED();
            }

            virtual Result UnmarkUsing(u64 id) override final {
                /* Check that the mapping is correct. */
                R_UNLESS(m_is_mapped,              ncm::ResultMapperNotMapped());
                R_UNLESS(m_mapped_memory.id == id, ncm::ResultMapperNotMapped());

                /* Mark as not using. */
                m_is_using = false;
                R_SUCCEED();
            }

            virtual Result MarkDirty(u64 id) override final {
                /* Check that the mapping is correct. */
                R_UNLESS(m_is_mapped,              ncm::ResultMapperNotMapped());
                R_UNLESS(m_mapped_memory.id == id, ncm::ResultMapperNotMapped());

                /* Mark as dirty. */
                m_is_dirty = true;
                R_SUCCEED();
            }
    };

    template<size_t MaxEntries>
    class MultiCacheReadonlyMapperBase : public IMapper {
        private:
            struct Entry {
                MappedMemory memory;
                u64 lru_counter;
                u32 use_count;
                bool is_mapped;
                u8 *buffer;
                size_t buffer_size;
            };
        private:
            Entry m_entry_storages[MaxEntries];
            Entry * const m_entries;
            size_t m_entry_count;
            u64 m_id_counter;
            u64 m_lru_counter;
            size_t m_accessible_size;
        public:
            template<impl::IsMappedMemorySpan... Args>
            MultiCacheReadonlyMapperBase(Args... args) : m_entries(m_entry_storages), m_entry_count(sizeof...(Args)), m_id_counter(impl::InitialIdCounterValue), m_lru_counter(1), m_accessible_size(0) {
                /* Check the argument count is valid. */
                static_assert(sizeof...(Args) <= MaxEntries);

                /* Initialize entries. */
                auto InitializeEntry = [](Entry *entry, Span<u8> span) ALWAYS_INLINE_LAMBDA -> void {
                    *entry = {};
                    entry->buffer      = span.data();
                    entry->buffer_size = span.size_bytes();
                };

                Entry *cur_entry = m_entries;
                (InitializeEntry(cur_entry++, args), ...);
            }

            size_t GetSize() {
                return m_accessible_size;
            }
        protected:
            void SetSize(size_t size) {
                m_accessible_size = size;
            }

            void Finalize() {
                /* Mark all entries as unmapped. */
                for (size_t i = 0; i < m_entry_count; ++i) {
                    /* We can't mark unmapped an entry which is in use. */
                    if (m_entries[i].use_count > 0) {
                        break;
                    }

                    if (m_entries[i].is_mapped) {
                        m_entries[i].is_mapped = false;
                    }
                }
            }
        private:
            Result GetMappedMemoryImpl(MappedMemory *out, const MappedMemory &src) {
                /* Set the output memory. */
                *out = src;
                out->buffer_size = std::min<size_t>(out->buffer_size, m_accessible_size - out->offset);
                R_SUCCEED();
            }
        public:
            virtual Result GetMappedMemory(MappedMemory *out, size_t offset, size_t size) override final {
                /* Try to find an entry which contains the desired region. */
                for (size_t i = 0; i < m_entry_count; ++i) {
                    if (m_entries[i].is_mapped && m_entries[i].memory.IsIncluded(offset, size)) {
                        R_RETURN(this->GetMappedMemoryImpl(out, m_entries[i].memory));
                    }
                }

                /* Find the oldest entry. */
                Entry *oldest     = nullptr;
                Entry *best_entry = nullptr;
                for (size_t i = 0; i < m_entry_count; ++i) {
                    if (m_entries[i].is_mapped) {
                        if (m_entries[i].use_count == 0) {
                            if (oldest == nullptr || m_entries[i].lru_counter < oldest->lru_counter) {
                                oldest = std::addressof(m_entries[i]);
                            }
                        }
                    } else {
                        best_entry = std::addressof(m_entries[i]);
                    }
                }

                /* If we didn't find a free entry, use the oldest. */
                best_entry = best_entry != nullptr ? best_entry : oldest;
                R_UNLESS(best_entry != nullptr, ncm::ResultMapperBusy());

                /* Ensure the best entry isn't mapped. */
                if (best_entry->is_mapped) {
                    best_entry->is_mapped = false;
                }

                /* Map. */
                R_TRY(this->MapImpl(std::addressof(best_entry->memory), Span<u8>(best_entry->buffer, best_entry->buffer_size), offset, size));

                /* Set our mapping id. */
                best_entry->memory.id = m_id_counter++;

                /* Get the output memory. */
                R_RETURN(this->GetMappedMemoryImpl(out, best_entry->memory));
            }

            virtual Result MarkUsing(u64 id) override final {
                /* Try to unmark the entry. */
                for (size_t i = 0; i < m_entry_count; ++i) {
                    if (m_entries[i].memory.id == id) {
                        ++m_entries[i].use_count;
                        m_entries[i].lru_counter = m_lru_counter++;
                        R_SUCCEED();
                    }
                }

                /* We failed to unmark. */
                R_THROW(ncm::ResultMapperNotMapped());
            }

            virtual Result UnmarkUsing(u64 id) override final {
                /* Try to unmark the entry. */
                for (size_t i = 0; i < m_entry_count; ++i) {
                    if (m_entries[i].memory.id == id) {
                        --m_entries[i].use_count;
                        R_SUCCEED();
                    }
                }

                /* We failed to unmark. */
                R_THROW(ncm::ResultMapperNotMapped());
            }

            virtual Result MarkDirty(u64) override  final{
                R_THROW(ncm::ResultMapperNotSupported());
            }
    };

    template<typename CacheMapperBase>
    class ExtendedDataMapperBase : public CacheMapperBase {
        private:
            static constexpr size_t MappingAlignment = 1_KB;
        private:
            util::optional<impl::MountNameString> m_mount_name = util::nullopt;
            ncm::FileMapperFile m_file_mapper{};
            size_t m_extended_data_offset;
            bool m_suppress_fs_auto_abort;
        public:
            template<typename... Args>
            ExtendedDataMapperBase(Args &&... args) : CacheMapperBase(std::forward<Args>(args)...) { /* ... */ }

            virtual ~ExtendedDataMapperBase() override {
                /* Finalize. */
                this->Finalize();
            }

            Result Initialize(const char *content_path, fs::ContentAttributes attr, bool suppress_fs_auto_abort) {
                /* Set whether we should suppress fs aborts. */
                m_suppress_fs_auto_abort = suppress_fs_auto_abort;

                /* Suppress fs auto abort, if we need to. */
                auto disable_aborts = this->GetFsAutoAbortDisabler();

                /* Mount the content. */
                auto mount_name = impl::CreateUniqueMountName();
                R_TRY(impl::MountContentMetaImpl(mount_name.str, content_path, attr));

                /* Set our mount name. */
                m_mount_name.emplace(mount_name.str);

                /* Open the root directory. */
                auto root_path = impl::GetRootDirectoryPath(mount_name);
                fs::DirectoryHandle dir;
                R_TRY(fs::OpenDirectory(std::addressof(dir), root_path.str, fs::OpenDirectoryMode_File));
                ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                /* Loop directory reading until we find the entry we're looking for. */
                while (true) {
                    /* Read one entry, and finish when we fail to read. */
                    fs::DirectoryEntry entry;
                    s64 num_read;
                    R_TRY(fs::ReadDirectory(std::addressof(num_read), std::addressof(entry), dir, 1));
                    if (num_read == 0) {
                        break;
                    }

                    /* If this is the content meta file, parse it. */
                    if (IsContentMetaFileName(entry.name)) {
                        /* Create the file path. */
                        impl::FilePathString file_path(root_path.str);
                        file_path.Append(entry.name);

                        /* Setup our file mapped. */
                        R_TRY(m_file_mapper.Initialize(file_path, FileMapperFile::OpenMode::Read));

                        /* Read the extended header. */
                        PackagedContentMetaHeader pkg_header;
                        R_TRY(m_file_mapper.Read(0, std::addressof(pkg_header), sizeof(pkg_header)));

                        /* Set our extended data offset. */
                        m_extended_data_offset = PackagedContentMetaReader(std::addressof(pkg_header), sizeof(pkg_header)).GetExtendedDataOffset();

                        const size_t accessible_size = m_file_mapper.GetFileSize() >= m_extended_data_offset;
                        R_UNLESS(accessible_size, ncm::ResultInvalidContentMetaFileSize());

                        /* Set our accessible size. */
                        this->SetSize(accessible_size);
                        R_SUCCEED();
                    }
                }

                R_THROW(ncm::ResultContentMetaNotFound());
            }

            void Finalize() {
                /* Suppress fs auto abort, if we need to. */
                auto disable_aborts = this->GetFsAutoAbortDisabler();

                /* Finalize our implementation. */
                CacheMapperBase::Finalize();

                /* Finalize our file mapper. */
                m_file_mapper.Finalize();

                /* Finalize our mount name. */
                if (m_mount_name.has_value()) {
                    fs::Unmount(m_mount_name.value().Get());
                    m_mount_name = util::nullopt;
                }
            }
        protected:
            virtual Result MapImpl(MappedMemory *out, Span<u8> data, size_t offset, size_t size) override final {
                /* Suppress fs auto abort, if we need to. */
                auto disable_aborts = this->GetFsAutoAbortDisabler();

                /* Get the requested map offset/size. */
                u8 *map_data    = data.data();
                size_t map_size = data.size_bytes();

                /* Align the mapping, and ensure it remains valid. */
                const size_t aligned_offset = util::AlignDown(offset, MappingAlignment);
                R_UNLESS((offset + size) - aligned_offset <= map_size, ncm::ResultMapperInvalidArgument());

                /* Read the data. */
                const size_t map_offset = m_extended_data_offset + aligned_offset;
                if (map_offset + map_size >= m_file_mapper.GetFileSize()) {
                    map_size = m_file_mapper.GetFileSize() - map_offset;
                }
                R_TRY(m_file_mapper.Read(map_offset, map_data, map_size));

                /* Create the output mapped memory. */
                *out = MappedMemory {
                    .id          = 0,
                    .offset      = aligned_offset,
                    .buffer      = map_data,
                    .buffer_size = map_size,
                };
                R_SUCCEED();
            }

            virtual Result UnmapImpl(MappedMemory *) override final {
                R_THROW(ncm::ResultMapperNotSupported());
            }

            virtual bool IsAccessibleSizeUpdatable() override final {
                return false;
            }
        private:
            util::optional<fs::ScopedAutoAbortDisabler> GetFsAutoAbortDisabler() {
                /* Create an abort disabler, if we should disable aborts. */
                util::optional<fs::ScopedAutoAbortDisabler> disable_abort{util::nullopt};
                if (m_suppress_fs_auto_abort) {
                    disable_abort.emplace();
                }
                return disable_abort;
            }
    };

    template<size_t N>
    using MultiCacheReadonlyMapper = ExtendedDataMapperBase<MultiCacheReadonlyMapperBase<N>>;

}