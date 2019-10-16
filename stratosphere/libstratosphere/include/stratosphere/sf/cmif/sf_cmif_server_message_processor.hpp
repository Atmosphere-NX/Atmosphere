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
#include "../sf_service_object.hpp"
#include "sf_cmif_pointer_and_size.hpp"

namespace sts::sf::cmif {

    /* Forward declare ServiceDispatchContext, ServiceObjectHolder. */
    struct ServiceDispatchContext;
    class  ServiceObjectHolder;
    struct DomainObjectId;

    class ServerMessageProcessor {
        public:
            /* Used to enabled templated message processors. */
            virtual void SetImplementationProcessor(ServerMessageProcessor *impl) { /* ... */ }

            virtual Result PrepareForProcess(const ServiceDispatchContext &ctx, size_t &headers_size) const = 0;
            virtual Result GetInObjects(ServiceObjectHolder *in_objects) const = 0;
            virtual HipcRequest PrepareForReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const size_t headers_size, size_t &num_out_object_handles) = 0;
            virtual void PrepareForErrorReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const size_t headers_size) = 0;
            virtual void SetOutObjects(const cmif::ServiceDispatchContext &ctx, const HipcRequest &response, ServiceObjectHolder *out_objects, DomainObjectId *ids) = 0;
    };
}
