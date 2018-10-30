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

enum MitmQueryServiceCommand {
    MQS_Cmd_ShouldMitm = 65000,
    MQS_Cmd_AssociatePidTid = 65001
};

namespace MitmQueryUtils {
    Result GetAssociatedTidForPid(u64 pid, u64 *tid);

    void AssociatePidToTid(u64 pid, u64 tid); 
}

template <typename T>
class MitmQueryService : public IServiceObject {
    protected:
        void ShouldMitm(Out<bool> should_mitm, u64 pid) {
            should_mitm.SetValue(false);
            u64 tid = 0;
            if (R_SUCCEEDED(MitmQueryUtils::GetAssociatedTidForPid(pid, &tid))) {
                should_mitm.SetValue(T::ShouldMitm(pid, tid));
            }
        }
        void AssociatePidToTid(u64 pid, u64 tid) {
            MitmQueryUtils::AssociatePidToTid(pid, tid);
        }
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<MQS_Cmd_ShouldMitm, &MitmQueryService<T>::ShouldMitm>(),
            MakeServiceCommandMeta<MQS_Cmd_AssociatePidTid, &MitmQueryService<T>::AssociatePidToTid>(),
        };
};