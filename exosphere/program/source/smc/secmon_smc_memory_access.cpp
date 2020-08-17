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
#include <exosphere.hpp>
#include "../secmon_error.hpp"
#include "../secmon_page_mapper.hpp"
#include "secmon_smc_memory_access.hpp"

namespace ams::secmon::smc {

    namespace {

        enum IramCopyType {
            IramCopyType_FromIramToDram = 0,
            IramCopyType_FromDramToIram = 1,
            IramCopyType_Count,
        };

        struct IramCopyOption {
            using CopyType = util::BitPack32::Field<0, 1, IramCopyType>;
        };

    }

    /* This is an atmosphere extension smc. */
    SmcResult SmcIramCopy(SmcArguments &args) {
        /* Decode arguments. */
        const uintptr_t dram_address = args.r[1];
        const uintptr_t iram_address = args.r[2];
        const size_t  size           = args.r[3];
        const util::BitPack32 option = { static_cast<u32>(args.r[4]) };

        const auto copy_type = option.Get<IramCopyOption::CopyType>();

        /* Validate arguments. */
        SMC_R_UNLESS(copy_type < IramCopyType_Count, InvalidArgument);

        {
            /* Map the pages. */
            AtmosphereUserPageMapper dram_mapper(dram_address);
            AtmosphereIramPageMapper iram_mapper(iram_address);
            SMC_R_UNLESS(dram_mapper.Map(), InvalidArgument);
            SMC_R_UNLESS(iram_mapper.Map(), InvalidArgument);

            /* Get the ranges we're copying. */
            const void * const src = (copy_type == IramCopyType_FromIramToDram) ? iram_mapper.GetPointerTo(iram_address, size) : dram_mapper.GetPointerTo(dram_address, size);
                  void * const dst = (copy_type == IramCopyType_FromIramToDram) ? dram_mapper.GetPointerTo(dram_address, size) : iram_mapper.GetPointerTo(iram_address, size);
            SMC_R_UNLESS(src != nullptr, InvalidArgument);
            SMC_R_UNLESS(dst != nullptr, InvalidArgument);

            /* Copy the data. */
            std::memcpy(dst, src, size);
        }

        return SmcResult::Success;
    }

    SmcResult SmcWriteAddress(SmcArguments &args) {
        /* NOTE: This smc was deprecated in Atmosphère 0.13.0. */
        AMS_UNUSED(args);
        return SmcResult::NotImplemented;
    }

}
