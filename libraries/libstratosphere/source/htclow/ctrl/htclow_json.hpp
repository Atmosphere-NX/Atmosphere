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
#pragma once
#include <stratosphere.hpp>

namespace ams::htclow::ctrl {

    class JsonHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<char>, JsonHandler>{
        private:
            enum class State {
                Begin                     = 0,
                ParseObject               = 1,
                ParseServiceChannels      = 2,
                ParseServiceChannelsArray = 3,
                ParseProtocolVersion      = 4,
                End                       = 5,
            };
        private:
            State m_state;
            s16 *m_version;
            const char **m_strings;
            int *m_num_strings;
            int m_max_strings;
        public:
            JsonHandler(s16 *vers, const char **str, int *ns, int max) : m_state(State::Begin), m_version(vers), m_strings(str), m_num_strings(ns), m_max_strings(max) { /* ... */ }

            bool Key(const Ch *str, rapidjson::SizeType len, bool copy);
            bool Uint(unsigned);
            bool String(const Ch *, rapidjson::SizeType, bool);

            bool StartObject();
            bool EndObject(rapidjson::SizeType);
            bool StartArray();
            bool EndArray(rapidjson::SizeType);
    };

}
