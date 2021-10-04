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
#include <stratosphere.hpp>
#include "htcfs_client_impl.hpp"

namespace ams::htcfs {

    enum class HtcfsResult {
        Success                    = 0,
        UnknownError               = 1,
        UnsupportedProtocolVersion = 2,
        InvalidRequest             = 3,
        InvalidHandle              = 4,
        OutOfHandle                = 5,
        Ready                      = 6,
    };

    inline Result ConvertHtcfsResult(HtcfsResult result) {
        switch (result) {
            case HtcfsResult::Success:
                return ResultSuccess();
            case HtcfsResult::UnknownError:
                return htcfs::ResultUnknownError();
            case HtcfsResult::UnsupportedProtocolVersion:
                return htcfs::ResultUnsupportedProtocolVersion();
            case HtcfsResult::InvalidRequest:
                return htcfs::ResultInvalidRequest();
            case HtcfsResult::InvalidHandle:
                return htcfs::ResultInvalidHandle();
            case HtcfsResult::OutOfHandle:
                return htcfs::ResultOutOfHandle();
            default:
                return htcfs::ResultUnknownError();
        }
    }

    inline Result ConvertHtcfsResult(s64 param) {
        return ConvertHtcfsResult(static_cast<HtcfsResult>(param));
    }

}
