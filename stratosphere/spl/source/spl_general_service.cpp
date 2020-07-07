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
#include <stratosphere.hpp>
#include "spl_api_impl.hpp"
#include "spl_general_service.hpp"

namespace ams::spl {

    Result GeneralService::GetConfig(sf::Out<u64> out, u32 which) {
        return impl::GetConfig(out.GetPointer(), static_cast<spl::ConfigItem>(which));
    }

    Result GeneralService::ModularExponentiate(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod) {
        return impl::ModularExponentiate(out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), exp.GetPointer(), exp.GetSize(), mod.GetPointer(), mod.GetSize());
    }

    Result GeneralService::SetConfig(u32 which, u64 value) {
        return impl::SetConfig(static_cast<spl::ConfigItem>(which), value);
    }

    Result GeneralService::GenerateRandomBytes(const sf::OutPointerBuffer &out) {
        return impl::GenerateRandomBytes(out.GetPointer(), out.GetSize());
    }

    Result GeneralService::IsDevelopment(sf::Out<bool> is_dev) {
        return impl::IsDevelopment(is_dev.GetPointer());
    }

    Result GeneralService::SetBootReason(BootReasonValue boot_reason) {
        return impl::SetBootReason(boot_reason);
    }

    Result GeneralService::GetBootReason(sf::Out<BootReasonValue> out) {
        return impl::GetBootReason(out.GetPointer());
    }

}
