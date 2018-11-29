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
 
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class FatalEventManager {
    private:
        static constexpr size_t NumFatalEvents = 3;
        
        HosMutex lock;
        size_t events_gotten = 0;
        Event events[NumFatalEvents];
    public:
        FatalEventManager();
        Result GetEvent(Handle *out);
        void SignalEvents();
};

FatalEventManager *GetEventManager();