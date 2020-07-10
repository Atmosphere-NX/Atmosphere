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
#include <stratosphere/ncm/ncm_content_info.hpp>
#include <stratosphere/ncm/ncm_content_meta_id.hpp>
#include <stratosphere/ncm/ncm_content_meta.hpp>
#include <stratosphere/ncm/ncm_firmware_variation.hpp>

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
            MemberTypePointer data;
            const size_t size;
        public:
            PatchMetaExtendedDataReaderWriterBase(MemberTypePointer d, size_t sz) : data(d), size(sz) { /* ... */ }
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
                return reinterpret_cast<DataTypePointer>(this->data);
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
                return reinterpret_cast<const FragmentSet *>(this->GetFragmentSetIndex(delta_index, fragment_set_index));
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
            void *data;
            const size_t size;
            bool is_header_valid;
        protected:
            constexpr SystemUpdateMetaExtendedDataReaderWriterBase(const void *d, size_t sz) : data(const_cast<void *>(d)), size(sz), is_header_valid(true) { /* ... */ }
            constexpr SystemUpdateMetaExtendedDataReaderWriterBase(void *d, size_t sz) : data(d), size(sz), is_header_valid(false) { /* ... */ }

            uintptr_t GetHeaderAddress() const {
                return reinterpret_cast<uintptr_t>(this->data);
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
                AMS_ABORT_UNLESS(this->is_header_valid);
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

}
