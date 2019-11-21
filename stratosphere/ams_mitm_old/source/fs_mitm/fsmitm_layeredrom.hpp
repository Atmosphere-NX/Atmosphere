/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "fs_istorage.hpp"
#include "fsmitm_romfsbuild.hpp"
#include "../utils.hpp"


/* Represents a merged RomFS. */
class LayeredRomFS : public IROStorage {
    private:
        /* Data Sources. */
        std::shared_ptr<IROStorage> storage_romfs;
        std::shared_ptr<IROStorage> file_romfs;
        /* Information about the merged RomFS. */
        u64 program_id;
        std::shared_ptr<std::vector<RomFSSourceInfo>> p_source_infos;

    public:
        LayeredRomFS(std::shared_ptr<IROStorage> s_r, std::shared_ptr<IROStorage> f_r, u64 tid);
        virtual ~LayeredRomFS() = default;

        virtual Result Read(void *buffer, size_t size, u64 offset) override;
        virtual Result GetSize(u64 *out_size) override;
        virtual Result OperateRange(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override;
};
