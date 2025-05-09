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
#include "erpt_srv_forced_shutdown.hpp"
#include "erpt_srv_context.hpp"
#include "erpt_srv_context_record.hpp"
#include "erpt_srv_reporter.hpp"
#include "erpt_srv_stream.hpp"

namespace ams::erpt::srv {

    namespace {

        constexpr u32 ForcedShutdownContextBufferSize = 1_KB;

        constexpr u32 ForcedShutdownContextVersion = 1;

        struct ForcedShutdownContextHeader {
            u32 version;
            u32 num_contexts;
        };
        static_assert(sizeof(ForcedShutdownContextHeader) == 8);

        struct ForcedShutdownContextEntry {
            u32 version;
            CategoryId category;
            u32 field_count;
            u32 array_buffer_size;
        };
        static_assert(sizeof(ForcedShutdownContextEntry) == 16);

        os::Event g_forced_shutdown_update_event(os::EventClearMode_ManualClear);

        constinit ContextEntry g_forced_shutdown_contexts[] = {
            { .category = CategoryId_RunningApplicationInfo,   },
            { .category = CategoryId_RunningAppletInfo,        },
            { .category = CategoryId_FocusedAppletHistoryInfo, },
        };

        bool IsForceShutdownDetected() {
            fs::DirectoryEntryType entry_type;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), ForcedShutdownContextFileName));
        }

        Result CreateForcedShutdownContext() {
            /* Create the context. */
            {
                /* Create the stream. */
                Stream stream;
                R_TRY(stream.OpenStream(ForcedShutdownContextFileName, StreamMode_Write, 0));

                /* Write a context header. */
                const ForcedShutdownContextHeader header = { .version = ForcedShutdownContextVersion, .num_contexts = 0, };
                R_TRY(stream.WriteStream(reinterpret_cast<const u8 *>(std::addressof(header)), sizeof(header)));
            }

            /* Commit the context. */
            R_TRY(Stream::CommitStream());

            R_SUCCEED();
        }

        Result CreateReportForForcedShutdown() {
            /* Create a new context record. */
            /* NOTE: Nintendo does not check that this allocation succeeds. */
            auto record = std::make_unique<ContextRecord>(CategoryId_ErrorInfo);

            /* Create error code for the report. */
            char error_code_str[err::ErrorCode::StringLengthMax];
            err::GetErrorCodeString(error_code_str, sizeof(error_code_str), err::ConvertResultToErrorCode(err::ResultForcedShutdownDetected()));

            /* Add error code to the context. */
            R_TRY(record->Add(FieldId_ErrorCode, error_code_str, std::strlen(error_code_str)));

            /* Create report. */
            R_TRY(Reporter::CreateReport(ReportType_Invisible, ResultSuccess(), std::move(record), nullptr, nullptr, 0, erpt::srv::MakeNoCreateReportOptionFlags(), nullptr));

            R_SUCCEED();
        }

        Result LoadForcedShutdownContext() {
            /* Create the stream to read the context. */
            Stream stream;
            R_TRY(stream.OpenStream(ForcedShutdownContextFileName, StreamMode_Read, ForcedShutdownContextBufferSize));

            /* Read the header. */
            u32 read_size;
            ForcedShutdownContextHeader header;
            R_TRY(stream.ReadStream(std::addressof(read_size), reinterpret_cast<u8 *>(std::addressof(header)), sizeof(header)));

            /* Validate the header. */
            R_SUCCEED_IF(read_size != sizeof(header));
            R_SUCCEED_IF(ForcedShutdownContextVersion);
            R_SUCCEED_IF(header.num_contexts == 0);

            /* Read out the contexts. */
            for (u32 i = 0; i < header.num_contexts; ++i) {
                /* Read the context entry header. */
                ForcedShutdownContextEntry entry_header;
                R_TRY(stream.ReadStream(std::addressof(read_size), reinterpret_cast<u8 *>(std::addressof(entry_header)), sizeof(entry_header)));

                if (read_size != sizeof(entry_header)) {
                    break;
                }

                if (entry_header.field_count == 0) {
                    continue;
                }

                /* Read the saved data into a context entry. */
                ContextEntry ctx = {
                    .version      = entry_header.version,
                    .field_count  = entry_header.field_count,
                    .category     = entry_header.category,
                };

                /* Check that the field count is valid. */
                AMS_ABORT_UNLESS(entry_header.field_count <= util::size(ctx.fields));

                /* Read the fields. */
                R_TRY(stream.ReadStream(std::addressof(read_size), reinterpret_cast<u8 *>(std::addressof(ctx.fields)), entry_header.field_count * sizeof(ctx.fields[0])));
                if (read_size != entry_header.field_count * sizeof(ctx.fields[0])) {
                    break;
                }

                /* Allocate an array buffer. */
                u8 *array_buffer = static_cast<u8 *>(Allocate(entry_header.array_buffer_size));
                if (array_buffer == nullptr) {
                    break;
                }
                ON_SCOPE_EXIT { Deallocate(array_buffer); };

                /* Read the array buffer data. */
                R_TRY(stream.ReadStream(std::addressof(read_size), array_buffer, entry_header.array_buffer_size));
                if (read_size != entry_header.array_buffer_size) {
                    break;
                }

                /* Create a record for the context. */
                auto record = std::make_unique<ContextRecord>();
                if (record == nullptr) {
                    break;
                }

                /* Initialize the record. */
                R_TRY(record->Initialize(std::addressof(ctx), array_buffer, entry_header.array_buffer_size));

                /* Submit the record. */
                R_TRY(Context::SubmitContextRecord(std::move(record)));
            }

            R_SUCCEED();
        }

        u32 GetForcedShutdownContextCount() {
            u32 count = 0;
            for (const auto &ctx : g_forced_shutdown_contexts) {
                if (ctx.field_count != 0) {
                    ++count;
                }
            }
            return count;
        }

        Result SaveForcedShutdownContextImpl() {
            /* Save context to file. */
            {
                /* Create the stream to write the context. */
                Stream stream;
                R_TRY(stream.OpenStream(ForcedShutdownContextFileName, StreamMode_Write, ForcedShutdownContextBufferSize));

                /* Write a context header. */
                const ForcedShutdownContextHeader header = { .version = ForcedShutdownContextVersion, .num_contexts = GetForcedShutdownContextCount(), };
                R_TRY(stream.WriteStream(reinterpret_cast<const u8 *>(std::addressof(header)), sizeof(header)));

                /* Write each context. */
                for (const auto &ctx : g_forced_shutdown_contexts) {
                    /* If the context has no fields, continue. */
                    if (ctx.field_count == 0) {
                        continue;
                    }

                    /* Write a context entry header. */
                    const ForcedShutdownContextEntry entry_header = {
                        .version           = ctx.version,
                        .category          = ctx.category,
                        .field_count       = ctx.field_count,
                        .array_buffer_size = ctx.array_buffer_size,
                    };
                    R_TRY(stream.WriteStream(reinterpret_cast<const u8 *>(std::addressof(entry_header)), sizeof(entry_header)));

                    /* Write all fields. */
                    for (u32 i = 0; i < ctx.field_count; ++i) {
                        R_TRY(stream.WriteStream(reinterpret_cast<const u8 *>(ctx.fields + i), sizeof(ctx.fields[0])));
                    }

                    /* Write the array buffer. */
                    R_TRY(stream.WriteStream(ctx.array_buffer, ctx.array_buffer_size));
                }
            }

            /* Commit the context. */
            R_TRY(Stream::CommitStream());

            R_SUCCEED();
        }

    }

    os::Event *GetForcedShutdownUpdateEvent() {
        return std::addressof(g_forced_shutdown_update_event);
    }

    void InitializeForcedShutdownDetection() {
        /* Check if the forced shutdown context exists; if it doesn't, we should create an empty one. */
        if (!IsForceShutdownDetected()) {
            /* NOTE: Nintendo does not check result here. */
            CreateForcedShutdownContext();
            return;
        }

        /* Load the forced shutdown context. */
        /* NOTE: Nintendo does not check that this succeeds. */
        LoadForcedShutdownContext();

        /* Create report for the forced shutdown. */
        /* NOTE: Nintendo does not check that this succeeds. */
        CreateReportForForcedShutdown();

        /* Clear the forced shutdown categories. */
        /* NOTE: Nintendo does not check that this succeeds. */
        Context::ClearContext(CategoryId_RunningApplicationInfo);
        Context::ClearContext(CategoryId_RunningAppletInfo);
        Context::ClearContext(CategoryId_FocusedAppletHistoryInfo);

        /* Save the forced shutdown context. */
        /* NOTE: Nintendo does not check that this succeeds. */
        SaveForcedShutdownContext();
    }

    void FinalizeForcedShutdownDetection() {
        /* Try to delete the context. */
        const Result result = Stream::DeleteStream(ForcedShutdownContextFileName);
        if (!fs::ResultPathNotFound::Includes(result)) {
            /* We must have succeeded, if the file existed. */
            R_ABORT_UNLESS(result);

            /* Commit the deletion. */
            R_ABORT_UNLESS(Stream::CommitStream());
        }
    }

    void SaveForcedShutdownContext() {
        /* NOTE: Nintendo does not check that saving the report succeeds. */
        SaveForcedShutdownContextImpl();
    }

    void SubmitContextForForcedShutdownDetection(const ContextEntry *entry, const u8 *data, u32 data_size) {
        /* If the context entry matches one of our tracked categories, update our stored category. */
        for (auto &ctx : g_forced_shutdown_contexts) {
            /* Check for a match. */
            if (ctx.category != entry->category) {
                continue;
            }

            /* If we have an existing array buffer, free it. */
            if (ctx.array_buffer != nullptr) {
                Deallocate(ctx.array_buffer);
                ctx.array_buffer      = nullptr;
                ctx.array_buffer_size = 0;
                ctx.array_free_count  = 0;
            }

            /* Copy in the context. */
            ctx = *entry;

            /* Add the submitted data. */
            if (data != nullptr && data_size > 0) {
                /* Allocate new array buffer. */
                ctx.array_buffer = static_cast<u8 *>(Allocate(data_size));
                if (ctx.array_buffer == nullptr) {
                    /* We failed to allocate; this is okay, but clear our field count. */
                    ctx.field_count = 0;
                    break;
                }

                /* Copy in the data. */
                std::memcpy(ctx.array_buffer, data, data_size);

                /* Set buffer extents. */
                ctx.array_buffer_size = data_size;
                ctx.array_free_count  = 0;
            } else {
                ctx.array_buffer      = nullptr;
                ctx.array_buffer_size = 0;
                ctx.array_free_count  = 0;
            }

            /* Signal, to notify that we had an update. */
            g_forced_shutdown_update_event.Signal();

            /* We're done processing, since we found a match. */
            break;
        }
    }

    Result InvalidateForcedShutdownDetection() {
        /* Delete the forced shutdown context. */
        R_TRY(Stream::DeleteStream(ForcedShutdownContextFileName));

        /* Commit the deletion. */
        R_TRY(Stream::CommitStream());

        R_SUCCEED();
    }


}
