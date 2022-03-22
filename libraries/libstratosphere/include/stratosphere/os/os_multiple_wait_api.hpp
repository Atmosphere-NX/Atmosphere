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
#include <vapours.hpp>
#include <stratosphere/os/os_message_queue_common.hpp>
#include <stratosphere/os/os_native_handle.hpp>

namespace ams::os {

    struct MultiWaitHolderType;
    struct MultiWaitType;

    void InitializeMultiWait(MultiWaitType *multi_wait);
    void FinalizeMultiWait(MultiWaitType *multi_wait);

    MultiWaitHolderType *WaitAny(MultiWaitType *multi_wait);
    MultiWaitHolderType *TryWaitAny(MultiWaitType *multi_wait);
    MultiWaitHolderType *TimedWaitAny(MultiWaitType *multi_wait, TimeSpan timeout);

    void FinalizeMultiWaitHolder(MultiWaitHolderType *holder);

    void LinkMultiWaitHolder(MultiWaitType *multi_wait, MultiWaitHolderType *holder);
    void UnlinkMultiWaitHolder(MultiWaitHolderType *holder);
    void UnlinkAllMultiWaitHolder(MultiWaitType *multi_wait);

    void MoveAllMultiWaitHolder(MultiWaitType *dst, MultiWaitType *src);

    void SetMultiWaitHolderUserData(MultiWaitHolderType *holder, uintptr_t user_data);
    uintptr_t GetMultiWaitHolderUserData(const MultiWaitHolderType *holder);

    void InitializeMultiWaitHolder(MultiWaitHolderType *holder, NativeHandle handle);

}
