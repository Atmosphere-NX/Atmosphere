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
#include <vapours.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 14.3.0.0 */
    template<typename F>
    class SwitchStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(SwitchStorage);
        NON_MOVEABLE(SwitchStorage);
        private:
            std::shared_ptr<fs::IStorage> m_true_storage;
            std::shared_ptr<fs::IStorage> m_false_storage;
            F m_truth_function;
        private:
            ALWAYS_INLINE std::shared_ptr<fs::IStorage> &SelectStorage() {
                return m_truth_function() ? m_true_storage : m_false_storage;
            }
        public:
            SwitchStorage(std::shared_ptr<fs::IStorage> t, std::shared_ptr<fs::IStorage> f, F func) : m_true_storage(std::move(t)), m_false_storage(std::move(f)), m_truth_function(func) { /* ... */ }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                R_RETURN(this->SelectStorage()->Read(offset, buffer, size));
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                switch (op_id) {
                    case fs::OperationId::Invalidate:
                        {
                            R_TRY(m_true_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                            R_TRY(m_false_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                            R_SUCCEED();
                        }
                    case fs::OperationId::QueryRange:
                        {
                            R_TRY(this->SelectStorage()->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                            R_SUCCEED();
                        }
                    default:
                        R_THROW(fs::ResultUnsupportedOperateRangeForSwitchStorage());
                }
            }

            virtual Result GetSize(s64 *out) override {
                R_RETURN(this->SelectStorage()->GetSize(out));
            }

            virtual Result Flush() override {
                R_RETURN(this->SelectStorage()->Flush());
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                R_RETURN(this->SelectStorage()->Write(offset, buffer, size));
            }

            virtual Result SetSize(s64 size) override {
                R_RETURN(this->SelectStorage()->SetSize(size));
            }
    };

    class RegionSwitchStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(RegionSwitchStorage);
        NON_MOVEABLE(RegionSwitchStorage);
        public:
            struct Region {
                s64 offset;
                s64 size;
            };
        private:
            std::shared_ptr<fs::IStorage> m_inside_region_storage;
            std::shared_ptr<fs::IStorage> m_outside_region_storage;
            Region m_region;
        public:
            RegionSwitchStorage(std::shared_ptr<fs::IStorage> &&i, std::shared_ptr<fs::IStorage> &&o, Region r) : m_inside_region_storage(std::move(i)), m_outside_region_storage(std::move(o)), m_region(r) {
                /* ... */
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Process until we're done. */
                size_t processed = 0;
                while (processed < size) {
                    /* Process on the appropriate storage. */
                    s64 cur_size = 0;
                    if (this->CheckRegions(std::addressof(cur_size), offset + processed, size - processed)) {
                        R_TRY(m_inside_region_storage->Read(offset + processed, static_cast<u8 *>(buffer) + processed, cur_size));
                    } else {
                        R_TRY(m_outside_region_storage->Read(offset + processed, static_cast<u8 *>(buffer) + processed, cur_size));
                    }

                    /* Advance. */
                    processed += cur_size;
                }

                R_SUCCEED();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* Process until we're done. */
                size_t processed = 0;
                while (processed < size) {
                    /* Process on the appropriate storage. */
                    s64 cur_size = 0;
                    if (this->CheckRegions(std::addressof(cur_size), offset + processed, size - processed)) {
                        R_TRY(m_inside_region_storage->Write(offset + processed, static_cast<const u8 *>(buffer) + processed, cur_size));
                    } else {
                        R_TRY(m_outside_region_storage->Write(offset + processed, static_cast<const u8 *>(buffer) + processed, cur_size));
                    }

                    /* Advance. */
                    processed += cur_size;
                }

                R_SUCCEED();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                switch (op_id) {
                    case fs::OperationId::Invalidate:
                        {
                            R_TRY(m_inside_region_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                            R_TRY(m_outside_region_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                            R_SUCCEED();
                        }
                    case fs::OperationId::QueryRange:
                        {
                            /* Create a new info. */
                            fs::QueryRangeInfo merged_info;
                            merged_info.Clear();

                            /* Process until we're done. */
                            s64 processed = 0;
                            while (processed < size) {
                                /* Process on the appropriate storage. */
                                s64 cur_size = 0;
                                fs::QueryRangeInfo cur_info;
                                if (this->CheckRegions(std::addressof(cur_size), offset + processed, size - processed)) {
                                    R_TRY(m_inside_region_storage->OperateRange(std::addressof(cur_info), sizeof(cur_info), op_id, offset + processed, cur_size, src, src_size));
                                } else {
                                    R_TRY(m_outside_region_storage->OperateRange(std::addressof(cur_info), sizeof(cur_info), op_id, offset + processed, cur_size, src, src_size));
                                }

                                /* Merge the current info. */
                                merged_info.Merge(cur_info);

                                /* Advance. */
                                processed += cur_size;
                            }

                            /* Write the merged info. */
                            *reinterpret_cast<fs::QueryRangeInfo *>(dst) = merged_info;
                            R_SUCCEED();
                        }
                    default:
                        R_THROW(fs::ResultUnsupportedOperateRangeForRegionSwitchStorage());
                }
            }

            virtual Result GetSize(s64 *out) override {
                R_RETURN(m_inside_region_storage->GetSize(out));
            }

            virtual Result Flush() override {
                /* Flush both storages. */
                R_TRY(m_inside_region_storage->Flush());
                R_TRY(m_outside_region_storage->Flush());
                R_SUCCEED();
            }

            virtual Result SetSize(s64 size) override {
                /* Set size for both storages. */
                R_TRY(m_inside_region_storage->SetSize(size));
                R_TRY(m_outside_region_storage->SetSize(size));
                R_SUCCEED();
            }
        private:
            bool CheckRegions(s64 *out_current_size, s64 offset, s64 size) const {
                /* Check if our region contains the access. */
                if (m_region.offset <= offset) {
                    if (offset < m_region.offset + m_region.size) {
                        if (m_region.offset + m_region.size <= offset + size) {
                            *out_current_size = m_region.offset + m_region.size - offset;
                        } else {
                            *out_current_size = size;
                        }
                        return true;
                    } else {
                        *out_current_size = size;
                        return false;
                    }
                } else {
                    if (m_region.offset <= offset + size) {
                        *out_current_size = m_region.offset - offset;
                    } else {
                        *out_current_size = size;
                    }
                    return false;
                }
            }
    };

}
