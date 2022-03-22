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
#include "fssrv_program_info.hpp"

namespace ams::fssrv::impl {

    class ProgramRegistryManager {
        NON_COPYABLE(ProgramRegistryManager);
        NON_MOVEABLE(ProgramRegistryManager);
        private:
            using ProgramInfoList = util::IntrusiveListBaseTraits<ProgramInfoNode>::ListType;
        private:
            ProgramInfoList m_program_info_list{};
            os::SdkMutex m_mutex{};
        public:
            constexpr ProgramRegistryManager() = default;

            Result RegisterProgram(u64 process_id, u64 program_id, u8 storage_id, const void *data, s64 data_size, const void *desc, s64 desc_size);
            Result UnregisterProgram(u64 process_id);

            Result GetProgramInfo(std::shared_ptr<ProgramInfo> *out, u64 process_id);
            Result GetProgramInfoByProgramId(std::shared_ptr<ProgramInfo> *out, u64 program_id);
    };

}

AMS_FSSYSTEM_ENABLE_PIMPL(::ams::fssrv::impl::ProgramRegistryManager);
