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

#pragma once
#include "sf_common.hpp"
#include "sf_out.hpp"

namespace ams::sf {

    class IServiceObject {
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
            std::shared_ptr<::Service> forward_service;
            sm::MitmProcessInfo client_info;
        public:
            MitmServiceImplBase(std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &c) : forward_service(std::move(s)), client_info(c) { /* ... */ }
    };

    template<typename T>
    concept IsMitmServiceObject = IsServiceObject<T> && std::derived_from<T, IMitmServiceObject>;

    template<typename T>
    concept IsMitmServiceImpl = requires (std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &c) {
        { T(std::forward<std::shared_ptr<::Service>>(s), c) };
        { T::ShouldMitm(c) } -> std::same_as<bool>;
    };

    template<typename Interface, typename Impl, typename... Arguments>
        requires std::constructible_from<Impl, Arguments...>
    constexpr ALWAYS_INLINE std::shared_ptr<typename Interface::ImplHolder<Impl>> MakeShared(Arguments &&... args) {
        return std::make_shared<typename Interface::ImplHolder<Impl>>(std::forward<Arguments>(args)...);
    }

    template<typename Interface, typename Impl, typename... Arguments>
        requires (std::constructible_from<Impl, Arguments...> && std::derived_from<Impl, std::enable_shared_from_this<Impl>>)
    constexpr ALWAYS_INLINE std::shared_ptr<typename Interface::ImplSharedPointer<Impl>> MakeShared(Arguments &&... args) {
        return std::make_shared<typename Interface::ImplSharedPointer<Impl>>(std::make_shared<Impl>(std::forward<Arguments>(args)...));
    }

    template<typename Interface, typename Impl>
    constexpr ALWAYS_INLINE std::shared_ptr<typename Interface::ImplPointer<Impl>> GetSharedPointerTo(Impl *impl) {
        return std::make_shared<typename Interface::ImplPointer<Impl>>(impl);
    }

    template<typename Interface, typename Impl>
    constexpr ALWAYS_INLINE std::shared_ptr<typename Interface::ImplPointer<Impl>> GetSharedPointerTo(Impl &impl) {
        return GetSharedPointerTo<Interface, Impl>(std::addressof(impl));
    }

}