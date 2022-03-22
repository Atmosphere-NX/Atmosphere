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
#include "htclow_json.hpp"

namespace ams::htclow::ctrl {

    namespace {

        constexpr const char ChannelKey[] = "Chan";
        constexpr const char ProtocolVersionKey[] = "Prot";

    }

    bool JsonHandler::Key(const Ch *str, rapidjson::SizeType len, bool copy) {
        AMS_UNUSED(len, copy);

        if (m_state == State::ParseObject) {
            if (!util::Strncmp(str, ChannelKey, sizeof(ChannelKey))) {
                m_state = State::ParseServiceChannels;
            }
            if (!util::Strncmp(str, ProtocolVersionKey, sizeof(ProtocolVersionKey))) {
                m_state = State::ParseProtocolVersion;
            }
        }
        return true;
    }

    bool JsonHandler::Uint(unsigned val) {
        if (m_state == State::ParseProtocolVersion) {
            *m_version = val;
        }
        return true;
    }

    bool JsonHandler::String(const Ch *str, rapidjson::SizeType len, bool copy) {
        AMS_UNUSED(len, copy);

        if (m_state == State::ParseServiceChannelsArray && *m_num_strings < m_max_strings) {
            m_strings[(*m_num_strings)++] = str;
        }
        return true;
    }

    bool JsonHandler::StartObject() {
        if (m_state == State::Begin) {
            m_state = State::ParseObject;
        }
        return true;
    }

    bool JsonHandler::EndObject(rapidjson::SizeType) {
        m_state = State::End;
        return true;
    }

    bool JsonHandler::StartArray() {
        if (m_state == State::ParseServiceChannels) {
            m_state = State::ParseServiceChannelsArray;
        }
        return true;
    }

    bool JsonHandler::EndArray(rapidjson::SizeType len) {
        if (m_state == State::ParseServiceChannelsArray && len) {
            m_state = State::ParseObject;
        }
        return true;
    }

}
