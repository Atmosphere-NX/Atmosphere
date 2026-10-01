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
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

namespace ams {

    namespace {

        template<typename T> struct BatchReadCommandMeta;
        template<typename... Args>
        struct BatchReadCommandMeta<std::tuple<Args...>> {
            static constexpr auto BufferAttributes = std::array<u32, sizeof...(Args)>{sf::BufferAttributes<std::remove_cvref_t<Args>>...};
        };
        using BatchReadMeta = BatchReadCommandMeta<fssrv::sf::IStorageForBatchRead::BatchReadArgumentsType>;

        /* Firmware 23 uses seven non-secure output map aliases followed by an input offset array. */
        static_assert(BatchReadMeta::BufferAttributes == std::array<u32, 8>{0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x05});

        /* Verify the server registers the complete firmware 23 command set. */
        static_assert([] {
            constexpr auto &entries = sf::cmif::ServiceDispatchTraits<fssrv::sf::IStorageForBatchRead>::DispatchTable.GetEntries();
            constexpr u32 command_ids[] = {0, 1, 2, 3, 4, 5, 10};
            if (entries.size() != util::size(command_ids)) {
                return false;
            }
            for (size_t i = 0; i < entries.size(); ++i) {
                if (!entries[i].Matches(command_ids[i], hos::Version_23_0_0) || entries[i].MatchesVersion(hos::Version_22_0_0)) {
                    return false;
                }
            }
            return true;
        }());

        /* Ordinary IStorage must continue to expose only commands 0-5. */
        static_assert([] {
            constexpr auto &entries = sf::cmif::ServiceDispatchTraits<fssrv::sf::IStorage>::DispatchTable.GetEntries();
            if (entries.size() != 6) {
                return false;
            }
            for (size_t i = 0; i < entries.size(); ++i) {
                if (!entries[i].Matches(i, hos::Version_23_0_0)) {
                    return false;
                }
            }
            return true;
        }());

        #if defined(ATMOSPHERE_OS_HORIZON)
        template<typename T> struct StorageCommandMeta;

        template<typename... Args>
        struct StorageCommandMeta<std::tuple<Args...>> : sf::impl::CommandMetaInfo<Args...> { };

        template<typename Args, size_t N>
        consteval bool HasCommandAbi(size_t in_size, size_t out_size, const std::array<u32, N> &attributes) {
            using Meta = StorageCommandMeta<Args>;
            return Meta::InDataSize == in_size && Meta::OutDataSize == out_size && Meta::BufferAttributes == attributes;
        }

        /* Check the Horizon serializer against the firmware interface, including alignment. */
        using BatchStorage = fssrv::sf::IStorageForBatchRead;
        static_assert(HasCommandAbi<BatchStorage::ReadArgumentsType>        (0x10, 0x00, std::array<u32, 1>{0x46}));
        static_assert(HasCommandAbi<BatchStorage::WriteArgumentsType>       (0x10, 0x00, std::array<u32, 1>{0x45}));
        static_assert(HasCommandAbi<BatchStorage::FlushArgumentsType>       (0x00, 0x00, std::array<u32, 0>{}));
        static_assert(HasCommandAbi<BatchStorage::SetSizeArgumentsType>     (0x08, 0x00, std::array<u32, 0>{}));
        static_assert(HasCommandAbi<BatchStorage::GetSizeArgumentsType>     (0x00, 0x08, std::array<u32, 0>{}));
        static_assert(HasCommandAbi<BatchStorage::OperateRangeArgumentsType>(0x18, 0x40, std::array<u32, 0>{}));
        static_assert(HasCommandAbi<BatchStorage::BatchReadArgumentsType>   (0x00, 0x00, std::array<u32, 8>{0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x05}));
        #endif

        class TestBatchReadStorage : public fs::MemoryStorage {
            private:
                int m_read_count = 0;
                int m_corrupt_reads = 0;
            public:
                using MemoryStorage::MemoryStorage;

