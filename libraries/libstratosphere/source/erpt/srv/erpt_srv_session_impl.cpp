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
#include "erpt_srv_session_impl.hpp"
#include "erpt_srv_report_impl.hpp"
#include "erpt_srv_manager_impl.hpp"
#include "erpt_srv_attachment_impl.hpp"

namespace ams::erpt::srv {

    extern ams::sf::ExpHeapAllocator g_sf_allocator;

    namespace {

        template<typename Interface, typename Impl>
        ALWAYS_INLINE Result OpenInterface(ams::sf::Out<ams::sf::SharedPointer<Interface>> &out) {
            /* Create an interface holder. */
            auto intf = ams::sf::ObjectFactory<ams::sf::ExpHeapAllocator::Policy>::CreateSharedEmplaced<Interface, Impl>(std::addressof(g_sf_allocator));
            R_UNLESS(intf != nullptr, erpt::ResultOutOfMemory());

            /* Return it. */
            out.SetValue(intf);
            return ResultSuccess();
        }

    }

    Result SessionImpl::OpenReport(ams::sf::Out<ams::sf::SharedPointer<erpt::sf::IReport>> out) {
        return OpenInterface<erpt::sf::IReport, ReportImpl>(out);
    }

    Result SessionImpl::OpenManager(ams::sf::Out<ams::sf::SharedPointer<erpt::sf::IManager>> out) {
        return OpenInterface<erpt::sf::IManager, ManagerImpl>(out);
    }

    Result SessionImpl::OpenAttachment(ams::sf::Out<ams::sf::SharedPointer<erpt::sf::IAttachment>> out) {
        return OpenInterface<erpt::sf::IAttachment, AttachmentImpl>(out);
    }

}
