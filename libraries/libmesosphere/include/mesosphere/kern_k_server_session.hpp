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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_session_request.hpp>
#include <mesosphere/kern_k_light_lock.hpp>

namespace ams::kern {

    class KSession;

    class KServerSession final : public KSynchronizationObject, public util::IntrusiveListBaseNode<KServerSession> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KServerSession, KSynchronizationObject);
        private:
            using RequestList = util::IntrusiveListBaseTraits<KSessionRequest>::ListType;
        private:
            KSession *m_parent;
            RequestList m_request_list;
            KSessionRequest *m_current_request;
            KLightLock m_lock;
        public:
            constexpr KServerSession() : m_parent(), m_request_list(), m_current_request(), m_lock() { /* ... */ }
            virtual ~KServerSession() { /* ... */ }

            virtual void Destroy() override;

            void Initialize(KSession *p) { m_parent = p; }

            constexpr const KSession *GetParent() const { return m_parent; }

            virtual bool IsSignaled() const override;

            Result OnRequest(KSessionRequest *request);

            Result ReceiveRequest(uintptr_t message, uintptr_t buffer_size, KPhysicalAddress message_paddr);
            Result SendReply(uintptr_t message, uintptr_t buffer_size, KPhysicalAddress message_paddr);

            void OnClientClosed();

            void Dump();
        private:
            ALWAYS_INLINE bool IsSignaledImpl() const;
            void CleanupRequests();
    };

}
