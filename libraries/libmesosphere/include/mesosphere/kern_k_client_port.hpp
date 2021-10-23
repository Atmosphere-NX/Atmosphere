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

namespace ams::kern {

    class KPort;
    class KSession;
    class KClientSession;
    class KLightSession;
    class KLightClientSession;

    class KClientPort final : public KSynchronizationObject {
        MESOSPHERE_AUTOOBJECT_TRAITS(KClientPort, KSynchronizationObject);
        private:
            util::Atomic<s32> m_num_sessions;
            util::Atomic<s32> m_peak_sessions;
            s32 m_max_sessions;
            KPort *m_parent;
        public:
            constexpr explicit KClientPort(util::ConstantInitializeTag) : KSynchronizationObject(util::ConstantInitialize), m_num_sessions(0), m_peak_sessions(0), m_max_sessions(), m_parent() { /* ... */ }

            explicit KClientPort() { /* ... */ }

            void Initialize(KPort *parent, s32 max_sessions);
            void OnSessionFinalized();
            void OnServerClosed();

            constexpr const KPort *GetParent() const { return m_parent; }

            ALWAYS_INLINE s32 GetNumSessions()  const { return m_num_sessions.Load(); }
            ALWAYS_INLINE s32 GetPeakSessions() const { return m_peak_sessions.Load(); }
            ALWAYS_INLINE s32 GetMaxSessions()  const { return m_max_sessions; }

            bool IsLight() const;
            bool IsServerClosed() const;

            /* Overridden virtual functions. */
            virtual void Destroy() override;
            virtual bool IsSignaled() const override;

            Result CreateSession(KClientSession **out);
            Result CreateLightSession(KLightClientSession **out);
    };

}
