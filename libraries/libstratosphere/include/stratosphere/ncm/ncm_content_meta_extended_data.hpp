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
#include <stratosphere/ncm/ncm_content_info.hpp>
#include <stratosphere/ncm/ncm_content_meta_id.hpp>
#include <stratosphere/ncm/ncm_content_meta.hpp>
#include <stratosphere/ncm/ncm_firmware_variation.hpp>
#include <stratosphere/ncm/ncm_mapped_memory.hpp>

namespace ams::ncm {

    enum class UpdateType : u8 {
        ApplyAsDelta = 0,
        Overwrite    = 1,
        Create       = 2,
    };

    struct FragmentIndicator {
        u16 content_info_index;
        u16 fragment_index;
    };

    struct FragmentSet {
        ContentId source_content_id;
        ContentId destination_content_id;
        u32 source_size_low;
        u16 source_size_high;
        u16 destination_size_high;
        u32 destination_size_low;
        u16 fragment_count;
        ContentType target_content_type;
        UpdateType update_type;
        u8 reserved[4];

        constexpr s64 GetSourceSize() const {
            return (static_cast<s64>(this->source_size_high) << 32) + this->source_size_low;
        }

        constexpr s64 GetDestinationSize() const {
            return (static_cast<s64>(this->destination_size_high) << 32) + this->destination_size_low;
        }

        constexpr void SetSourceSize(s64 size) {
            this->source_size_low  = size & 0xFFFFFFFFll;
            this->source_size_high = static_cast<u16>(size >> 32);
        }

        constexpr void SetDestinationSize(s64 size) {
            this->destination_size_low  = size & 0xFFFFFFFFll;
            this->destination_size_high = static_cast<u16>(size >> 32);
        }
    };

    struct SystemUpdateMetaExtendedDataHeader {
        u32 version;
        u32 firmware_variation_count;
    };

    struct DeltaMetaExtendedDataHeader {
        PatchId source_id;
        PatchId destination_id;
        u32 source_version;
        u32 destination_version;
        u16 fragment_set_count;
        u8 reserved[6];
    };

    struct PatchMetaExtendedDataHeader {
        u32 history_count;
        u32 delta_history_count;
        u32 delta_count;
        u32 fragment_set_count;
        u32 history_content_total_count;
        u32 delta_content_total_count;
        u8 reserved[4];
    };

    struct PatchHistoryHeader {
        ContentMetaKey key;
        Digest digest;
        u16 content_count;
        u8 reserved[2];
    };

    struct PatchDeltaHistory {
        PatchId source_id;
        PatchId destination_id;
        u32 source_version;
        u32 destination_version;
        u64 download_size;
        u8 reserved[4];
    };

    struct PatchDeltaHeader {
        DeltaMetaExtendedDataHeader delta;
        u16 content_count;
        u8 reserved[4];
    };

    template<typename MemberTypePointer, typename DataTypePointer>
    class PatchMetaExtendedDataReaderWriterBase {
        private:
            MemberTypePointer m_data;
            const size_t m_size;
        public:
            PatchMetaExtendedDataReaderWriterBase(MemberTypePointer d, size_t sz) : m_data(d), m_size(sz) { /* ... */ }
        protected:
            s32 CountFragmentSet(s32 delta_index) const {
                auto delta_header = this->GetPatchDeltaHeader(0);
                s32 count = 0;
                for (s32 i = 0; i < delta_index; i++) {
                    count += delta_header[i].delta.fragment_set_count;
                }
                return count;
            }

            s32 CountHistoryContent(s32 history_index) const {
                auto history_header = this->GetPatchHistoryHeader(0);
                s32 count = 0;
                for (s32 i = 0; i < history_index; i++) {
                    count += history_header[i].content_count;
                }
                return count;
            }

            s32 CountDeltaContent(s32 delta_index) const {
                auto delta_header = this->GetPatchDeltaHeader(0);
                s32 count = 0;
                for (s32 i = 0; i < delta_index; i++) {
                    count += delta_header[i].content_count;
                }
                return count;
            }

