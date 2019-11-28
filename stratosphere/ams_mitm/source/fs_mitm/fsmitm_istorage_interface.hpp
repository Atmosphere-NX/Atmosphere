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
#include <stratosphere.hpp>

namespace ams::mitm::fs {

    class IStorageInterface : public sf::IServiceObject {
        private:
            enum class CommandId {
                Read         = 0,
                Write        = 1,
                Flush        = 2,
                SetSize      = 3,
                GetSize      = 4,
                OperateRange = 5,
            };
        private:
            std::unique_ptr<ams::fs::IStorage> base_storage;
        public:
            IStorageInterface(ams::fs::IStorage *s) : base_storage(s) { /* ... */ }
            IStorageInterface(std::unique_ptr<ams::fs::IStorage> s) : base_storage(std::move(s)) { /* ... */ }
        private:
            /* Command API. */
            virtual Result Read(s64 offset, const sf::OutNonSecureBuffer &buffer, s64 size) final;
            virtual Result Write(s64 offset, const sf::InNonSecureBuffer &buffer, s64 size) final;
            virtual Result Flush() final;
            virtual Result SetSize(s64 size) final;
            virtual Result GetSize(sf::Out<s64> out) final;
            virtual Result OperateRange(sf::Out<ams::fs::StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) final;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* 1.0.0- */
                MAKE_SERVICE_COMMAND_META(Read),
                MAKE_SERVICE_COMMAND_META(Write),
                MAKE_SERVICE_COMMAND_META(Flush),
                MAKE_SERVICE_COMMAND_META(SetSize),
                MAKE_SERVICE_COMMAND_META(GetSize),

                /* 4.0.0- */
                MAKE_SERVICE_COMMAND_META(OperateRange, hos::Version_400),
            };
    };

}
