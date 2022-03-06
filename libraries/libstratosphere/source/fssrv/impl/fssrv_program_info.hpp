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

namespace ams::fssrv::impl {

    class ProgramInfo : public ::ams::fs::impl::Newable {
        private:
            u64 m_process_id;
            ncm::ProgramId m_program_id;
            ncm::StorageId m_storage_id;
            /* TODO: AccessControl m_access_control; */
        public:
            ProgramInfo(u64 process_id, u64 program_id, u8 storage_id, const void *data, s64 data_size, const void *desc, s64 desc_size) : m_process_id(process_id) /* TODO: m_access_control */ {
                m_program_id.value = program_id;
                m_storage_id       = static_cast<ncm::StorageId>(storage_id);

                /* TODO */
                AMS_UNUSED(data, data_size, desc, desc_size);
            }

            bool Contains(u64 process_id) const { return m_process_id == process_id; }
            u64 GetProcessId() const { return m_process_id; }
            ncm::ProgramId GetProgramId() const { return m_program_id; }
            u64 GetProgramIdValue() const { return m_program_id.value; }
            ncm::StorageId GetStorageId() const { return m_storage_id; }
        public:
            static std::shared_ptr<ProgramInfo> GetProgramInfoForInitialProcess();
        private:
            ProgramInfo(const void *data, s64 data_size, const void *desc, s64 desc_size) : m_process_id(os::InvalidProcessId), m_program_id(ncm::InvalidProgramId), m_storage_id(static_cast<ncm::StorageId>(0)) /* TODO: m_access_control */ {
                /* TODO */
                AMS_UNUSED(data, data_size, desc, desc_size);
            }
    };

    struct ProgramInfoNode : public util::IntrusiveListBaseNode<ProgramInfoNode>, public ::ams::fs::impl::Newable {
        std::shared_ptr<ProgramInfo> program_info;
    };

    bool IsInitialProgram(u64 process_id);
    bool IsCurrentProcess(u64 process_id);

}
