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
#include "spl_general_service.hpp"

namespace ams::spl {

    class CryptoService : public GeneralService {
        public:
            explicit CryptoService(SecureMonitorManager *manager) : GeneralService(manager) { /* ... */ }
        public:
            virtual ~CryptoService(){
                /* Free any keyslots this service is using. */
                m_manager.DeallocateAesKeySlots(this);
            }
        public:
            /* Actual commands. */
            Result GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option) {
                return m_manager.GenerateAesKek(out_access_key.GetPointer(), key_source, generation, option);
            }

            Result LoadAesKey(s32 keyslot, AccessKey access_key, KeySource key_source) {
                return m_manager.LoadAesKey(keyslot, this, access_key, key_source);
            }

            Result GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source) {
                return m_manager.GenerateAesKey(out_key.GetPointer(), access_key, key_source);
            }

            Result DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option) {
                return m_manager.DecryptAesKey(out_key.GetPointer(), key_source, generation, option);
            }

            Result ComputeCtr(const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr) {
                return m_manager.ComputeCtr(out_buf.GetPointer(), out_buf.GetSize(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize(), iv_ctr);
            }

            Result ComputeCmac(sf::Out<Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf) {
                return m_manager.ComputeCmac(out_cmac.GetPointer(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize());
            }

            Result AllocateAesKeySlot(sf::Out<s32> out_keyslot) {
                return m_manager.AllocateAesKeySlot(out_keyslot.GetPointer(), this);
            }

            Result DeallocateAesKeySlot(s32 keyslot) {
                return m_manager.DeallocateAesKeySlot(keyslot, this);
            }

            Result GetAesKeySlotAvailableEvent(sf::OutCopyHandle out_hnd) {
                out_hnd.SetValue(m_manager.GetAesKeySlotAvailableEvent()->GetReadableHandle(), false);
                return ResultSuccess();
            }
    };
    static_assert(spl::impl::IsICryptoInterface<CryptoService>);

}
