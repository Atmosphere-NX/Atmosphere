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
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_thread_queue.hpp>

namespace ams::kern {

    class KLightSession;

    class KLightServerSession final : public KAutoObjectWithSlabHeapAndContainer<KLightServerSession, KAutoObjectWithList>, public util::IntrusiveListBaseNode<KLightServerSession> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KLightServerSession, KAutoObject);
        private:
            KLightSession *parent;
            KThreadQueue request_queue;
            KThreadQueue server_queue;
            KThread *current_request;
            KThread *server_thread;
        public:
            constexpr KLightServerSession() : parent(), request_queue(), server_queue(), current_request(), server_thread() { /* ... */ }
            virtual ~KLightServerSession() { /* ... */ }

            void Initialize(KLightSession *parent) {
                /* Set member variables. */
                this->parent = parent;
            }

            virtual void Destroy() override;
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            constexpr const KLightSession *GetParent() const { return this->parent; }

            Result OnRequest(KThread *request_thread);
            Result ReplyAndReceive(u32 *data);

            void OnClientClosed();
        private:
            void CleanupRequests();
    };

}
