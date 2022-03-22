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
#include <vapours.hpp>
#pragma once

namespace ams::nxboot {

    struct IniKeyValueEntry {
        util::IntrusiveListNode list_node;
        char *key;
        char *value;
    };

    using IniKeyValueList = typename util::IntrusiveListMemberTraits<&IniKeyValueEntry::list_node>::ListType;

    struct IniSection {
        util::IntrusiveListNode list_node;
        IniKeyValueList kv_list;
        char *name;
    };

    using IniSectionList = typename util::IntrusiveListMemberTraits<&IniSection::list_node>::ListType;

    enum ParseIniResult {
        ParseIniResult_Success,
        ParseIniResult_NoFile,
        ParseIniResult_InvalidFormat,
    };

    ParseIniResult ParseIniFile(IniSectionList &out_sections, const char *ini_path);

}