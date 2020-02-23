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
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_linked_list.hpp>
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
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_thread.hpp>

namespace ams::kern {

    class KSynchronization {
        private:
            friend class KSynchronizationObject;
        public:
            constexpr KSynchronization() { /* ... */ }

            Result Wait(s32 *out_index, KSynchronizationObject **objects, const s32 num_objects, s64 timeout);
        private:
            void OnAvailable(KSynchronizationObject *object);
            void OnAbort(KSynchronizationObject *object, Result abort_reason);
    };

}
