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
#include <mesosphere.hpp>

namespace ams::kern {

    void KLightServerSession::Destroy() {
        MESOSPHERE_ASSERT_THIS();

        this->CleanupRequests();

        this->parent->OnServerClosed();
    }

    void KLightServerSession::OnClientClosed() {
        MESOSPHERE_ASSERT_THIS();
    }

    Result KLightServerSession::OnRequest(KThread *request_thread) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KLightServerSession::ReplyAndReceive(u32 *data) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    void KLightServerSession::CleanupRequests() {
        MESOSPHERE_UNIMPLEMENTED();
    }

}
