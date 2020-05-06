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

#pragma once
#include "../sf_service_object.hpp"
#include "sf_cmif_pointer_and_size.hpp"

namespace ams::sf::cmif {

    /* Forward declare ServiceDispatchContext, ServiceObjectHolder. */
    struct ServiceDispatchContext;
    class  ServiceObjectHolder;
    struct DomainObjectId;

    /* This is needed for non-templated domain message processing. */
    struct ServerMessageRuntimeMetadata {
        u16 in_data_size;
        u16 out_data_size;
        u8 in_headers_size;
        u8 out_headers_size;
        u8 in_object_count;
        u8 out_object_count;

        constexpr size_t GetInDataSize() const {
            return size_t(this->in_data_size);
        }

        constexpr size_t GetOutDataSize() const {
            return size_t(this->out_data_size);
        }

        constexpr size_t GetInHeadersSize() const {
            return size_t(this->in_headers_size);
        }

        constexpr size_t GetOutHeadersSize() const {
            return size_t(this->out_headers_size);
        }

        constexpr size_t GetInObjectCount() const {
            return size_t(this->in_object_count);
        }

        constexpr size_t GetOutObjectCount() const {
            return size_t(this->out_object_count);
        }

        constexpr size_t GetUnfixedOutPointerSizeOffset() const {
            return this->GetInDataSize() + this->GetInHeadersSize() + 0x10 /* padding. */;
        }
    };

    static_assert(util::is_pod<ServerMessageRuntimeMetadata>::value, "util::is_pod<ServerMessageRuntimeMetadata>::value");
    static_assert(sizeof(ServerMessageRuntimeMetadata) == sizeof(u64), "sizeof(ServerMessageRuntimeMetadata)");

    class ServerMessageProcessor {
        public:
            /* Used to enabled templated message processors. */
            virtual void SetImplementationProcessor(ServerMessageProcessor *impl) = 0;
            virtual const ServerMessageRuntimeMetadata GetRuntimeMetadata() const = 0;

            virtual Result PrepareForProcess(const ServiceDispatchContext &ctx, const ServerMessageRuntimeMetadata runtime_metadata) const = 0;
            virtual Result GetInObjects(ServiceObjectHolder *in_objects) const = 0;
            virtual HipcRequest PrepareForReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const ServerMessageRuntimeMetadata runtime_metadata) = 0;
            virtual void PrepareForErrorReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const ServerMessageRuntimeMetadata runtime_metadata) = 0;
            virtual void SetOutObjects(const cmif::ServiceDispatchContext &ctx, const HipcRequest &response, ServiceObjectHolder *out_objects, DomainObjectId *ids) = 0;
    };
}