            s32 CountFragment(s32 index) const {
                auto fragment_set = this->GetFragmentSet(0, 0);
                s32 count = 0;
                for (s32 i = 0; i < index; i++) {
                    count += fragment_set[i].fragment_count;
                }
                return count;
            }

            DataTypePointer GetHeaderAddress() const {
                return reinterpret_cast<DataTypePointer>(m_data);
            }

            DataTypePointer GetPatchHistoryHeaderAddress(s32 index) const {
                auto header = this->GetHeader();
                AMS_ABORT_UNLESS(static_cast<u16>(index) < header->history_count);

                return this->GetHeaderAddress()
                       + sizeof(PatchMetaExtendedDataHeader)
                       + sizeof(PatchHistoryHeader)  * index;
            }

            DataTypePointer GetPatchDeltaHistoryAddress(s32 index) const {
                auto header = this->GetHeader();
                AMS_ABORT_UNLESS(static_cast<u16>(index) < header->delta_history_count);

                return this->GetHeaderAddress()
                       + sizeof(PatchMetaExtendedDataHeader)
                       + sizeof(PatchHistoryHeader)  * header->history_count
                       + sizeof(PatchDeltaHistory)   * index;
            }

            DataTypePointer GetPatchDeltaHeaderAddress(s32 index) const {
                auto header = this->GetHeader();
                AMS_ABORT_UNLESS(static_cast<u16>(index) < header->delta_count);

                return this->GetHeaderAddress()
                       + sizeof(PatchMetaExtendedDataHeader)
                       + sizeof(PatchHistoryHeader)  * header->history_count
                       + sizeof(PatchDeltaHistory)   * header->delta_history_count
                       + sizeof(PatchDeltaHeader)    * index;
            }

            DataTypePointer GetFragmentSetAddress(s32 delta_index, s32 fragment_set_index) const {
                auto header = this->GetHeader();
                AMS_ABORT_UNLESS(static_cast<u16>(delta_index) < header->delta_count);

                auto delta_header = this->GetPatchDeltaHeader(delta_index);
                AMS_ABORT_UNLESS(static_cast<u16>(fragment_set_index) < delta_header->delta.fragment_set_count);

                auto previous_fragment_set_count = this->CountFragmentSet(delta_index);

                return this->GetHeaderAddress()
                       + sizeof(PatchMetaExtendedDataHeader)
                       + sizeof(PatchHistoryHeader)  * header->history_count
                       + sizeof(PatchDeltaHistory)   * header->delta_history_count
                       + sizeof(PatchDeltaHeader)    * header->delta_count
                       + sizeof(FragmentSet)         * (previous_fragment_set_count + fragment_set_index);
            }

            DataTypePointer GetPatchHistoryContentInfoAddress(s32 history_index, s32 content_index) const {
                auto header = this->GetHeader();
                auto history_header = this->GetPatchHistoryHeader(history_index);
                AMS_ABORT_UNLESS(static_cast<u16>(content_index) < history_header->content_count);

                auto prev_history_count = this->CountHistoryContent(history_index);

                return this->GetHeaderAddress()
                       + sizeof(PatchMetaExtendedDataHeader)
                       + sizeof(PatchHistoryHeader)  * header->history_count
                       + sizeof(PatchDeltaHistory)   * header->delta_history_count
                       + sizeof(PatchDeltaHeader)    * header->delta_count
                       + sizeof(FragmentSet)         * header->fragment_set_count
                       + sizeof(ContentInfo)         * (prev_history_count + content_index);
            }

            DataTypePointer GetPatchDeltaPackagedContentInfoAddress(s32 delta_index, s32 content_index) const {
                auto header = this->GetHeader();
                auto delta_header = this->GetPatchDeltaHeader(delta_index);
                AMS_ABORT_UNLESS(static_cast<u16>(content_index) < delta_header->content_count);

                auto content_count = this->CountDeltaContent(delta_index);

                return this->GetHeaderAddress()
                       + sizeof(PatchMetaExtendedDataHeader)
                       + sizeof(PatchHistoryHeader)  * header->history_count
                       + sizeof(PatchDeltaHistory)   * header->delta_history_count
                       + sizeof(PatchDeltaHeader)    * header->delta_count
                       + sizeof(FragmentSet)         * header->fragment_set_count
                       + sizeof(ContentInfo)         * header->history_content_total_count
                       + sizeof(PackagedContentInfo) * (content_count + content_index);
            }

