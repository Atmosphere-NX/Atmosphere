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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_thread_queue.hpp>

namespace ams::kern {

    class KLightSession;

    class KLightServerSession final : public KAutoObject, public util::IntrusiveListBaseNode<KLightServerSession> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KLightServerSession, KAutoObject);
        private:
            KLightSession *m_parent;
            KThread::WaiterList m_request_list;
            KThread *m_current_request;
            u64 m_server_thread_id;
            KThread *m_server_thread;
        public:
            explicit KLightServerSession() : m_current_request(nullptr), m_server_thread_id(std::numeric_limits<u64>::max()), m_server_thread() { /* ... */ }

            void Initialize(KLightSession *parent) {
                /* Set member variables. */
                m_parent = parent;
            }

            virtual void Destroy() override;

            constexpr const KLightSession *GetParent() const { return m_parent; }

            Result OnRequest(KThread *request_thread);
            Result ReplyAndReceive(u32 *data);

            void OnClientClosed();
        private:
            void CleanupRequests();
    };

}
