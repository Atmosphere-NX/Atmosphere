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
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KPort;
    class KServerSession;
    class KLightServerSession;

    class KServerPort final : public KSynchronizationObject {
        MESOSPHERE_AUTOOBJECT_TRAITS(KServerPort, KSynchronizationObject);
        private:
            using SessionList      = util::IntrusiveListBaseTraits<KServerSession>::ListType;
            using LightSessionList = util::IntrusiveListBaseTraits<KLightServerSession>::ListType;
        private:
            SessionList session_list;
            LightSessionList light_session_list;
            KPort *parent;
        public:
            constexpr KServerPort() : session_list(), light_session_list(), parent() { /* ... */ }
            virtual ~KServerPort() { /* ... */ }

            void Initialize(KPort *parent);
            void EnqueueSession(KServerSession *session);
            void EnqueueSession(KLightServerSession *session);

            KServerSession *AcceptSession();
            KLightServerSession *AcceptLightSession();

            constexpr const KPort *GetParent() const { return this->parent; }

            bool IsLight() const;

            /* Overridden virtual functions. */
            virtual void Destroy() override;
            virtual bool IsSignaled() const override;
        private:
            void CleanupSessions();
    };

}