            DataTypePointer GetFragmentIndicatorAddress(s32 delta_index, s32 fragment_set_index, s32 index) const {
                auto header = this->GetHeader();
                auto fragment_set = this->GetFragmentSet(delta_index, fragment_set_index);
                AMS_ABORT_UNLESS(static_cast<u16>(index) < fragment_set->fragment_count);

                auto fragment_set_count = this->CountFragmentSet(delta_index);
                auto fragment_count     = this->CountFragment(fragment_set_count + fragment_set_index);

                return this->GetHeaderAddress()
                       + sizeof(PatchMetaExtendedDataHeader)
                       + sizeof(PatchHistoryHeader)  * header->history_count
                       + sizeof(PatchDeltaHistory)   * header->delta_history_count
                       + sizeof(PatchDeltaHeader)    * header->delta_count
                       + sizeof(FragmentSet)         * header->fragment_set_count
                       + sizeof(ContentInfo)         * header->history_content_total_count
                       + sizeof(PackagedContentInfo) * header->delta_content_total_count
                       + sizeof(FragmentIndicator)   * (fragment_count + index);
            }
        public:
            const PatchMetaExtendedDataHeader *GetHeader() const {
                return reinterpret_cast<const PatchMetaExtendedDataHeader *>(this->GetHeaderAddress());
            }

            const PatchHistoryHeader *GetPatchHistoryHeader(s32 index) const {
                return reinterpret_cast<const PatchHistoryHeader *>(this->GetPatchHistoryHeaderAddress(index));
            }

            const PatchDeltaHistory *GetPatchDeltaHistory(s32 index) const {
                return reinterpret_cast<const PatchDeltaHistory *>(this->GetPatchDeltaHistoryAddress(index));
            }

            const ContentInfo *GetPatchHistoryContentInfo(s32 history_index, s32 content_index) const {
                return reinterpret_cast<const ContentInfo *>(this->GetPatchHistoryContentInfoAddress(history_index, content_index));
            }

            const PatchDeltaHeader *GetPatchDeltaHeader(s32 index) const {
                return reinterpret_cast<const PatchDeltaHeader *>(this->GetPatchDeltaHeaderAddress(index));
            }

            const PackagedContentInfo *GetPatchDeltaPackagedContentInfo(s32 delta_index, s32 content_index) const {
                return reinterpret_cast<const PackagedContentInfo *>(this->GetPatchDeltaPackagedContentInfoAddress(delta_index, content_index));
            }

            const FragmentSet *GetFragmentSet(s32 delta_index, s32 fragment_set_index) const {
                return reinterpret_cast<const FragmentSet *>(this->GetFragmentSetAddress(delta_index, fragment_set_index));
            }

            const FragmentIndicator *GetFragmentIndicator(s32 delta_index, s32 fragment_set_index, s32 index) const {
                return reinterpret_cast<const FragmentIndicator *>(this->GetFragmentIndicatorAddress(delta_index, fragment_set_index, index));
            }

            const FragmentIndicator *FindFragmentIndicator(s32 delta_index, s32 fragment_set_index, s32 fragment_index) const {
                auto fragment_set = this->GetFragmentSet(delta_index, fragment_set_index);
                auto fragment     = this->GetFragmentIndicator(delta_index, fragment_set_index, 0);
                for (s32 i = 0; i < fragment_set->fragment_count; i++) {
                    if (fragment[i].fragment_index == fragment_index) {
                        return std::addressof(fragment[i]);
                    }
                }
                return nullptr;
            }
    };

    class PatchMetaExtendedDataReader : public PatchMetaExtendedDataReaderWriterBase<const void *, const u8 *> {
        public:
            PatchMetaExtendedDataReader(const void *data, size_t size) : PatchMetaExtendedDataReaderWriterBase(data, size) { /* ... */ }
    };

