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
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_service_object_base.hpp>
#include <stratosphere/tipc/impl/tipc_impl_template_base.hpp>

namespace ams::tipc {

    namespace impl {

        template<typename Impl>
        class EmplacedImplHolderBaseGetter {
            public:
                using Type = Impl;
        };

        template<typename Impl>
        class EmplacedImplHolder {
            template<typename, typename, typename, typename>
            friend class impl::ImplTemplateBaseT;
            private:
                using Impl2 = typename EmplacedImplHolderBaseGetter<Impl>::Type;
                static_assert(!std::is_abstract<Impl2>::value);
            private:
                Impl2 m_impl;
            private:
                template<typename... Args>
                constexpr explicit EmplacedImplHolder(Args &&... args) : m_impl(std::forward<Args>(args)...) { /* ... */ }
            public:
                static constexpr Impl *GetImplPointer(EmplacedImplHolder *holder) {
                    return std::addressof(holder->m_impl);
                }
        };

    }

    template<typename Interface, typename Impl>
    class ServiceObject final : public impl::ImplTemplateBase<Interface, Interface, impl::EmplacedImplHolder<Impl>, impl::EmplacedImplHolder<Impl>> {
        private:
            using ImplBase = impl::ImplTemplateBase<Interface, Interface, impl::EmplacedImplHolder<Impl>, impl::EmplacedImplHolder<Impl>>;
        public:
            using ImplBase::ImplBase;

            constexpr Impl &GetImpl() { return *impl::EmplacedImplHolder<Impl>::GetImplPointer(this); }
    };

}

