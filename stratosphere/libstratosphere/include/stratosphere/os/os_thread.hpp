/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>

namespace sts::os {

    class Thread {
        private:
            ::Thread thr = {};
        public:
            Thread() {}

            Result Initialize(ThreadFunc entry, void *arg, size_t stack_sz, int prio, int cpuid = -2) {
                return threadCreate(&this->thr, entry, arg, stack_sz, prio, cpuid);
            }

            Handle GetHandle() const {
                return this->thr.handle;
            }

            Result Start() {
                return threadStart(&this->thr);
            }

            Result Join() {
                R_TRY(threadWaitForExit(&this->thr));
                R_TRY(threadClose(&this->thr));
                return ResultSuccess;
            }

            Result CancelSynchronization() {
                return svcCancelSynchronization(this->thr.handle);
            }
    };

}
