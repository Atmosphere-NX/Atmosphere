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
#include "spl_secure_monitor_manager.hpp"

namespace ams::spl {

    class GeneralService {
        protected:
            SecureMonitorManager &m_manager;
        public:
            explicit GeneralService(SecureMonitorManager *manager) : m_manager(*manager) { /* ... */ }
        public:
            /* Actual commands. */
            Result GetConfig(sf::Out<u64> out, u32 key) {
                return m_manager.GetConfig(out.GetPointer(), static_cast<spl::ConfigItem>(key));
            }

            Result ModularExponentiate(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod) {
                return m_manager.ModularExponentiate(out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), exp.GetPointer(), exp.GetSize(), mod.GetPointer(), mod.GetSize());
            }

            Result SetConfig(u32 key, u64 value) {
                return m_manager.SetConfig(static_cast<spl::ConfigItem>(key), value);
            }

            Result GenerateRandomBytes(const sf::OutPointerBuffer &out) {
                return m_manager.GenerateRandomBytes(out.GetPointer(), out.GetSize());
            }

            Result IsDevelopment(sf::Out<bool> is_dev) {
                return m_manager.IsDevelopment(is_dev.GetPointer());
            }

            Result SetBootReason(BootReasonValue boot_reason) {
                return m_manager.SetBootReason(boot_reason);
            }

            Result GetBootReason(sf::Out<BootReasonValue> out) {
                return m_manager.GetBootReason(out.GetPointer());
            }
    };
    static_assert(spl::impl::IsIGeneralInterface<GeneralService>);

}
