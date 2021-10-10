/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "fatal_config.hpp"
#include "fatal_task_error_report.hpp"
#include "fatal_scoped_file.hpp"

namespace ams::fatal::srv {

    namespace {

        /* Helpers. */
        void TryEnsureReportDirectories() {
            fs::EnsureDirectoryRecursively("sdmc:/atmosphere/fatal_reports/dumps");
        }

        bool TryGetCurrentTimestamp(u64 *out) {
            /* Clear output. */
            *out = 0;

            /* Check if we have time service. */
            {
                bool has_time_service = false;
                if (R_FAILED(sm::HasService(std::addressof(has_time_service), sm::ServiceName::Encode("time:s"))) || !has_time_service) {
                    return false;
                }
            }

            /* Try to get the current time. */
            {
                if (R_FAILED(::timeInitialize())) {
                    return false;
                }
                ON_SCOPE_EXIT { ::timeExit(); };

                return R_SUCCEEDED(::timeGetCurrentTime(TimeType_LocalSystemClock, out));
            }
        }

        /* Task definition. */
        class ErrorReportTask : public ITaskWithDefaultStack {
            private:
                void SaveReportToSdCard();
            public:
                virtual Result Run() override;
                virtual const char *GetName() const override {
                    return "WriteErrorReport";
                }
        };

        /* Task globals. */
        ErrorReportTask g_error_report_task;

