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
#include <stratosphere.hpp>
#include "lm_log_getter.hpp"
#include "lm_log_getter_impl.hpp"

namespace ams::lm::srv {

    extern bool g_is_logging_to_custom_sink;

    Result LogGetter::StartLogging() {
        g_is_logging_to_custom_sink = true;
        return ResultSuccess();
    }

    Result LogGetter::StopLogging() {
        g_is_logging_to_custom_sink = false;
        return ResultSuccess();
    }

    Result LogGetter::GetLog(const sf::OutAutoSelectBuffer &message, sf::Out<s64> out_size, sf::Out<u32> out_drop_count) {
        /* Try to flush logs. */
        if (LogGetterImpl::GetBuffer().TryFlush()) {
            *out_size = LogGetterImpl::GetLog(message.GetPointer(), message.GetSize(), out_drop_count.GetPointer());
        } else {
            /* Otherwise, we got no data. */
            *out_size = 0;
        }

        return ResultSuccess();
    }

}
