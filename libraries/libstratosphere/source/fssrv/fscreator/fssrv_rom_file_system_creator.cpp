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
#include <stratosphere.hpp>

namespace ams::fssrv::fscreator {

    namespace {

        class RomFileSystemWithBuffer : public ::ams::fssystem::RomFsFileSystem {
            private:
                void *meta_cache_buffer;
                size_t meta_cache_buffer_size;
                MemoryResource *allocator;
            public:
                explicit RomFileSystemWithBuffer(MemoryResource *mr) : meta_cache_buffer(nullptr), allocator(mr) { /* ... */ }

                ~RomFileSystemWithBuffer() {
                    if (this->meta_cache_buffer != nullptr) {
                        this->allocator->Deallocate(this->meta_cache_buffer, this->meta_cache_buffer_size);
                    }
                }

                Result Initialize(std::shared_ptr<fs::IStorage> storage) {
                    /* Check if the buffer is eligible for cache. */
                    size_t buffer_size = 0;
                    if (R_FAILED(RomFsFileSystem::GetRequiredWorkingMemorySize(std::addressof(buffer_size), storage.get())) || buffer_size == 0 || buffer_size >= 128_KB) {
                        return RomFsFileSystem::Initialize(std::move(storage), nullptr, 0, false);
                    }

                    /* Allocate a buffer. */
                    this->meta_cache_buffer = this->allocator->Allocate(buffer_size);
                    if (this->meta_cache_buffer == nullptr) {
                        return RomFsFileSystem::Initialize(std::move(storage), nullptr, 0, false);
                    }

                    /* Initialize with cache buffer. */
                    this->meta_cache_buffer_size = buffer_size;
                    return RomFsFileSystem::Initialize(std::move(storage), this->meta_cache_buffer, this->meta_cache_buffer_size, true);
                }
        };

    }

    Result RomFileSystemCreator::Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::IStorage> storage) {
        /* Allocate a filesystem. */
        std::shared_ptr fs = fssystem::AllocateShared<RomFileSystemWithBuffer>(this->allocator);
        R_UNLESS(fs != nullptr, fs::ResultAllocationFailureInRomFileSystemCreatorA());

        /* Initialize the filesystem. */
        R_TRY(fs->Initialize(std::move(storage)));

        /* Set the output. */
        *out = std::move(fs);
        return ResultSuccess();
    }

}
