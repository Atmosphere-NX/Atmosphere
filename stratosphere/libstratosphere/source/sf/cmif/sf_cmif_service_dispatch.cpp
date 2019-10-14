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
#include <stratosphere.hpp>

namespace sts::sf::cmif {

    Result impl::ServiceDispatchTableBase::ProcessMessageImpl(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const ServiceCommandMeta *entries, const size_t entry_count) const {
        /* Get versioning info. */
        const auto hos_version      = hos::GetVersion();
        const u32  max_cmif_version = hos_version >= hos::Version_500 ? 1 : 0;

        /* Parse the CMIF in header. */
        const CmifInHeader *in_header = reinterpret_cast<const CmifInHeader *>(in_raw_data.GetPointer());
        R_UNLESS(in_raw_data.GetSize() >= sizeof(*in_header), ResultServiceFrameworkInvalidCmifHeaderSize);
        R_UNLESS(in_header->magic == CMIF_IN_HEADER_MAGIC && in_header->version <= max_cmif_version, ResultServiceFrameworkInvalidCmifInHeader);
        const u32 cmd_id = in_header->command_id;

        /* Find a handler. */
        decltype(ServiceCommandMeta::handler) cmd_handler = nullptr;
        for (size_t i = 0; i < entry_count; i++) {
            if (entries[i].Matches(cmd_id, hos_version)) {
                cmd_handler = entries[i].GetHandler();
                break;
            }
        }
        R_UNLESS(cmd_handler != nullptr, ResultServiceFrameworkUnknownCmifCommandId);

        /* Invoke handler. */
        CmifOutHeader *out_header = nullptr;
        Result command_result = cmd_handler(&out_header, ctx, cmif::PointerAndSize(in_raw_data.GetAddress() + sizeof(*in_header), in_raw_data.GetSize() - sizeof(*in_header)));

        /* Forward forwardable results, otherwise ensure we can send result to user. */
        R_TRY_CATCH(command_result) {
            R_CATCH(ResultServiceFrameworkRequestDeferredByUser) { return ResultServiceFrameworkRequestDeferredByUser; }
            R_CATCH_ALL() { STS_ASSERT(out_header != nullptr); }
        } R_END_TRY_CATCH;

        /* Write output header to raw data. */
        if (out_header != nullptr) {
            *out_header = CmifOutHeader{CMIF_OUT_HEADER_MAGIC, 0, command_result, 0};
        }

        return ResultSuccess;
    }

    Result impl::ServiceDispatchTableBase::ProcessMessageForMitmImpl(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const ServiceCommandMeta *entries, const size_t entry_count) const {
        /* Get versioning info. */
        const auto hos_version      = hos::GetVersion();
        const u32  max_cmif_version = hos_version >= hos::Version_500 ? 1 : 0;

        /* Parse the CMIF in header. */
        const CmifInHeader *in_header = reinterpret_cast<const CmifInHeader *>(in_raw_data.GetPointer());
        R_UNLESS(in_raw_data.GetSize() >= sizeof(*in_header), ResultServiceFrameworkInvalidCmifHeaderSize);
        R_UNLESS(in_header->magic == CMIF_IN_HEADER_MAGIC && in_header->version <= max_cmif_version, ResultServiceFrameworkInvalidCmifInHeader);
        const u32 cmd_id = in_header->command_id;

        /* Find a handler. */
        decltype(ServiceCommandMeta::handler) cmd_handler = nullptr;
        for (size_t i = 0; i < entry_count; i++) {
            if (entries[i].Matches(cmd_id, hos_version)) {
                cmd_handler = entries[i].GetHandler();
                break;
            }
        }

        /* If we didn't find a handler, forward the request. */
        if (cmd_handler == nullptr) {
            /* TODO: FORWARD REQUEST */
            STS_ASSERT(false);
        }

        /* Invoke handler. */
        CmifOutHeader *out_header = nullptr;
        Result command_result = cmd_handler(&out_header, ctx, cmif::PointerAndSize(in_raw_data.GetAddress() + sizeof(*in_header), in_raw_data.GetSize() - sizeof(*in_header)));

        /* Forward forwardable results, otherwise ensure we can send result to user. */
        R_TRY_CATCH(command_result) {
            R_CATCH(ResultServiceFrameworkRequestDeferredByUser) { return ResultServiceFrameworkRequestDeferredByUser; }
            R_CATCH(ResultAtmosphereMitmShouldForwardToSession) {
                /* TODO: Restore TLS. */
                /* TODO: FORWARD REQUEST */
                STS_ASSERT(false);
            }
            R_CATCH_ALL() { STS_ASSERT(out_header != nullptr); }
        } R_END_TRY_CATCH;

        /* Write output header to raw data. */
        if (out_header != nullptr) {
            *out_header = CmifOutHeader{CMIF_OUT_HEADER_MAGIC, 0, command_result, 0};
        }

        return ResultSuccess;
    }

}
