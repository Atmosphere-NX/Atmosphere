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
#include <exosphere.hpp>

namespace ams::secmon {

    enum UserRebootType {
        UserRebootType_None         = 0,
        UserRebootType_ToRcm        = 1,
        UserRebootType_ToPayload    = 2,
        UserRebootType_ToFatalError = 3,
    };

    void PerformUserRebootToRcm();
    void PerformUserRebootToPayload();
    void PerformUserRebootToFatalError();
    void PerformUserShutDown();

}
