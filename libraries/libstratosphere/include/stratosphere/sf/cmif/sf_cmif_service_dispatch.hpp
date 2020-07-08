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
#include "sf_cmif_server_message_processor.hpp"

namespace ams::sf::hipc {

    class ServerSessionManager;
    class ServerSession;

}

namespace ams::sf::cmif {

    class ServerMessageProcessor;

    struct HandlesToClose {
        Handle handles[8];
        size_t num_handles;
    };

    struct ServiceDispatchContext {
        sf::IServiceObject *srv_obj;
        hipc::ServerSessionManager *manager;
        hipc::ServerSession *session;
        ServerMessageProcessor *processor;
        HandlesToClose *handles_to_close;
        const PointerAndSize pointer_buffer;
        const PointerAndSize in_message_buffer;
        const PointerAndSize out_message_buffer;
        const HipcParsedRequest request;
    };

    struct ServiceCommandMeta {
        hos::Version hosver_low;
        hos::Version hosver_high;
        u32 cmd_id;
        Result (*handler)(CmifOutHeader **out_header_ptr, ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data);

        constexpr inline bool Matches(u32 cmd_id, hos::Version hosver) const {
            return this->cmd_id == cmd_id && this->hosver_low <= hosver && hosver <= this->hosver_high;
        }

        constexpr inline decltype(handler) GetHandler() const {
            return this->handler;
        }
    };
    static_assert(util::is_pod<ServiceCommandMeta>::value && sizeof(ServiceCommandMeta) == 0x18, "sizeof(ServiceCommandMeta)");

    namespace impl {

        class ServiceDispatchTableBase {
            protected:
                Result ProcessMessageImpl(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const ServiceCommandMeta *entries, const size_t entry_count) const;
                Result ProcessMessageForMitmImpl(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const ServiceCommandMeta *entries, const size_t entry_count) const;
            public:
                /* CRTP. */
                template<typename T>
                Result ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const {
                    static_assert(std::is_base_of<ServiceDispatchTableBase, T>::value, "ServiceDispatchTableBase::Process<T>");
                    return static_cast<const T *>(this)->ProcessMessage(ctx, in_raw_data);
                }

                template<typename T>
                Result ProcessMessageForMitm(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const {
                    static_assert(std::is_base_of<ServiceDispatchTableBase, T>::value, "ServiceDispatchTableBase::ProcessForMitm<T>");
                    return static_cast<const T *>(this)->ProcessMessageForMitm(ctx, in_raw_data);
                }
        };

        template<size_t N, class = std::make_index_sequence<N>>
        class ServiceDispatchTableImpl;

        template<size_t N, size_t... Is>
        class ServiceDispatchTableImpl<N, std::index_sequence<Is...>> : public ServiceDispatchTableBase {
            private:
                template<size_t>
                using EntryType = ServiceCommandMeta;
            private:
                const std::array<ServiceCommandMeta, N> entries;
            public:
                explicit constexpr ServiceDispatchTableImpl(EntryType<Is>... args) : entries { args... } { /* ... */ }

                Result ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const {
                    return this->ProcessMessageImpl(ctx, in_raw_data, this->entries.data(), this->entries.size());
                }

                Result ProcessMessageForMitm(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const {
                    return this->ProcessMessageForMitmImpl(ctx, in_raw_data, this->entries.data(), this->entries.size());
                }
        };

    }

    template<typename ...Entries>
    class ServiceDispatchTable : public impl::ServiceDispatchTableImpl<sizeof...(Entries)> {
        public:
            explicit constexpr ServiceDispatchTable(Entries... entries) : impl::ServiceDispatchTableImpl<sizeof...(Entries)>(entries...) { /* ... */ }
    };

    #define AMS_SF_CMIF_IMPL_DEFINE_SERVICE_DISPATCH_TABLE \
    template<typename ServiceImpl> \
    static constexpr inline ::ams::sf::cmif::ServiceDispatchTable s_CmifServiceDispatchTable

    struct ServiceDispatchMeta {
        const impl::ServiceDispatchTableBase *DispatchTable;
        Result (impl::ServiceDispatchTableBase::*ProcessHandler)(ServiceDispatchContext &, const cmif::PointerAndSize &) const;

        constexpr uintptr_t GetServiceId() const {
            return reinterpret_cast<uintptr_t>(this->DispatchTable);
        }
    };

    template<typename T> requires sf::IsServiceObject<T>
    struct ServiceDispatchTraits {
        using ProcessHandlerType = decltype(ServiceDispatchMeta::ProcessHandler);

        static constexpr inline auto DispatchTable = T::template s_CmifServiceDispatchTable<T>;
        using DispatchTableType = decltype(DispatchTable);

        static constexpr ProcessHandlerType ProcessHandlerImpl = sf::IsMitmServiceObject<T> ? (&impl::ServiceDispatchTableBase::ProcessMessageForMitm<DispatchTableType>)
                                                                                            : (&impl::ServiceDispatchTableBase::ProcessMessage<DispatchTableType>);

        static constexpr inline ServiceDispatchMeta Meta{&DispatchTable, ProcessHandlerImpl};
    };

    template<typename T>
    constexpr ALWAYS_INLINE const ServiceDispatchMeta *GetServiceDispatchMeta() {
        return &ServiceDispatchTraits<T>::Meta;
    }

}
