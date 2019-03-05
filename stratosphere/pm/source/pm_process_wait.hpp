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
#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>
#include "pm_registration.hpp"

class ProcessWaiter final : public IWaitable {
    public:
        std::shared_ptr<Registration::Process> process;
        
        ProcessWaiter(std::shared_ptr<Registration::Process> p) : process(p) {
            /* ... */
        }
        
        std::shared_ptr<Registration::Process> GetProcess() { 
            return this->process; 
        }
        
        /* IWaitable */        
        Handle GetHandle() override {
            return this->process->handle;
        }
        
        Result HandleSignaled(u64 timeout) override {
            return Registration::HandleSignaledProcess(this->GetProcess());
        }
};

class ProcessList final {
    private:      
        HosRecursiveMutex mutex;
    public:
        std::vector<std::shared_ptr<Registration::Process>> processes;
        
        auto GetUniqueLock() {
            return std::unique_lock{this->mutex};
        }
};

