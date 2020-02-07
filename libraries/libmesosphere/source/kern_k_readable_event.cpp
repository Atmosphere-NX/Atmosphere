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

    bool KReadableEvent::IsSignaled() const {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        return this->is_signaled;
    }

    void KReadableEvent::Destroy() {
        MESOSPHERE_ASSERT_THIS();
        if (this->parent_event) {
            this->parent_event->Close();
        }
    }

    Result KReadableEvent::Signal() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock lk;

        if (!this->is_signaled) {
            this->is_signaled = true;
            this->NotifyAvailable();
        }

        return ResultSuccess();
    }

    Result KReadableEvent::Clear() {
        MESOSPHERE_ASSERT_THIS();

        this->Reset();

        return ResultSuccess();
    }

    Result KReadableEvent::Reset() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock lk;

        R_UNLESS(this->is_signaled, svc::ResultInvalidState());

        this->is_signaled = false;
        return ResultSuccess();
    }

}
