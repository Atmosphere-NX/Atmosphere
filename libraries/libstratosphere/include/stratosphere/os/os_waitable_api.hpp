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
#include <vapours.hpp>
#include <stratosphere/os/os_message_queue_common.hpp>

namespace ams::os {

    struct WaitableHolderType;
    struct WaitableManagerType;

    void InitializeWaitableManager(WaitableManagerType *manager);
    void FinalizeWaitableManager(WaitableManagerType *manager);

    WaitableHolderType *WaitAny(WaitableManagerType *manager);
    WaitableHolderType *TryWaitAny(WaitableManagerType *manager);
    WaitableHolderType *TimedWaitAny(WaitableManagerType *manager, TimeSpan timeout);

    void FinalizeWaitableHolder(WaitableHolderType *holder);

    void LinkWaitableHolder(WaitableManagerType *manager, WaitableHolderType *holder);
    void UnlinkWaitableHolder(WaitableHolderType *holder);
    void UnlinkAllWaitableHolder(WaitableManagerType *manager);

    void MoveAllWaitableHolder(WaitableManagerType *dst, WaitableManagerType *src);

    void SetWaitableHolderUserData(WaitableHolderType *holder, uintptr_t user_data);
    uintptr_t GetWaitableHolderUserData(const WaitableHolderType *holder);

    void InitializeWaitableHolder(WaitableHolderType *holder, Handle handle);

}
