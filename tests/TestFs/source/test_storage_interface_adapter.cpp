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

        class TestBatchReadStorage : public fs::MemoryStorage {
            public:
                int read_count = 0;
                int corrupt_reads = 0;
            public:
                using MemoryStorage::MemoryStorage;

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    ++read_count;
                    if (corrupt_reads > 0) {
                        --corrupt_reads;
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
        auto Reset = [&] {
            std::memset(output, 0xCC, sizeof(output));
            storage->read_count = 0;
            storage->corrupt_reads = 0;
        };

        /* Exercise partial and full batches, non-contiguous offsets and zero-sized reads. */
        for (size_t count = 0; count <= 7; ++count) {
            Reset();
            R_ABORT_UNLESS(ReadBatch(count));
            AMS_ABORT_UNLESS(storage->read_count == static_cast<int>(count));
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
        AMS_ABORT_UNLESS(storage->read_count == 0);

        /* A negative offset is rejected and subsequent entries are not read. */
        Reset();
        offsets[1] = -1;
        AMS_ABORT_UNLESS(fs::ResultInvalidOffset::Includes(ReadBatch(7)));
        AMS_ABORT_UNLESS(storage->read_count == 1);
        AMS_ABORT_UNLESS(output[0][0] == data[offsets[0]]);
        AMS_ABORT_UNLESS(output[1][0] == 0xCC && output[3][0] == 0xCC);
        offsets[1] = 3;

        /* Preserve storage errors and stop before subsequent reads. */
        Reset();
        offsets[1] = sizeof(data);
        AMS_ABORT_UNLESS(fs::ResultOutOfRange::Includes(ReadBatch(7)));
        AMS_ABORT_UNLESS(storage->read_count == 2);
        AMS_ABORT_UNLESS(output[1][0] == 0xCC && output[3][0] == 0xCC);
        offsets[1] = 3;

        /* Offset/size overflow must fail through the normal read path. */
        Reset();
        offsets[0] = std::numeric_limits<s64>::max();
        AMS_ABORT_UNLESS(fs::ResultOutOfRange::Includes(ReadBatch(1)));
        offsets[0] = 18;
        sizes[0] = static_cast<size_t>(std::numeric_limits<s64>::max()) + 1;
        AMS_ABORT_UNLESS(fs::ResultInvalidSize::Includes(ReadBatch(1)));
        AMS_ABORT_UNLESS(storage->read_count == 1);
        sizes[0] = 3;

        /* Batch reads retain ordinary reads' bounded corruption retry behavior. */
        Reset();
        storage->corrupt_reads = 1;
        R_ABORT_UNLESS(ReadBatch(7));
        AMS_ABORT_UNLESS(storage->read_count == 8);
        AMS_ABORT_UNLESS(output[0][0] == data[offsets[0]]);
        AMS_ABORT_UNLESS(output[6][0] == data[offsets[6]]);

        Reset();
        storage->corrupt_reads = 2;
        AMS_ABORT_UNLESS(fs::ResultHostFileDataCorrupted::Includes(ReadBatch(7)));
        AMS_ABORT_UNLESS(storage->read_count == 2);
        AMS_ABORT_UNLESS(output[0][0] == 0xCC && output[1][0] == 0xCC);

        /* The inherited single-read API remains usable on the same storage. */
        Reset();
        R_ABORT_UNLESS(adapter.Read(3, {output[0], sizeof(output[0])}, 9));
        AMS_ABORT_UNLESS(std::memcmp(output[0], data + 3, 9) == 0);
        AMS_ABORT_UNLESS(output[0][9] == 0xCC);
        s64 size;
        R_ABORT_UNLESS(adapter.GetSize(std::addressof(size)));
        AMS_ABORT_UNLESS(size == static_cast<s64>(sizeof(data)));

        /* The service object exposes both the inherited commands and BatchRead. */
        auto service = sf::CreateSharedObjectEmplaced<fssrv::sf::IStorageForBatchRead, fssrv::impl::StorageInterfaceAdapter>(storage);
        Reset();
        R_ABORT_UNLESS(service->BatchRead({output[0], 3}, {}, {}, {}, {}, {}, {}, {offsets, 1}));
        AMS_ABORT_UNLESS(std::memcmp(output[0], data + offsets[0], 3) == 0);
        R_ABORT_UNLESS(service->Read(3, {output[1], 9}, 9));
        AMS_ABORT_UNLESS(std::memcmp(output[1], data + 3, 9) == 0);
        R_ABORT_UNLESS(service->GetSize(std::addressof(size)));
        AMS_ABORT_UNLESS(size == static_cast<s64>(sizeof(data)));
    }

}