        /* Task Implementation. */
        void ErrorReportTask::SaveReportToSdCard() {
            char file_path[fs::EntryNameLengthMax + 1];

            /* Try to Ensure path exists. */
            TryEnsureReportDirectories();

            /* Get a timestamp. */
            u64 timestamp;
            if (!TryGetCurrentTimestamp(std::addressof(timestamp))) {
                timestamp = os::GetSystemTick().GetInt64Value();
            }

            /* Open report file. */
            {
                util::SNPrintf(file_path, sizeof(file_path) - 1, "sdmc:/atmosphere/fatal_reports/%011lu_%016lx.log", timestamp, static_cast<u64>(m_context->program_id));
                ScopedFile file(file_path);
                if (file.IsOpen()) {
                    file.WriteFormat("Atmosphère Fatal Report (v1.1):\n");
                    file.WriteFormat("Result:                          0x%X (2%03d-%04d)\n\n", m_context->result.GetValue(), m_context->result.GetModule(), m_context->result.GetDescription());
                    file.WriteFormat("Program ID:                      %016lx\n", static_cast<u64>(m_context->program_id));
                    if (strlen(m_context->proc_name)) {
                        file.WriteFormat("Process Name:                    %s\n", m_context->proc_name);
                    }
                    file.WriteFormat("Firmware:                        %s (Atmosphère %u.%u.%u-%s)\n", GetFatalConfig().GetFirmwareVersion().display_version, ATMOSPHERE_RELEASE_VERSION, ams::GetGitRevision());

                    if (m_context->cpu_ctx.architecture == CpuContext::Architecture_Aarch32) {
                        file.WriteFormat("General Purpose Registers:\n");
                        for (size_t i = 0; i <= aarch32::RegisterName_PC; i++) {
                            if (m_context->cpu_ctx.aarch32_ctx.HasRegisterValue(static_cast<aarch32::RegisterName>(i))) {
                                file.WriteFormat( "        %3s:                     %08x\n", aarch32::CpuContext::RegisterNameStrings[i], m_context->cpu_ctx.aarch32_ctx.r[i]);
                            }
                        }
                        file.WriteFormat("Start Address:                   %08x\n", m_context->cpu_ctx.aarch32_ctx.base_address);
                        file.WriteFormat("Stack Trace:\n");
                        for (unsigned int i = 0; i < m_context->cpu_ctx.aarch32_ctx.stack_trace_size; i++) {
                            file.WriteFormat("        ReturnAddress[%02u]:       %08x\n", i, m_context->cpu_ctx.aarch32_ctx.stack_trace[i]);
                        }
                    } else {
                        file.WriteFormat("General Purpose Registers:\n");
                        for (size_t i = 0; i <= aarch64::RegisterName_PC; i++) {
                            if (m_context->cpu_ctx.aarch64_ctx.HasRegisterValue(static_cast<aarch64::RegisterName>(i))) {
                                file.WriteFormat( "        %3s:                     %016lx\n", aarch64::CpuContext::RegisterNameStrings[i], m_context->cpu_ctx.aarch64_ctx.x[i]);
                            }
                        }
                        file.WriteFormat("Start Address:                   %016lx\n", m_context->cpu_ctx.aarch64_ctx.base_address);
                        file.WriteFormat("Stack Trace:\n");
                        for (unsigned int i = 0; i < m_context->cpu_ctx.aarch64_ctx.stack_trace_size; i++) {
                            file.WriteFormat("        ReturnAddress[%02u]:       %016lx\n", i, m_context->cpu_ctx.aarch64_ctx.stack_trace[i]);
                        }
                    }

                    if (m_context->stack_dump_size != 0) {
                        file.WriteFormat("Stack Dump:                               00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
                        for (size_t i = 0; i < 0x10; i++) {
                            const size_t ofs = i * 0x10;
                            file.WriteFormat("                             %012lx %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                m_context->stack_dump_base + ofs, m_context->stack_dump[ofs + 0], m_context->stack_dump[ofs + 1], m_context->stack_dump[ofs + 2], m_context->stack_dump[ofs + 3], m_context->stack_dump[ofs + 4], m_context->stack_dump[ofs + 5], m_context->stack_dump[ofs + 6], m_context->stack_dump[ofs + 7],
                                m_context->stack_dump[ofs + 8], m_context->stack_dump[ofs + 9], m_context->stack_dump[ofs + 10], m_context->stack_dump[ofs + 11], m_context->stack_dump[ofs + 12], m_context->stack_dump[ofs + 13], m_context->stack_dump[ofs + 14], m_context->stack_dump[ofs + 15]);
                        }
                    }

                    if (m_context->tls_address != 0) {
                        file.WriteFormat("TLS Address:                 %016lx\n", m_context->tls_address);
                        file.WriteFormat("TLS Dump:                                 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
                        for (size_t i = 0; i < 0x10; i++) {
                            const size_t ofs = i * 0x10;
                            file.WriteFormat("                             %012lx %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                m_context->tls_address + ofs, m_context->tls_dump[ofs + 0], m_context->tls_dump[ofs + 1], m_context->tls_dump[ofs + 2], m_context->tls_dump[ofs + 3], m_context->tls_dump[ofs + 4], m_context->tls_dump[ofs + 5], m_context->tls_dump[ofs + 6], m_context->tls_dump[ofs + 7],
                                m_context->tls_dump[ofs + 8], m_context->tls_dump[ofs + 9], m_context->tls_dump[ofs + 10], m_context->tls_dump[ofs + 11], m_context->tls_dump[ofs + 12], m_context->tls_dump[ofs + 13], m_context->tls_dump[ofs + 14], m_context->tls_dump[ofs + 15]);
                        }
                    }
                }
            }

            /* Dump data to file. */
            {
                util::SNPrintf(file_path, sizeof(file_path) - 1, "sdmc:/atmosphere/fatal_reports/dumps/%011lu_%016lx.bin", timestamp, static_cast<u64>(m_context->program_id));
                ScopedFile file(file_path);
                if (file.IsOpen()) {
                    file.Write(m_context->tls_dump, sizeof(m_context->tls_dump));
                    if (m_context->stack_dump_size) {
                        file.Write(m_context->stack_dump, m_context->stack_dump_size);
                    }
                }
            }
        }

        Result ErrorReportTask::Run() {
            if (m_context->generate_error_report) {
                /* Here, Nintendo creates an error report with erpt. AMS will not do that. */
            }

            /* Save report to SD card. */
            if (!m_context->is_creport) {
                this->SaveReportToSdCard();
            }

            /* Signal we're done with our job. */
            m_context->erpt_event->Signal();

            return ResultSuccess();
        }

    }

    ITask *GetErrorReportTask(const ThrowContext *ctx) {
        g_error_report_task.Initialize(ctx);
        return std::addressof(g_error_report_task);
    }

}
