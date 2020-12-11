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

namespace ams::kern {

    class KPort;
    class KSession;
    class KClientSession;
    class KLightSession;
    class KLightClientSession;

    class KClientPort final : public KSynchronizationObject {
        MESOSPHERE_AUTOOBJECT_TRAITS(KClientPort, KSynchronizationObject);
        private:
            std::atomic<s32> num_sessions;
            std::atomic<s32> peak_sessions;
            s32 max_sessions;
            KPort *parent;
        public:
            constexpr KClientPort() : num_sessions(), peak_sessions(), max_sessions(), parent() { /* ... */ }
            virtual ~KClientPort() { /* ... */ }

            void Initialize(KPort *parent, s32 max_sessions);
            void OnSessionFinalized();
            void OnServerClosed();

            constexpr const KPort *GetParent() const { return this->parent; }

            ALWAYS_INLINE s32 GetNumSessions()  const { return this->num_sessions; }
            ALWAYS_INLINE s32 GetPeakSessions() const { return this->peak_sessions; }
            ALWAYS_INLINE s32 GetMaxSessions()  const { return this->max_sessions; }

            bool IsLight() const;

            /* Overridden virtual functions. */
            virtual void Destroy() override;
            virtual bool IsSignaled() const override;

            Result CreateSession(KClientSession **out);
            Result CreateLightSession(KLightClientSession **out);
    };

}
