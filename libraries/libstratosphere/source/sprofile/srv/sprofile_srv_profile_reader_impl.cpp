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
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_profile_reader_impl.hpp"

namespace ams::sprofile::srv {

    Result ProfileReaderImpl::GetSigned64(sf::Out<s64> out, sprofile::Identifier profile, sprofile::Identifier key) {
        return m_manager->GetSigned64(out.GetPointer(), profile, key);
    }

    Result ProfileReaderImpl::GetUnsigned64(sf::Out<u64> out, sprofile::Identifier profile, sprofile::Identifier key) {
        return m_manager->GetUnsigned64(out.GetPointer(), profile, key);
    }

    Result ProfileReaderImpl::GetSigned32(sf::Out<s32> out, sprofile::Identifier profile, sprofile::Identifier key) {
        return m_manager->GetSigned32(out.GetPointer(), profile, key);
    }

    Result ProfileReaderImpl::GetUnsigned32(sf::Out<u32> out, sprofile::Identifier profile, sprofile::Identifier key) {
        return m_manager->GetUnsigned32(out.GetPointer(), profile, key);
    }

    Result ProfileReaderImpl::GetByte(sf::Out<u8> out, sprofile::Identifier profile, sprofile::Identifier key) {
        return m_manager->GetByte(out.GetPointer(), profile, key);
    }

}
