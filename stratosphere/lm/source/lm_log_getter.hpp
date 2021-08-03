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
#include <stratosphere.hpp>

#define AMS_LM_I_LOG_GETTER_INFO(C, H)                                                                                                                                                                                          \
    AMS_SF_METHOD_INFO(C, H,  0, void, StartLogging, (), ())                                                                                                                                                                    \
    AMS_SF_METHOD_INFO(C, H,  1, void, StopLogging,  (), ())                                                                                                                                                                    \
    AMS_SF_METHOD_INFO(C, H,  2, void, GetLog,       (const sf::OutAutoSelectBuffer &out_log_buffer, sf::Out<size_t> out_size, sf::Out<u64> out_log_packet_drop_count), (out_log_buffer, out_size, out_log_packet_drop_count))

AMS_SF_DEFINE_INTERFACE(ams::lm::impl, ILogGetter, AMS_LM_I_LOG_GETTER_INFO)

namespace ams::lm {

    class LogGetter {
        public:
            void StartLogging();
            void StopLogging();
            void GetLog(const sf::OutAutoSelectBuffer &out_log_buffer, sf::Out<size_t> out_size, sf::Out<u64> out_log_packet_drop_count);
    };
    static_assert(impl::IsILogGetter<LogGetter>);

}