    class SystemUpdateMetaExtendedDataReaderWriterBase {
        private:
            void *m_data;
            const size_t m_size;
            bool m_is_header_valid;
        protected:
            constexpr SystemUpdateMetaExtendedDataReaderWriterBase(const void *d, size_t sz) : m_data(const_cast<void *>(d)), m_size(sz), m_is_header_valid(true) { /* ... */ }
            constexpr SystemUpdateMetaExtendedDataReaderWriterBase(void *d, size_t sz) : m_data(d), m_size(sz), m_is_header_valid(false) { /* ... */ }

            uintptr_t GetHeaderAddress() const {
                return reinterpret_cast<uintptr_t>(m_data);
            }

            uintptr_t GetFirmwareVariationIdStartAddress() const {
                return this->GetHeaderAddress() + sizeof(SystemUpdateMetaExtendedDataHeader);
            }

            uintptr_t GetFirmwareVariationIdAddress(size_t i) const {
                return this->GetFirmwareVariationIdStartAddress() + i * sizeof(FirmwareVariationId);
            }

            uintptr_t GetFirmwareVariationInfoStartAddress() const {
                return this->GetFirmwareVariationIdAddress(this->GetFirmwareVariationCount());
            }

            uintptr_t GetFirmwareVariationInfoAddress(size_t i) const {
                return this->GetFirmwareVariationInfoStartAddress() + i * sizeof(FirmwareVariationInfo);
            }

            uintptr_t GetContentMetaInfoStartAddress() const {
                return this->GetFirmwareVariationInfoAddress(this->GetFirmwareVariationCount());
            }

            uintptr_t GetContentMetaInfoAddress(size_t i) const {
                return this->GetContentMetaInfoStartAddress() + i * sizeof(ContentMetaInfo);
            }
        public:
            const SystemUpdateMetaExtendedDataHeader *GetHeader() const {
                AMS_ABORT_UNLESS(m_is_header_valid);
                return reinterpret_cast<const SystemUpdateMetaExtendedDataHeader *>(this->GetHeaderAddress());
            }

            size_t GetFirmwareVariationCount() const {
                return this->GetHeader()->firmware_variation_count;
            }

            const FirmwareVariationId *GetFirmwareVariationId(size_t i) const {
                AMS_ABORT_UNLESS(i < this->GetFirmwareVariationCount());

                return reinterpret_cast<FirmwareVariationId *>(this->GetFirmwareVariationIdAddress(i));
            }

            const FirmwareVariationInfo *GetFirmwareVariationInfo(size_t i) const {
                AMS_ABORT_UNLESS(i < this->GetFirmwareVariationCount());

                return reinterpret_cast<FirmwareVariationInfo *>(this->GetFirmwareVariationInfoAddress(i));
            }

            void GetContentMetaInfoList(Span<const ContentMetaInfo> *out_list, size_t i) const {
                size_t preceding_content_meta_count = 0;

                /* Count the number of preceding content metas. */
                for (size_t j = 0; j < i; j++) {
                    preceding_content_meta_count += this->GetFirmwareVariationInfo(j)->content_meta_count;
                }

                /* Output the list. */
                *out_list = Span<const ContentMetaInfo>(reinterpret_cast<const ContentMetaInfo *>(this->GetContentMetaInfoAddress(preceding_content_meta_count)), this->GetFirmwareVariationInfo(i)->content_meta_count);
            }
    };

    class SystemUpdateMetaExtendedDataReader : public SystemUpdateMetaExtendedDataReaderWriterBase {
        public:
            constexpr SystemUpdateMetaExtendedDataReader(const void *data, size_t size) : SystemUpdateMetaExtendedDataReaderWriterBase(data, size) { /* ... */ }
    };

    template<typename T>
    class ReadableStructPin;

    class AccessorBase {
        public:
            template<typename T>
            class PinBase {
                private:
                    AccessorBase *m_accessor;
                    u64 m_pin_id;
                    T *m_data;
                    size_t m_size;
                public:
                    PinBase() : m_accessor(nullptr), m_data(nullptr), m_size(0) {
                        /* ... */
                    }

                    PinBase(const PinBase &) = delete;
                    PinBase &operator=(const PinBase &) = delete;

