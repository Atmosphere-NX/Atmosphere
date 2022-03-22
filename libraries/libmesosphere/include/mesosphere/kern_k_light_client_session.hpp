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

    class KLightSession;

    class KLightClientSession final : public KAutoObject {
        MESOSPHERE_AUTOOBJECT_TRAITS(KLightClientSession, KAutoObject);
        private:
            KLightSession *m_parent;
        public:
            explicit KLightClientSession() { /* ... */ }

            void Initialize(KLightSession *parent) {
                /* Set member variables. */
                m_parent = parent;
            }

            virtual void Destroy() override;

            constexpr const KLightSession *GetParent() const { return m_parent; }

            Result SendSyncRequest(u32 *data);

            void OnServerClosed();
    };

}
