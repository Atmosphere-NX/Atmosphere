/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <vapours.hpp>
#include <stratosphere/erpt/erpt_types.hpp>
#include <stratosphere/erpt/sf/erpt_sf_i_report.hpp>
#include <stratosphere/erpt/sf/erpt_sf_i_manager.hpp>
#include <stratosphere/erpt/sf/erpt_sf_i_attachment.hpp>

namespace ams::erpt::sf {

    class ISession : public ams::sf::IServiceObject {
        protected:
            enum class CommandId {
                OpenReport     = 0,
                OpenManager    = 1,
                OpenAttachment = 2,
            };
        public:
            /* Actual commands. */
            virtual Result OpenReport(ams::sf::Out<std::shared_ptr<erpt::sf::IReport>> out) = 0;
            virtual Result OpenManager(ams::sf::Out<std::shared_ptr<erpt::sf::IManager>> out) = 0;
            virtual Result OpenAttachment(ams::sf::Out<std::shared_ptr<erpt::sf::IAttachment>> out) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(OpenReport),
                MAKE_SERVICE_COMMAND_META(OpenManager),
                MAKE_SERVICE_COMMAND_META(OpenAttachment, hos::Version_8_0_0),
            };
    };

}