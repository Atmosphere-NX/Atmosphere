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
#include <stratosphere/iserviceobject.hpp>

#include "debug.hpp"

enum MitMQueryServiceCommand {
    MQS_Cmd_ShouldMitm = 65000,
    MQS_Cmd_AssociatePidTid = 65001
};

namespace MitMQueryUtils {
    Result get_associated_tid_for_pid(u64 pid, u64 *tid);

    void associate_pid_to_tid(u64 pid, u64 tid); 
}


template <typename T>
class MitMQueryService : public IServiceObject {
    public:
        MitMQueryService<T> *clone() override {
            return new MitMQueryService<T>();
        }
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override {
            Log(armGetTls(), 0x100);
            switch (cmd_id) {
                case MQS_Cmd_ShouldMitm:
                    return WrapIpcCommandImpl<&MitMQueryService::should_mitm>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                case MQS_Cmd_AssociatePidTid:
                    return WrapIpcCommandImpl<&MitMQueryService::associate_pid_tid>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                default:
                    return 0xF601;
            }
            if (cmd_id == 65000) {
                
            } else {
                return 0xF601;
            }
        }
        Result handle_deferred() override {
            /* This service is never deferrable. */
            return 0;
        }
        
    protected:
        std::tuple<Result, u64> should_mitm(u64 pid) {
            u64 should_mitm = 0;
            u64 tid = 0;
            if (R_SUCCEEDED(MitMQueryUtils::get_associated_tid_for_pid(pid, &tid))) {
                should_mitm = T::should_mitm(pid, tid);
            }
            return {0, should_mitm};
        }
        std::tuple<Result> associate_pid_tid(u64 pid, u64 tid) {
            MitMQueryUtils::associate_pid_to_tid(pid, tid);
            return {0x0};
        }
};