/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>
#include "dnsmitm_debug.hpp"
#include "dnsmitm_host_redirection.hpp"

namespace ams::mitm::socket::resolver {

    namespace {

        /* 23.0.0 added a new, asynchronous (c-ares style) set of sfdnsres commands. */
        /* nim/ns/qlaunch built with SDK 23.x resolve via these instead of the legacy */
        /* GetHostByName/GetAddrInfo commands which dns.mitm overrides. */
        /* Observed in firmware 23.0.0 bsdsocket (server) and nim (client): */
        /*   99-101, 103, 104, 106 (client) / 100-105, 107, 110 (server). */
        /* Switchbrew documents 100 QueryAddrInfoRequestWithOptions,            */
        /*   101 QueryNameInfoRequestWithOptions, 102 FetchAddrInfoRequestWithOptions, */
        /*   103 FetchNameInfoRequestWithOptions, 104 CancelQuery.              */
        constexpr u32 SfdnsresFirstNewCommandId = 99;
        constexpr u32 SfdnsresLastNewCommandId  = 110;

        constexpr bool IsNewResolverCommand(u32 cmd_id) {
            return SfdnsresFirstNewCommandId <= cmd_id && cmd_id <= SfdnsresLastNewCommandId;
        }

        constexpr bool IsPrintableDnsChar(char c) {
            return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '-' || c == '.';
        }

        bool BufferContainsRedirectedHost(const u8 *buf, size_t size) {
            if (buf == nullptr || size == 0) {
                return false;
            }

            /* Walk the buffer, extracting NUL/space-delimited hostname-ish tokens and */
            /* testing each against the hosts redirection list. */
            size_t start = 0;
            for (size_t i = 0; i <= size; ++i) {
                const bool at_end = i == size;
                if (!at_end && IsPrintableDnsChar(static_cast<char>(buf[i]))) {
                    continue;
                }

                const size_t len = i - start;
                if (4 <= len && len <= 255) {
                    char name[256];
                    std::memcpy(name, buf + start, len);
                    name[len] = '\x00';

                    ams::socket::InAddrT redirect_addr = {};
                    if (GetRedirectedHostByName(std::addressof(redirect_addr), name)) {
                        LogDebug("[raw] intercepted new-command query for %s\n", name);
                        return true;
                    }
                }

                if (at_end) {
                    break;
                }
                start = i + 1;
            }

            return false;
        }

        bool RequestContainsRedirectedHost(const ams::sf::cmif::ServiceDispatchContext &ctx, const ams::sf::cmif::PointerAndSize &in_raw_data) {
            const HipcParsedRequest &req = ctx.request;

            /* X buffers (send statics). */
            for (u32 i = 0; i < req.meta.num_send_statics; ++i) {
                const HipcStaticDescriptor &d = req.data.send_statics[i];
                const uintptr_t addr = (static_cast<uintptr_t>(d.address_high) << 36) | (static_cast<uintptr_t>(d.address_mid) << 32) | d.address_low;
                if (BufferContainsRedirectedHost(reinterpret_cast<const u8 *>(addr), d.size)) {
                    return true;
                }
            }

            /* A/B/C buffers. */
            const struct {
                const HipcBufferDescriptor *buffers;
                u32 count;
            } buffer_lists[] = {
                { req.data.send_buffers, req.meta.num_send_buffers },
                { req.data.recv_buffers, req.meta.num_recv_buffers },
                { req.data.exch_buffers, req.meta.num_exch_buffers },
            };

            for (const auto &list : buffer_lists) {
                for (u32 i = 0; i < list.count; ++i) {
                    const HipcBufferDescriptor &d = list.buffers[i];
                    const uintptr_t addr = (static_cast<uintptr_t>(d.address_high) << 36) | (static_cast<uintptr_t>(d.address_mid) << 32) | d.address_low;
                    const size_t size = static_cast<size_t>(d.size_low) | (static_cast<size_t>(d.size_high) << 32);
                    if (BufferContainsRedirectedHost(reinterpret_cast<const u8 *>(addr), size)) {
                        return true;
                    }
                }
            }

            /* Also scan the raw (non-buffer) section of the message. */
            return BufferContainsRedirectedHost(reinterpret_cast<const u8 *>(in_raw_data.GetPointer()), in_raw_data.GetSize());

        }

        Result InterceptRawCommandImpl(u32 cmd_id, ams::sf::cmif::ServiceDispatchContext &ctx, const ams::sf::cmif::PointerAndSize &in_raw_data) {
            AMS_UNUSED(in_raw_data);

            /* Only the new async resolver commands need raw interception. */
            if (!IsNewResolverCommand(cmd_id)) {
                R_RETURN(sm::mitm::ResultShouldForwardToSession());
            }

            /* If the query carries a hostname which the hosts file redirects, refuse the query. */
            /* This makes the client's resolution attempt fail, exactly like a blocked lookup. */
            if (RequestContainsRedirectedHost(ctx, in_raw_data)) {
                R_THROW(sf::cmif::ResultUnknownCommandId());
            }

            /* Otherwise, behave exactly as an unpatched mitm would (forward). */
            R_RETURN(sm::mitm::ResultShouldForwardToSession());
        }

    }

    void InitializeRawCommandInterceptor() {
        ams::sf::cmif::impl::SetMitmRawInterceptFunction(InterceptRawCommandImpl);
    }

}
