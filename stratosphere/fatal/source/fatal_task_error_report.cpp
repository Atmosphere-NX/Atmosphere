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
 
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <switch.h>
#include <atmosphere/version.h>

#include "fatal_task_error_report.hpp"
#include "fatal_config.hpp"

void ErrorReportTask::EnsureReportDirectories() {
    char path[FS_MAX_PATH];  
    strcpy(path, "sdmc:/atmosphere");
    mkdir(path, S_IRWXU);
    strcat(path, "/fatal_reports");
    mkdir(path, S_IRWXU);
    strcat(path, "/dumps");
    mkdir(path, S_IRWXU);
}

bool ErrorReportTask::GetCurrentTime(u64 *out) {
    *out = 0;
    
    /* Verify that pcv isn't dead. */
    {
        bool has_time_service;
        DoWithSmSession([&]() {
            Handle dummy;
            if (R_SUCCEEDED(smRegisterService(&dummy, "time:s", false, 0x20))) {
                svcCloseHandle(dummy);
                has_time_service = false;
            } else {
                has_time_service = true;
            }
        });
        if (!has_time_service) {
            return false;
        }
    }
    
    /* Try to get the current time. */
    bool success = true;
    DoWithSmSession([&]() {
        success &= R_SUCCEEDED(timeInitialize());
    });
    if (success) {
        success &= R_SUCCEEDED(timeGetCurrentTime(TimeType_LocalSystemClock, out));
        timeExit();
    }
    return success;
}

void ErrorReportTask::SaveReportToSdCard() {
    char file_path[FS_MAX_PATH];
    
    /* Ensure path exists. */
    EnsureReportDirectories();
    
    /* Get a timestamp. */
    u64 timestamp;
    if (!GetCurrentTime(&timestamp)) {
        timestamp = svcGetSystemTick();
    }
    
    /* Open report file. */
    snprintf(file_path, sizeof(file_path) - 1, "sdmc:/atmosphere/fatal_reports/%011lu_%016lx.log", timestamp, this->title_id);
    FILE *f_report = fopen(file_path, "w");
    if (f_report != NULL) {
        ON_SCOPE_EXIT { fclose(f_report); };
        
        fprintf(f_report, "Atmosphère Fatal Report (v1.0):\n");
        fprintf(f_report, "Result:                          0x%X (2%03d-%04d)\n\n", this->ctx->error_code, R_MODULE(this->ctx->error_code), R_DESCRIPTION(this->ctx->error_code));
        fprintf(f_report, "Title ID:                        %016lx\n", this->title_id);
        if (strlen(this->ctx->proc_name)) {
            fprintf(f_report, "Process Name:                    %s\n", this->ctx->proc_name);
        }
        fprintf(f_report, u8"Firmware:                        %s (Atmosphère %u.%u.%u-%s)\n", GetFatalConfig()->firmware_version.display_version, CURRENT_ATMOSPHERE_VERSION, GetAtmosphereGitRevision());
        
        if (this->ctx->cpu_ctx.is_aarch32) {
            fprintf(f_report, "General Purpose Registers:\n");
            for (size_t i = 0; i < NumAarch32Gprs; i++) {
                if (this->ctx->has_gprs[i]) {
                    fprintf(f_report,  "        %3s:                     %08x\n", Aarch32GprNames[i], this->ctx->cpu_ctx.aarch32_ctx.r[i]);
                }
            }
            fprintf(f_report, "    PC:                          %08x\n", this->ctx->cpu_ctx.aarch32_ctx.pc);
            fprintf(f_report, "Start Address:                   %08x\n", this->ctx->cpu_ctx.aarch32_ctx.start_address);
            fprintf(f_report, "Stack Trace:\n");
            for (unsigned int i = 0; i < this->ctx->cpu_ctx.aarch32_ctx.stack_trace_size; i++) {
                fprintf(f_report, "        ReturnAddress[%02u]:       %08x\n", i, this->ctx->cpu_ctx.aarch32_ctx.stack_trace[i]);
            }
        } else {
            fprintf(f_report, "General Purpose Registers:\n");
            for (size_t i = 0; i < NumAarch64Gprs; i++) {
                if (this->ctx->has_gprs[i]) {
                    fprintf(f_report,  "        %3s:                     %016lx\n", Aarch64GprNames[i], this->ctx->cpu_ctx.aarch64_ctx.x[i]);
                }
            }
            fprintf(f_report, "    PC:                          %016lx\n", this->ctx->cpu_ctx.aarch64_ctx.pc);
            fprintf(f_report, "Start Address:                   %016lx\n", this->ctx->cpu_ctx.aarch64_ctx.start_address);
            fprintf(f_report, "Stack Trace:\n");
            for (unsigned int i = 0; i < this->ctx->cpu_ctx.aarch64_ctx.stack_trace_size; i++) {
                fprintf(f_report, "        ReturnAddress[%02u]:       %016lx\n", i, this->ctx->cpu_ctx.aarch64_ctx.stack_trace[i]);
            }
        }
        
    }
    
    if (this->ctx->stack_dump_size) {
        snprintf(file_path, sizeof(file_path) - 1, "sdmc:/atmosphere/fatal_reports/dumps/%011lu_%016lx.bin", timestamp, this->title_id);
        FILE *f_stackdump = fopen(file_path, "wb");
        if (f_stackdump == NULL) { return; }
        ON_SCOPE_EXIT { fclose(f_stackdump); };
        
        fwrite(this->ctx->stack_dump, this->ctx->stack_dump_size, 1, f_stackdump);
    }
}

Result ErrorReportTask::Run() {
    if (this->create_report) {
        /* Here, Nintendo creates an error report with erpt. AMS will not do that. */
    }
    
    /* Save report to SD card. */
    if (!this->ctx->is_creport) {
        SaveReportToSdCard();
    }
    
    /* Signal we're done with our job. */
    eventFire(this->erpt_event);
    
    
    return ResultSuccess;
}