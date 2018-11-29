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
#include "fatal_task.hpp"

class PowerControlTask : public IFatalTask {
    private:
        Event *erpt_event;
        Event *battery_event;
    private:
        bool TryShutdown();
        void MonitorBatteryState();
    public:
        PowerControlTask(FatalThrowContext *ctx, u64 title_id, Event *er_evt, Event *bt_evt) : IFatalTask(ctx, title_id), erpt_event(er_evt), battery_event(bt_evt) { }
        virtual Result Run() override;
        virtual const char *GetName() const override {
            return "PowerControlTask";
        }
};

class PowerButtonObserveTask : public IFatalTask {
    private:
        Event *erpt_event;
    private:
        void WaitForPowerButton();
    public:
        PowerButtonObserveTask(FatalThrowContext *ctx, u64 title_id, Event *er_evt) : IFatalTask(ctx, title_id), erpt_event(er_evt) { }
        virtual Result Run() override;
        virtual const char *GetName() const override {
            return "PowerButtonObserveTask";
        }
};

class StateTransitionStopTask : public IFatalTask {
    public:
        StateTransitionStopTask(FatalThrowContext *ctx, u64 title_id) : IFatalTask(ctx, title_id) { }
        virtual Result Run() override;
        virtual const char *GetName() const override {
            return "StateTransitionStopTask";
        }
};