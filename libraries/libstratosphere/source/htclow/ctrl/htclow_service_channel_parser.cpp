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
#include "htclow_service_channel_parser.hpp"

namespace ams::htclow::ctrl {

    namespace {

        constexpr auto JsonParseFlags = rapidjson::kParseTrailingCommasFlag | rapidjson::kParseInsituFlag;

        void ParseBody(s16 *out_version, const char **out_channels, int *out_num_channels, int max_channels, void *str, size_t str_size) {
            AMS_UNUSED(str_size);

            /* Create JSON handler. */
            JsonHandler json_handler(out_version, out_channels, out_num_channels, max_channels);

            /* Create reader. */
            rapidjson::Reader json_reader;

            /* Create stream. */
            rapidjson::InsituStringStream json_stream(static_cast<char *>(str));

            /* Parse the json. */
            json_reader.Parse<JsonParseFlags>(json_stream, json_handler);
        }

        constexpr bool IsNumericCharacter(char c) {
            return '0' <= c && c <= '9';
        }

        constexpr bool IsValidCharacter(char c) {
            return IsNumericCharacter(c) || c == ':';
        }

        bool IntegerToModuleId(ModuleId *out, int id) {
            switch (id) {
                case 0:
                case 1:
                case 3:
                case 4:
                    *out = static_cast<ModuleId>(id);
                    return true;
                default:
                    return false;
            }
        }

        bool StringToChannel(impl::ChannelInternalType *out, char *str, size_t size) {
            enum class State {
                Begin,
                ModuleId,
                Sep1,
                Reserved,
                Sep2,
                ChannelId
            };

            State state = State::Begin;

            const char * cur = nullptr;

            const char * const str_end = str + size;
            while (str < str_end && IsValidCharacter(*str)) {
                const char c = *str;

                switch (state) {
                    case State::Begin:
                        if (IsNumericCharacter(c)) {
                            cur   = str;
                            state = State::ModuleId;
                        }
                        break;
                    case State::ModuleId:
                        if (c == ':') {
                            *str = 0;
                            if (!IntegerToModuleId(std::addressof(out->module_id), atoi(cur))) {
                                return false;
                            }
                            state = State::Sep1;
                        } else if (!IsNumericCharacter(c)) {
                            return false;
                        }
                        break;
                    case State::Sep1:
                        if (IsNumericCharacter(c)) {
                            cur   = str;
                            state = State::Reserved;
                        }
                        break;
                    case State::Reserved:
                        if (c == ':') {
                            *str = 0;
                            out->reserved = 0;
                            state = State::Sep2;
                        } else if (!IsNumericCharacter(c)) {
                            return false;
                        }
                        break;
                    case State::Sep2:
                        if (IsNumericCharacter(c)) {
                            cur   = str;
                            state = State::ChannelId;
                        }
                        break;
                    case State::ChannelId:
                        if (!IsNumericCharacter(c)) {
                            return false;
                        }
                        break;
                }

                ++str;
            }

            if (str != str_end) {
                return false;
            }

            out->channel_id = atoi(cur);

            return true;
        }

    }

    void ParseServiceChannel(s16 *out_version, impl::ChannelInternalType *out_channels, int *out_num_channels, int max_channels, void *str, size_t str_size) {
        /* Parse the JSON. */
        const char *channel_strs[0x20];
        int num_channels = 0;
        ParseBody(out_version, channel_strs, std::addressof(num_channels), util::size(channel_strs), str, str_size);

        /* Parse the channel strings. */
        char * const str_end = static_cast<char *>(str) + str_size;
        int parsed_channels = 0;
        for (auto i = 0; i < num_channels && i < max_channels; ++i) {
            impl::ChannelInternalType channel;
            if (StringToChannel(std::addressof(channel), const_cast<char *>(channel_strs[i]), util::Strnlen(channel_strs[i], str_end - channel_strs[i]))) {
                out_channels[parsed_channels++] = channel;
            }
        }

        *out_num_channels = parsed_channels;
    }

}