                    PinBase(PinBase &&rhs) : m_accessor(rhs.m_accessor), m_pin_id(rhs.m_pin_id), m_data(rhs.m_data), m_size(rhs.m_size) {
                        rhs.m_accessor = nullptr;
                    }

                    PinBase &operator=(PinBase &&rhs) {
                        m_accessor     = rhs.m_accessor;
                        m_pin_id       = rhs.m_pin_id;
                        m_data         = rhs.m_data;
                        m_size         = rhs.m_size;
                        rhs.m_accessor = nullptr;

                        return *this;
                    }

                    virtual ~PinBase() {
                        this->Reset();
                    }
                public:
                    void Reset() {
                        if (m_accessor != nullptr) {
                            m_accessor->ReleasePin(m_pin_id);
                            m_accessor = nullptr;
                        }
                    }

                    void Reset(AccessorBase *accessor, u64 pin_id, void *data, size_t size) {
                        AMS_ASSERT(data != nullptr || size == 0);

                        this->Reset();

                        m_accessor = accessor;
                        m_pin_id   = pin_id;
                        m_data     = reinterpret_cast<T *>(data);
                        m_size     = size;
                    }

                    T *GetData() const {
                        return m_data;
                    }

                    size_t GetDataSize() const {
                        return m_size;
                    }
            };
        private:
            IMapper *m_mapper;
        public:
            AccessorBase(IMapper *mapper) : m_mapper(mapper) {
                /* ... */
            }

            template<typename T>
            Result AcquireReadableStructPin(ReadableStructPin<T> *out, size_t offset) {
                /* Acquire mapped memory for the pin. */
                MappedMemory memory = {};
                R_TRY(m_mapper->GetMappedMemory(std::addressof(memory), offset, sizeof(T)));

                /* Mark the memory as in use. */
                R_RETURN(m_mapper->MarkUsing(memory.id));

                /* Setup the pin. */
                out->Reset(this, memory.id, memory.GetBuffer(offset, sizeof(T)), sizeof(T));
                R_SUCCEED();
            }

            Result ReleasePin(u64 id) {
                R_RETURN(m_mapper->UnmarkUsing(id));
            }

            template<typename T>
            Result ReadStruct(T *out, size_t offset) {
                /* Acquire mapped memory for the pin. */
                MappedMemory memory = {};
                R_TRY(m_mapper->GetMappedMemory(std::addressof(memory), offset, sizeof(T)));

                /* Mark the memory as in use. */
                R_RETURN(m_mapper->MarkUsing(memory.id));
                ON_SCOPE_EXIT { this->ReleasePin(memory.id); };

                /* Copy out the struct. */
                *out = *reinterpret_cast<const T *>(memory.GetBuffer(offset, sizeof(T)));
                R_SUCCEED();
            }
    };

    template<typename T>
    class ReadableStructPin final : public AccessorBase::PinBase<const u8> {
        public:
            using PinBase::PinBase;
            using PinBase::operator=;

            const T *Get() const {
                return reinterpret_cast<const T *>(this->GetData());
            }

            size_t GetSize() const {
                return this->GetDataSize();
            }

            const T &operator*() const { return *this->Get(); }
            const T *operator->() const { return this->Get(); }
    };

    class PatchMetaExtendedDataAccessor : public AccessorBase {
        private:
            struct CachedCount {
                s32 index;
                s32 count;
            };
        private:
            util::optional<CachedCount> m_cached_history_content_count    = util::nullopt;
            util::optional<CachedCount> m_cached_delta_content_count      = util::nullopt;
            util::optional<CachedCount> m_cached_fragment_set_count       = util::nullopt;
            util::optional<CachedCount> m_cached_fragment_indicator_count = util::nullopt;
            util::optional<PatchMetaExtendedDataHeader> m_header          = util::nullopt;
        public:
            using AccessorBase::AccessorBase;
        public:
            Result GetHeader(ReadableStructPin<PatchMetaExtendedDataHeader> *out) { R_RETURN(this->AcquireReadableStructPin(out, 0)); }
            Result GetHeader(PatchMetaExtendedDataHeader *out) { R_RETURN(this->template ReadStruct<PatchMetaExtendedDataHeader>(out, 0)); }

