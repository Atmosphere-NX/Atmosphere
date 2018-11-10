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
#include "fatal_task_error_report.hpp"

Result ErrorReportTask::Run() {
    if (this->create_report) {
        /* Here, Nintendo creates an error report with erpt. AMS will not do that. */
        /* TODO: Should atmosphere log reports to to the SD card? */
    }
    
    /* Signal we're done with our job. */
    eventFire(this->erpt_event);
    
    return 0;
}