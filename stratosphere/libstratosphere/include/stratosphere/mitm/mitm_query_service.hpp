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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>
#include "../ncm.hpp"

template <typename T>
class MitmQueryService : public IServiceObject {
    private:
        enum class CommandId {
            ShouldMitm      = 65000,
        };
    protected:
        void ShouldMitm(Out<bool> should_mitm, u64 process_id, sts::ncm::TitleId title_id) {
            should_mitm.SetValue(T::ShouldMitm(process_id, title_id));
        }
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(MitmQueryService<T>, ShouldMitm),
        };
};