            Result GetHistoryHeader(ReadableStructPin<PatchHistoryHeader> *out, s32 index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= index && static_cast<u32>(index) < m_header->history_count, ncm::ResultInvalidOffset());

                /* Get the header. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * index;
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result GetPatchDeltaHistory(ReadableStructPin<PatchDeltaHistory> *out, s32 index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= index && static_cast<u32>(index) < m_header->delta_history_count, ncm::ResultInvalidOffset());

                /* Get the history. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * index;
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result GetPatchDeltaHeader(ReadableStructPin<PatchDeltaHeader> *out, s32 index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= index && static_cast<u32>(index) < m_header->delta_count, ncm::ResultInvalidOffset());

                /* Get the header. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * m_header->delta_history_count + sizeof(PatchDeltaHeader) * index;
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result GetFragmentSet(ReadableStructPin<FragmentSet> *out, s32 delta_index, s32 fragment_set_index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= delta_index && static_cast<u32>(delta_index) < m_header->delta_count, ncm::ResultInvalidOffset());

                /* Get the previous fragment set count. */
                s32 previous_fragment_set_count = 0;
                R_TRY(this->CountFragmentSet(std::addressof(previous_fragment_set_count), delta_index));

                /* Get the set. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * m_header->delta_history_count + sizeof(PatchDeltaHeader) * m_header->delta_count + sizeof(FragmentSet) * (previous_fragment_set_count + fragment_set_index);
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result GetFragmentSetDirectly(ReadableStructPin<FragmentSet> *out, s32 fragment_set_direct_index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= fragment_set_direct_index && static_cast<u32>(fragment_set_direct_index) < m_header->fragment_set_count, ncm::ResultInvalidOffset());

                /* Get the set. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * m_header->delta_history_count + sizeof(PatchDeltaHeader) * m_header->delta_count + sizeof(FragmentSet) * (fragment_set_direct_index);
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result GetPatchHistoryContentInfo(ReadableStructPin<ContentInfo> *out, s32 history_index, s32 content_index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= history_index && static_cast<u32>(history_index) < m_header->history_count, ncm::ResultInvalidOffset());

                /* Determine the true history content index. */
                s32 prev_history_count = 0;
                R_TRY(this->CountHistoryContentInfo(std::addressof(prev_history_count), history_index));

                /* Adjust and check the content index. */
                content_index += prev_history_count;
                R_UNLESS(0 <= content_index && static_cast<u32>(content_index) < m_header->history_content_total_count, ncm::ResultInvalidOffset());

                /* Get the info. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * m_header->delta_history_count + sizeof(PatchDeltaHeader) * m_header->delta_count + sizeof(FragmentSet) * m_header->fragment_set_count + sizeof(ContentInfo) * content_index;
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result GetPatchDeltaContentInfo(ReadableStructPin<PackagedContentInfo> *out, s32 delta_index, s32 content_index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= delta_index && static_cast<u32>(delta_index) < m_header->delta_count, ncm::ResultInvalidOffset());

                /* Determine the true delta content index. */
                s32 prev_delta_count = 0;
                R_TRY(this->CountDeltaContentInfo(std::addressof(prev_delta_count), delta_index));

                /* Adjust and check the content index. */
                content_index += prev_delta_count;
                R_UNLESS(0 <= content_index && static_cast<u32>(content_index) < m_header->delta_content_total_count, ncm::ResultInvalidOffset());

                /* Get the info. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * m_header->delta_history_count + sizeof(PatchDeltaHeader) * m_header->delta_count + sizeof(FragmentSet) * m_header->fragment_set_count + sizeof(ContentInfo) * m_header->history_content_total_count + sizeof(PackagedContentInfo) * content_index;
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result GetFragmentIndicator(ReadableStructPin<FragmentIndicator> *out, s32 delta_index, s32 fragment_set_index, s32 index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= delta_index && static_cast<u32>(delta_index) < m_header->delta_count, ncm::ResultInvalidOffset());

                /* Get the previous fragment set count. */
                s32 previous_fragment_set_count = 0;
                R_TRY(this->CountFragmentSet(std::addressof(previous_fragment_set_count), delta_index));

