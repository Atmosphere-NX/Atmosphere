/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>

#include "fsmitm_romstorage.hpp"
#include "fsmitm_romfsbuild.hpp"
#include "fsmitm_utils.hpp"


/* Represents a merged RomFS. */
class LayeredRomFS : public IROStorage {
    private:
        /* Data Sources. */
        std::shared_ptr<RomInterfaceStorage> storage_romfs;
        std::shared_ptr<RomFileStorage> file_romfs;
        /* Information about the merged RomFS. */
        u64 title_id;
        std::shared_ptr<std::vector<RomFSSourceInfo>> p_source_infos;
                
    public:
        LayeredRomFS(std::shared_ptr<RomInterfaceStorage> s_r, std::shared_ptr<RomFileStorage> f_r, u64 tid);
        virtual ~LayeredRomFS() = default;
        
        virtual Result Read(void *buffer, size_t size, u64 offset) override;
        virtual Result GetSize(u64 *out_size) override;
        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override;
};
