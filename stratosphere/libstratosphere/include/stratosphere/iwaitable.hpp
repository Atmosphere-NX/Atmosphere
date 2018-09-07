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

#include "waitablemanagerbase.hpp"

class WaitableManager;

class IWaitable {
    private:
        u64 wait_priority = 0;
        bool is_deferred = false;
        WaitableManagerBase *manager;
    public:
        virtual ~IWaitable() { }
        
        virtual void handle_deferred() = 0;
        virtual Handle get_handle() = 0;
        virtual Result handle_signaled(u64 timeout) = 0;
                
        WaitableManager *get_manager() {
            return (WaitableManager *)this->manager;
        }
        
        void set_manager(WaitableManagerBase *m) {
            this->manager = m;
        }
        
        void update_priority() {
            if (manager) {
                this->wait_priority = this->manager->get_priority();
            }
        }
        
        bool get_deferred() {
            return this->is_deferred;
        }
        
        void set_deferred(bool d) {
            this->is_deferred = d;
        }
        
        static bool compare(IWaitable *a, IWaitable *b) {
            return (a->wait_priority < b->wait_priority) && !a->is_deferred;
        }
};

#include "waitablemanager.hpp"