                /* Get the previous fragment indicator count. */
                s32 previous_fragment_count = 0;
                R_TRY(this->CountFragmentIndicator(std::addressof(previous_fragment_count), previous_fragment_count + fragment_set_index));

                /* Get the info. */
                const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * m_header->delta_history_count + sizeof(PatchDeltaHeader) * m_header->delta_count + sizeof(FragmentSet) * m_header->fragment_set_count + sizeof(ContentInfo) * m_header->history_content_total_count + sizeof(PackagedContentInfo) * m_header->delta_content_total_count + sizeof(FragmentIndicator) * (previous_fragment_count + index);
                R_RETURN(this->AcquireReadableStructPin(out, offset));
            }

            Result FindFragmentIndicator(ReadableStructPin<FragmentIndicator> *out, s32 delta_index, s32 fragment_set_index, s32 fragment_index) {
                /* Ensure we have our header. */
                R_TRY(this->EnsureHeader());

                /* Check that the index is valid. */
                R_UNLESS(0 <= delta_index && static_cast<u32>(delta_index) < m_header->delta_count, ncm::ResultInvalidOffset());

                /* Get the fragment count. */
                s32 fragment_count = 0;
                {
                    ReadableStructPin<FragmentSet> set;
                    R_TRY(this->GetFragmentSet(std::addressof(set), delta_index, fragment_set_index));

                    fragment_count = set->fragment_count;
                }

                /* Get the previous fragment set count. */
                s32 previous_fragment_set_count = 0;
                R_TRY(this->CountFragmentSet(std::addressof(previous_fragment_set_count), delta_index));

                /* Get the previous fragment indicator count. */
                s32 previous_fragment_count = 0;
                R_TRY(this->CountFragmentIndicator(std::addressof(previous_fragment_count), previous_fragment_count + fragment_set_index));

                /* Look for a correct indicator. */
                for (auto i = 0; i < fragment_count; ++i) {
                    /* Get the current info. */
                    ReadableStructPin<FragmentIndicator> indicator;
                    const size_t offset = sizeof(PatchHistoryHeader) + sizeof(PatchHistoryHeader) * m_header->history_count + sizeof(PatchDeltaHistory) * m_header->delta_history_count + sizeof(PatchDeltaHeader) * m_header->delta_count + sizeof(FragmentSet) * m_header->fragment_set_count + sizeof(ContentInfo) * m_header->history_content_total_count + sizeof(PackagedContentInfo) * m_header->delta_content_total_count + sizeof(FragmentIndicator) * (previous_fragment_count + i);
                    R_TRY(this->AcquireReadableStructPin(std::addressof(indicator), offset));

                    /* If it matches, return it. */
                    if (indicator->fragment_index == fragment_index) {
                        *out = std::move(indicator);
                        R_SUCCEED();
                    }
                }

                /* We didn't find an indicator. */
                R_THROW(ncm::ResultFragmentIndicatorNotFound());
            }

