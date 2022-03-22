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
#include <stratosphere/sf/sf_service_object.hpp>
#include <stratosphere/sf/cmif/sf_cmif_pointer_and_size.hpp>
#include <stratosphere/sf/cmif/sf_cmif_server_message_processor.hpp>

namespace ams::sf::hipc {

    class ServerSessionManager;
    class ServerSession;

}

namespace ams::sf::cmif {

    class ServerMessageProcessor;

    struct HandlesToClose {
        os::NativeHandle handles[8];
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

        constexpr inline bool MatchesVersion(hos::Version hosver) const {
            const bool min_valid = this->hosver_low == hos::Version_Min;
            const bool max_valid = this->hosver_high == hos::Version_Max;

            return (min_valid || this->hosver_low <= hosver) && (max_valid || hosver <= this->hosver_high);
        }

        constexpr inline bool Matches(u32 cmd_id, hos::Version hosver) const {
            return this->cmd_id == cmd_id && this->MatchesVersion(hosver);
        }

        constexpr inline decltype(handler) GetHandler() const {
            return this->handler;
        }

        constexpr inline bool operator>(const ServiceCommandMeta &rhs) const {
            if (this->cmd_id > rhs.cmd_id) {
                return true;
            } else if (this->cmd_id == rhs.cmd_id && this->hosver_low > rhs.hosver_low) {
                return true;
            } else if (this->cmd_id == rhs.cmd_id && this->hosver_low == rhs.hosver_low && this->hosver_high == rhs.hosver_high){
                return true;
            } else {
                return false;
            }
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

        template<size_t N>
        class ServiceDispatchTableImpl : public ServiceDispatchTableBase {
            public:
                static constexpr size_t NumEntries = N;
            private:
                const std::array<ServiceCommandMeta, N> m_entries;
            public:
                explicit constexpr ServiceDispatchTableImpl(const std::array<ServiceCommandMeta, N> &e) : m_entries{e} { /* ... */ }

                Result ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const {
                    return this->ProcessMessageImpl(ctx, in_raw_data, m_entries.data(), m_entries.size());
                }

                Result ProcessMessageForMitm(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const {
                    return this->ProcessMessageForMitmImpl(ctx, in_raw_data, m_entries.data(), m_entries.size());
                }

                constexpr const std::array<ServiceCommandMeta, N> &GetEntries() const {
                    return m_entries;
                }
        };

    }

    template<size_t N>
    class ServiceDispatchTable : public impl::ServiceDispatchTableImpl<N> {
        public:
            explicit constexpr ServiceDispatchTable(const std::array<ServiceCommandMeta, N> &e) : impl::ServiceDispatchTableImpl<N>(e) { /* ... */ }
    };

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

        static constexpr inline ServiceDispatchMeta Meta{std::addressof(DispatchTable), ProcessHandlerImpl};
    };

    template<>
    struct ServiceDispatchTraits<sf::IServiceObject> {
        static constexpr inline auto DispatchTable = ServiceDispatchTable<0>(std::array<ServiceCommandMeta, 0>{});
    };

    template<>
    struct ServiceDispatchTraits<sf::IMitmServiceObject> {
        static constexpr inline auto DispatchTable = ServiceDispatchTable<0>(std::array<ServiceCommandMeta, 0>{});
    };

    template<typename T>
    constexpr ALWAYS_INLINE const ServiceDispatchMeta *GetServiceDispatchMeta() {
        return std::addressof(ServiceDispatchTraits<T>::Meta);
    }

}
