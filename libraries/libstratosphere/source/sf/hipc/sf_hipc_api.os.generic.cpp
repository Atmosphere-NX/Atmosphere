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

namespace ams::sf::hipc {

    void AttachMultiWaitHolderForAccept(os::MultiWaitHolderType *, os::NativeHandle) {
        AMS_ABORT("TODO: Generic ams::sf::hipc::AttachMultiWaitHolderForAccept");
    }

    void AttachMultiWaitHolderForReply(os::MultiWaitHolderType *, os::NativeHandle) {
        AMS_ABORT("TODO: Generic ams::sf::hipc::AttachMultiWaitHolderForAccept");
    }

    Result Receive(ReceiveResult *, os::NativeHandle, const cmif::PointerAndSize &) {
        AMS_ABORT("TODO: Generic ams::sf::hipc::Receive(ReceiveResult *, os::NativeHandle, const cmif::PointerAndSize &)");
    }

    Result Receive(bool *, os::NativeHandle, const cmif::PointerAndSize &) {
        AMS_ABORT("TODO: Generic ams::sf::hipc::Receive(bool *, os::NativeHandle, const cmif::PointerAndSize &)");
    }

    Result Reply(os::NativeHandle, const cmif::PointerAndSize &) {
        AMS_ABORT("TODO: Generic ams::sf::hipc::Reply");
    }

    Result CreateSession(os::NativeHandle *, os::NativeHandle *) {
        AMS_ABORT("TODO: Generic ams::sf::hipc::CreateSession");
    }

}
