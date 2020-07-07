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
#include "erpt_srv_session_impl.hpp"
#include "erpt_srv_report_impl.hpp"
#include "erpt_srv_manager_impl.hpp"
#include "erpt_srv_attachment_impl.hpp"

namespace ams::erpt::srv {

    namespace {

        template<typename Interface, typename Impl>
        ALWAYS_INLINE Result OpenInterface(ams::sf::Out<std::shared_ptr<Interface>> &out) {
            /* Define holder type. */
            using Holder = typename Interface::ImplHolder<Impl>;

            /* Create an interface holder. */
            auto intf = std::shared_ptr<Holder>(new (std::nothrow) Holder);
            R_UNLESS(intf != nullptr, erpt::ResultOutOfMemory());

            /* Return it. */
            out.SetValue(std::move(intf));
            return ResultSuccess();
        }

    }

    Result SessionImpl::OpenReport(ams::sf::Out<std::shared_ptr<erpt::sf::IReport>> out) {
        return OpenInterface<erpt::sf::IReport, ReportImpl>(out);
    }

    Result SessionImpl::OpenManager(ams::sf::Out<std::shared_ptr<erpt::sf::IManager>> out) {
        return OpenInterface<erpt::sf::IManager, ManagerImpl>(out);
    }

    Result SessionImpl::OpenAttachment(ams::sf::Out<std::shared_ptr<erpt::sf::IAttachment>> out) {
        return OpenInterface<erpt::sf::IAttachment, AttachmentImpl>(out);
    }

}
