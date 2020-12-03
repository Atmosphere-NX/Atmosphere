/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "erpt_srv_allocator.hpp"
#include "erpt_srv_cipher.hpp"

namespace ams::erpt::srv {

    class ContextRecord;
    class Report;

    class Context : public Allocator, public util::IntrusiveListBaseNode<Context> {
        private:
            const CategoryId category;
            const u32 max_record_count;
            u32 record_count;
            util::IntrusiveListBaseTraits<ContextRecord>::ListType record_list;
        public:
            Context(CategoryId cat, u32 max_records);
            ~Context();

            Result AddCategoryToReport(Report *report);
            Result AddContextToCategory(const ContextEntry *entry, const u8 *data, u32 data_size);
            Result AddContextRecordToCategory(std::unique_ptr<ContextRecord> record);
        public:
            static Result SubmitContext(const ContextEntry *entry, const u8 *data, u32 data_size);
            static Result SubmitContextRecord(std::unique_ptr<ContextRecord> record);
            static Result WriteContextsToReport(Report *report);
    };

}
