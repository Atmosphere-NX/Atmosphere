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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fs::impl {

    template<typename StorageInterface>
    class StorageServiceObjectAdapter : public ::ams::fs::impl::Newable, public ::ams::fs::IStorage {
        NON_COPYABLE(StorageServiceObjectAdapter);
        NON_MOVEABLE(StorageServiceObjectAdapter);
        private:
            sf::SharedPointer<StorageInterface> m_x;
        public:
            explicit StorageServiceObjectAdapter(sf::SharedPointer<StorageInterface> &&o) : m_x(o) { /* ... */}
            virtual ~StorageServiceObjectAdapter() { /* ... */ }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override final {
                R_RETURN(m_x->Read(offset, sf::OutNonSecureBuffer(buffer, size), static_cast<s64>(size)));
            }

            virtual Result GetSize(s64 *out) override final {
                R_RETURN(m_x->GetSize(out));
            }

            virtual Result Flush() override final {
                R_RETURN(m_x->Flush());
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override final {
                R_RETURN(m_x->Write(offset, sf::InNonSecureBuffer(buffer, size), static_cast<s64>(size)));
            }

            virtual Result SetSize(s64 size) override final {
                R_RETURN(m_x->SetSize(size));
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                AMS_UNUSED(src, src_size);
                switch (op_id) {
                    case OperationId::Invalidate:
                        {
                            fs::QueryRangeInfo dummy_range_info;
                            R_RETURN(m_x->OperateRange(std::addressof(dummy_range_info), static_cast<s32>(op_id), offset, size));
                        }
                    case OperationId::QueryRange:
                        {
                            R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                            R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                            R_RETURN(m_x->OperateRange(reinterpret_cast<fs::QueryRangeInfo *>(dst), static_cast<s32>(op_id), offset, size));
                        }
                    default:
                        {
                            R_THROW(fs::ResultUnsupportedOperateRangeForStorageServiceObjectAdapter());
                        }
                }
            }
    };

}
