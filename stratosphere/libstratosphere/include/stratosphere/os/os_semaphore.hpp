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

namespace ams::os {

    class Semaphore {
        NON_COPYABLE(Semaphore);
        NON_MOVEABLE(Semaphore);
        private:
            ::Semaphore s;
        public:
            Semaphore() {
                semaphoreInit(&s, 0);
            }

            Semaphore(u64 c) {
                semaphoreInit(&s, c);
            }

            void Signal() {
                semaphoreSignal(&s);
            }

            void Wait() {
                semaphoreWait(&s);
            }

            bool TryWait() {
                return semaphoreTryWait(&s);
            }
    };

}
