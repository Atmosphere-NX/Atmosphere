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

namespace ams::sf::cmif {

    Result impl::ServiceDispatchTableBase::ProcessMessageImpl(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const ServiceCommandMeta *entries, const size_t entry_count) const {
        /* Get versioning info. */
        const auto hos_version      = hos::GetVersion();
        const u32  max_cmif_version = hos_version >= hos::Version_5_0_0 ? 1 : 0;

        /* Parse the CMIF in header. */
        const CmifInHeader *in_header = reinterpret_cast<const CmifInHeader *>(in_raw_data.GetPointer());
        R_UNLESS(in_raw_data.GetSize() >= sizeof(*in_header), sf::cmif::ResultInvalidHeaderSize());
        R_UNLESS(in_header->magic == CMIF_IN_HEADER_MAGIC && in_header->version <= max_cmif_version, sf::cmif::ResultInvalidInHeader());
        const cmif::PointerAndSize in_message_raw_data = cmif::PointerAndSize(in_raw_data.GetAddress() + sizeof(*in_header), in_raw_data.GetSize() - sizeof(*in_header));
        const u32 cmd_id = in_header->command_id;

        /* Find a handler. */
        decltype(ServiceCommandMeta::handler) cmd_handler = nullptr;
        for (size_t i = 0; i < entry_count; i++) {
            if (entries[i].Matches(cmd_id, hos_version)) {
                cmd_handler = entries[i].GetHandler();
                break;
            }
        }
        R_UNLESS(cmd_handler != nullptr, sf::cmif::ResultUnknownCommandId());

        /* Invoke handler. */
        CmifOutHeader *out_header = nullptr;
        Result command_result = cmd_handler(&out_header, ctx, in_message_raw_data);

        /* Forward any meta-context change result. */
        if (sf::impl::ResultRequestContextChanged::Includes(command_result)) {
            return command_result;
        }

        /* Otherwise, ensure that we're able to write the output header. */
        if (out_header == nullptr) {
            AMS_ABORT_UNLESS(R_FAILED(command_result));
            return command_result;
        }

        /* Write output header to raw data. */
        *out_header = CmifOutHeader{CMIF_OUT_HEADER_MAGIC, 0, command_result.GetValue(), 0};

        return ResultSuccess();
    }

    Result impl::ServiceDispatchTableBase::ProcessMessageForMitmImpl(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const ServiceCommandMeta *entries, const size_t entry_count) const {
        /* Get versioning info. */
        const auto hos_version      = hos::GetVersion();
        const u32  max_cmif_version = hos_version >= hos::Version_5_0_0 ? 1 : 0;

        /* Parse the CMIF in header. */
        const CmifInHeader *in_header = reinterpret_cast<const CmifInHeader *>(in_raw_data.GetPointer());
        R_UNLESS(in_raw_data.GetSize() >= sizeof(*in_header), sf::cmif::ResultInvalidHeaderSize());
        R_UNLESS(in_header->magic == CMIF_IN_HEADER_MAGIC && in_header->version <= max_cmif_version, sf::cmif::ResultInvalidInHeader());
        const cmif::PointerAndSize in_message_raw_data = cmif::PointerAndSize(in_raw_data.GetAddress() + sizeof(*in_header), in_raw_data.GetSize() - sizeof(*in_header));
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
            return ctx.session->ForwardRequest(ctx);
        }

        /* Invoke handler. */
        CmifOutHeader *out_header = nullptr;
        Result command_result = cmd_handler(&out_header, ctx, in_message_raw_data);

        /* If we should, forward the request to the forward session. */
        if (sm::mitm::ResultShouldForwardToSession::Includes(command_result)) {
            return ctx.session->ForwardRequest(ctx);
        }

        /* Forward any meta-context change result. */
        if (sf::impl::ResultRequestContextChanged::Includes(command_result)) {
            return command_result;
        }

        /* Otherwise, ensure that we're able to write the output header. */
        if (out_header == nullptr) {
            AMS_ABORT_UNLESS(R_FAILED(command_result));
            return command_result;
        }

        /* Write output header to raw data. */
        *out_header = CmifOutHeader{CMIF_OUT_HEADER_MAGIC, 0, command_result.GetValue(), 0};

        return ResultSuccess();
    }

}