                int GetReadCount() const { return m_read_count; }
                void SetCorruptReads(int count) { m_corrupt_reads = count; }
                void Reset() {
                    m_read_count = 0;
                    m_corrupt_reads = 0;
                }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    ++m_read_count;
                    if (m_corrupt_reads > 0) {
                        --m_corrupt_reads;
                        R_THROW(fs::ResultHostFileDataCorrupted());
                    }
                    R_RETURN(MemoryStorage::Read(offset, buffer, size));
                }
        };

    }

    void DoStorageInterfaceAdapterTests() {
        u8 data[64];
        for (size_t i = 0; i < sizeof(data); ++i) {
            data[i] = static_cast<u8>(i);
        }

        auto storage = std::make_shared<TestBatchReadStorage>(data, sizeof(data));
        fssrv::impl::StorageInterfaceAdapter adapter{storage};
        u8 output[7][16];
        s64 offsets[8] = {18, 3, 49, 0, 31, 12, 7, 0};
        size_t sizes[7] = {3, 9, 0, 16, 7, 5, 2};
        auto ReadBatch = [&] (size_t count) {
            return adapter.BatchRead({output[0], sizes[0]}, {output[1], sizes[1]}, {output[2], sizes[2]}, {output[3], sizes[3]}, {output[4], sizes[4]}, {output[5], sizes[5]}, {output[6], sizes[6]}, {offsets, count});
        };
        auto Reset = [&] () {
            std::memset(output, 0xCC, sizeof(output));
            storage->Reset();
        };

        /* Exercise partial and full batches, non-contiguous offsets and zero-sized reads. */
        for (size_t count = 0; count <= 7; ++count) {
            Reset();
            R_ABORT_UNLESS(ReadBatch(count));
            AMS_ABORT_UNLESS(storage->GetReadCount() == static_cast<int>(count));
            for (size_t i = 0; i < 7; ++i) {
                for (size_t j = 0; j < sizeof(output[i]); ++j) {
                    const u8 expected = i < count && j < sizes[i] ? data[offsets[i] + j] : 0xCC;
                    AMS_ABORT_UNLESS(output[i][j] == expected);
                }
            }
        }

        /* Reject an oversized batch before reading any storage or indexing an output buffer. */
        Reset();
        AMS_ABORT_UNLESS(fs::ResultInvalidSize::Includes(ReadBatch(8)));
        AMS_ABORT_UNLESS(storage->GetReadCount() == 0);

        /* A negative offset is rejected and subsequent entries are not read. */
        Reset();
        offsets[1] = -1;
        AMS_ABORT_UNLESS(fs::ResultInvalidOffset::Includes(ReadBatch(7)));
        AMS_ABORT_UNLESS(storage->GetReadCount() == 1);
        AMS_ABORT_UNLESS(output[0][0] == data[offsets[0]]);
        AMS_ABORT_UNLESS(output[1][0] == 0xCC && output[3][0] == 0xCC);
        offsets[1] = 3;

        /* Preserve storage errors and stop before subsequent reads. */
        Reset();
        offsets[1] = sizeof(data);
        AMS_ABORT_UNLESS(fs::ResultOutOfRange::Includes(ReadBatch(7)));
        AMS_ABORT_UNLESS(storage->GetReadCount() == 2);
        AMS_ABORT_UNLESS(output[1][0] == 0xCC && output[3][0] == 0xCC);
        offsets[1] = 3;

        /* Offset/size overflow must fail through the normal read path. */
        Reset();
        offsets[0] = std::numeric_limits<s64>::max();
        AMS_ABORT_UNLESS(fs::ResultOutOfRange::Includes(ReadBatch(1)));
        offsets[0] = 18;
        sizes[0] = static_cast<size_t>(std::numeric_limits<s64>::max()) + 1;
        AMS_ABORT_UNLESS(fs::ResultInvalidSize::Includes(ReadBatch(1)));
        AMS_ABORT_UNLESS(storage->GetReadCount() == 1);
        sizes[0] = 3;

        /* Batch reads retain ordinary reads' bounded corruption retry behavior. */
        Reset();
        storage->SetCorruptReads(1);
        R_ABORT_UNLESS(ReadBatch(7));
        AMS_ABORT_UNLESS(storage->GetReadCount() == 8);
        AMS_ABORT_UNLESS(output[0][0] == data[offsets[0]]);
        AMS_ABORT_UNLESS(output[6][0] == data[offsets[6]]);

        Reset();
        storage->SetCorruptReads(2);
        AMS_ABORT_UNLESS(fs::ResultHostFileDataCorrupted::Includes(ReadBatch(7)));
        AMS_ABORT_UNLESS(storage->GetReadCount() == 2);
        AMS_ABORT_UNLESS(output[0][0] == 0xCC && output[1][0] == 0xCC);

        /* The single-read API remains usable on the same storage. */
        Reset();
        R_ABORT_UNLESS(adapter.Read(3, {output[0], sizeof(output[0])}, 9));
        AMS_ABORT_UNLESS(std::memcmp(output[0], data + 3, 9) == 0);
        AMS_ABORT_UNLESS(output[0][9] == 0xCC);
        s64 size;
        R_ABORT_UNLESS(adapter.GetSize(std::addressof(size)));
        AMS_ABORT_UNLESS(size == static_cast<s64>(sizeof(data)));

        /* Exercise all seven commands through the service object. */
        auto service = sf::CreateSharedObjectEmplaced<fssrv::sf::IStorageForBatchRead, fssrv::impl::StorageInterfaceAdapter>(storage);
        Reset();
        R_ABORT_UNLESS(service->BatchRead({output[0], 3}, {}, {}, {}, {}, {}, {}, {offsets, 1}));
        AMS_ABORT_UNLESS(std::memcmp(output[0], data + offsets[0], 3) == 0);
        R_ABORT_UNLESS(service->Read(3, {output[1], 9}, 9));
        AMS_ABORT_UNLESS(std::memcmp(output[1], data + 3, 9) == 0);
        R_ABORT_UNLESS(service->GetSize(std::addressof(size)));
        AMS_ABORT_UNLESS(size == static_cast<s64>(sizeof(data)));

        const u8 replacement[] = {0xA5, 0x5A, 0xFF};
        R_ABORT_UNLESS(service->Write(20, {replacement, sizeof(replacement)}, sizeof(replacement)));
        R_ABORT_UNLESS(service->Read(20, {output[0], sizeof(replacement)}, sizeof(replacement)));
        AMS_ABORT_UNLESS(std::memcmp(output[0], replacement, sizeof(replacement)) == 0);
        R_ABORT_UNLESS(service->Flush());
        AMS_ABORT_UNLESS(fs::ResultInvalidSize::Includes(service->SetSize(-1)));
        AMS_ABORT_UNLESS(fs::ResultUnsupportedSetSizeForMemoryStorage::Includes(service->SetSize(sizeof(data))));

        fs::StorageQueryRangeInfo range_info;
        std::memset(std::addressof(range_info), 0xCC, sizeof(range_info));
        R_ABORT_UNLESS(service->OperateRange(std::addressof(range_info), static_cast<s32>(fs::OperationId::QueryRange), 0, sizeof(data)));
        const fs::StorageQueryRangeInfo empty_range_info{};
        AMS_ABORT_UNLESS(std::memcmp(std::addressof(range_info), std::addressof(empty_range_info), sizeof(range_info)) == 0);
        R_ABORT_UNLESS(service->OperateRange(std::addressof(range_info), static_cast<s32>(fs::OperationId::Invalidate), 0, sizeof(data)));

        /* Read-only backing storage must reject writes and resizing through the batch interface. */
        auto read_only_storage = std::make_shared<fs::ReadOnlyStorageAdapter>(storage);
        auto read_only_service = sf::CreateSharedObjectEmplaced<fssrv::sf::IStorageForBatchRead, fssrv::impl::StorageInterfaceAdapter>(read_only_storage);
        AMS_ABORT_UNLESS(fs::ResultUnsupportedOperation::Includes(read_only_service->Write(0, {replacement, sizeof(replacement)}, sizeof(replacement))));
        AMS_ABORT_UNLESS(fs::ResultUnsupportedOperation::Includes(read_only_service->SetSize(sizeof(data))));
        R_ABORT_UNLESS(read_only_service->Read(0, {output[0], 3}, 3));
        AMS_ABORT_UNLESS(output[0][0] == 0 && output[0][1] == 1 && output[0][2] == 2);
        R_ABORT_UNLESS(read_only_service->BatchRead({output[0], 3}, {}, {}, {}, {}, {}, {}, {offsets, 1}));
        AMS_ABORT_UNLESS(std::memcmp(output[0], data + offsets[0], 3) == 0);
    }

}