            Result GetHistoryHeader(PatchHistoryHeader *out, s32 index) {
                /* Get the pin. */
                ReadableStructPin<PatchHistoryHeader> pin;
                R_TRY(this->GetHistoryHeader(std::addressof(pin), index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result GetPatchDeltaHistory(PatchDeltaHistory *out, s32 index) {
                /* Get the pin. */
                ReadableStructPin<PatchDeltaHistory> pin;
                R_TRY(this->GetPatchDeltaHistory(std::addressof(pin), index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result GetPatchDeltaHeader(PatchDeltaHeader *out, s32 index) {
                /* Get the pin. */
                ReadableStructPin<PatchDeltaHeader> pin;
                R_TRY(this->GetPatchDeltaHeader(std::addressof(pin), index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result GetFragmentSet(FragmentSet *out, s32 delta_index, s32 fragment_set_index) {
                /* Get the pin. */
                ReadableStructPin<FragmentSet> pin;
                R_TRY(this->GetFragmentSet(std::addressof(pin), delta_index, fragment_set_index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result GetPatchHistoryContentInfo(ContentInfo *out, s32 history_index, s32 content_index) {
                /* Get the header. */
                ReadableStructPin<ContentInfo> pin;
                R_TRY(this->GetPatchHistoryContentInfo(std::addressof(pin), history_index, content_index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result GetPatchDeltaContentInfo(PackagedContentInfo *out, s32 delta_index, s32 content_index) {
                /* Get the header. */
                ReadableStructPin<PackagedContentInfo> pin;
                R_TRY(this->GetPatchDeltaContentInfo(std::addressof(pin), delta_index, content_index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result GetFragmentIndicator(FragmentIndicator *out, s32 delta_index, s32 fragment_set_index, s32 index) {
                /* Get the header. */
                ReadableStructPin<FragmentIndicator> pin;
                R_TRY(this->GetFragmentIndicator(std::addressof(pin), delta_index, fragment_set_index, index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result FindFragmentIndicator(FragmentIndicator *out, s32 delta_index, s32 fragment_set_index, s32 fragment_index) {
                /* Get the header. */
                ReadableStructPin<FragmentIndicator> pin;
                R_TRY(this->FindFragmentIndicator(std::addressof(pin), delta_index, fragment_set_index, fragment_index));

                /* Copy it out. */
                *out = *pin;
                R_SUCCEED();
            }

            Result CountHistoryContentInfo(s32 *out, s32 index) {
                R_RETURN(this->CountImpl(out, index, m_cached_history_content_count, [&](s32 *out, s32 i) -> Result {
                    /* Get the history header. */
                    ReadableStructPin<ncm::PatchHistoryHeader> header;
                    R_TRY(this->GetHistoryHeader(std::addressof(header), i));

                    /* Set the content count. */
                    *out = header->content_count;
                    R_SUCCEED();
                }));
            }

            Result CountDeltaContentInfo(s32 *out, s32 index) {
                R_RETURN(this->CountImpl(out, index, m_cached_delta_content_count, [&](s32 *out, s32 i) -> Result {
                    /* Get the history header. */
                    ReadableStructPin<ncm::PatchDeltaHeader> header;
                    R_TRY(this->GetPatchDeltaHeader(std::addressof(header), i));

                    /* Set the content count. */
                    *out = header->content_count;
                    R_SUCCEED();
                }));
            }

            Result CountFragmentSet(s32 *out, s32 index) {
                R_RETURN(this->CountImpl(out, index, m_cached_fragment_set_count, [&](s32 *out, s32 i) -> Result {
                    /* Get the history header. */
                    ReadableStructPin<ncm::PatchDeltaHeader> header;
                    R_TRY(this->GetPatchDeltaHeader(std::addressof(header), i));

                    /* Set the fragment set count. */
                    *out = header->delta.fragment_set_count;
                    R_SUCCEED();
                }));
            }

            Result CountFragmentIndicator(s32 *out, s32 index) {
                R_RETURN(this->CountImpl(out, index, m_cached_fragment_indicator_count, [&](s32 *out, s32 i) -> Result {
                    /* Get the history header. */
                    ReadableStructPin<ncm::FragmentSet> set;
                    R_TRY(this->GetFragmentSetDirectly(std::addressof(set), i));

                    /* Set the indicator count. */
                    *out = set->fragment_count;
                    R_SUCCEED();
                }));
            }
        private:
            Result CountImpl(s32 *out, s32 index, util::optional<CachedCount> &cache, auto get_count_impl) const {
                /* Ensure the value is cached. */
                if (!(cache.has_value() && cache->index == index)) {
                    /* Determine the count. */
                    CachedCount calc = { .index = index, .count = 0 };
                    for (auto i = 0; i < index; ++i) {
                        s32 cur_count = 0;
                        R_TRY(get_count_impl(std::addressof(cur_count), i));

                        calc.count += cur_count;
                    }

                    /* Cache the count. */
                    cache = calc;
                }

                /* Set the output count. */
                *out = cache->count;
                R_SUCCEED();
            }
        private:
            Result EnsureHeader() {
                /* If we have our header, we're good. */
                R_SUCCEED_IF(m_header.has_value());

                /* Get our header. */
                PatchMetaExtendedDataHeader header{};
                R_TRY(this->GetHeader(std::addressof(header)));

                /* Set our header. */
                m_header.emplace(header);
                R_SUCCEED();
            }


    };

}
