/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include <stratosphere.hpp>
#include "pm_process_track.hpp"
#include "pm_registration.hpp"

void ProcessTracking::MainLoop(void *arg) {
    /* Make a new waitable manager. */
    MultiThreadedWaitableManager *process_waiter = new MultiThreadedWaitableManager(1, U64_MAX);
    process_waiter->add_waitable(Registration::GetProcessLaunchStartEvent());
    Registration::SetProcessListManager(process_waiter);
    
    /* Service processes. */
    process_waiter->process();
    
    delete process_waiter;
    svcExitThread();
}