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
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/sf/sf_out.hpp>
#include <stratosphere/sf/sf_shared_object.hpp>

namespace ams::sf {

    class IServiceObject : public ISharedObject {
        public:
            virtual ~IServiceObject() { /* ... */ }
    };

    template<typename T>
    concept IsServiceObject = std::derived_from<T, IServiceObject>;

    class IMitmServiceObject : public IServiceObject {
        public:
            virtual ~IMitmServiceObject() { /* ... */ }
    };

    class MitmServiceImplBase {
        protected:
            std::shared_ptr<::Service> m_forward_service;
            sm::MitmProcessInfo m_client_info;
        public:
            MitmServiceImplBase(std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &c) : m_forward_service(std::move(s)), m_client_info(c) { /* ... */ }
    };

    template<typename T>
    concept IsMitmServiceObject = IsServiceObject<T> && std::derived_from<T, IMitmServiceObject>;

    template<typename T>
    concept IsMitmServiceImpl = requires (std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &c) {
        { T(std::forward<std::shared_ptr<::Service>>(s), c) };
        { T::ShouldMitm(c) } -> std::same_as<bool>;
    };

}
