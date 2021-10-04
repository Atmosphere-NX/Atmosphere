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
#include <stratosphere/sf/impl/sf_impl_autogen_interface_macros.hpp>
#include <stratosphere/sf/impl/sf_impl_template_base.hpp>

namespace ams::sf::impl {

    #define AMS_SF_IMPL_DEFINE_IMPL_SYNC_METHOD(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        virtual RETURN AMS_SF_IMPL_SYNC_FUNCTION_NAME(NAME) ARGS override {                                                \
            return ImplGetter::GetImplPointer(static_cast<ImplHolder *>(this))->NAME ARGNAMES;                             \
        }

    #define AMS_SF_DEFINE_INTERFACE_WITH_DEFAULT_BASE(NAMESPACE, INTERFACE, BASE, CMD_MACRO)                                       \
        namespace NAMESPACE {                                                                                                      \
                                                                                                                                   \
            AMS_SF_DEFINE_INTERFACE_IMPL(BASE, INTERFACE, CMD_MACRO)                                                               \
                                                                                                                                   \
        }                                                                                                                          \
                                                                                                                                   \
        namespace ams::sf::impl {                                                                                                  \
                                                                                                                                   \
            template<typename Base, typename ImplHolder, typename ImplGetter, typename Root>                                       \
            class ImplTemplateBaseT<::NAMESPACE::INTERFACE, Base, ImplHolder, ImplGetter, Root> : public Base, public ImplHolder { \
                public:                                                                                                            \
                    template<typename... Args>                                                                                     \
                    constexpr explicit ImplTemplateBaseT(Args &&...args) : ImplHolder(std::forward<Args>(args)...) { /* ... */ }   \
                private:                                                                                                           \
                    CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_IMPL_SYNC_METHOD)                                                      \
            };                                                                                                                     \
                                                                                                                                   \
        }

    #define AMS_SF_DEFINE_INTERFACE(NAMESPACE, INTERFACE, CMD_MACRO) \
        AMS_SF_DEFINE_INTERFACE_WITH_DEFAULT_BASE(NAMESPACE, INTERFACE, ::ams::sf::IServiceObject, CMD_MACRO)

    #define AMS_SF_DEFINE_MITM_INTERFACE(NAMESPACE, INTERFACE, CMD_MACRO) \
        AMS_SF_DEFINE_INTERFACE_WITH_DEFAULT_BASE(NAMESPACE, INTERFACE, ::ams::sf::IMitmServiceObject, CMD_MACRO)

    #define AMS_SF_DEFINE_INTERFACE_WITH_BASE(NAMESPACE, INTERFACE, BASE, CMD_MACRO)                                                                                   \
        namespace NAMESPACE {                                                                                                                                          \
                                                                                                                                                                       \
            AMS_SF_DEFINE_INTERFACE_IMPL(BASE, INTERFACE, CMD_MACRO)                                                                                                   \
                                                                                                                                                                       \
        }                                                                                                                                                              \
                                                                                                                                                                       \
        namespace ams::sf::impl {                                                                                                                                      \
                                                                                                                                                                       \
            template<typename Base, typename ImplHolder, typename ImplGetter, typename Root>                                                                           \
            class ImplTemplateBaseT<::NAMESPACE::INTERFACE, Base, ImplHolder, ImplGetter, Root> : public ImplTemplateBaseT<BASE, Base, ImplHolder, ImplGetter, Root> { \
                private:                                                                                                                                               \
                    using BaseImplTemplateBase = ImplTemplateBaseT<BASE, Base, ImplHolder, ImplGetter, Root>;                                                          \
                public:                                                                                                                                                \
                    template<typename... Args>                                                                                                                         \
                    constexpr explicit ImplTemplateBaseT(Args &&...args) : BaseImplTemplateBase(std::forward<Args>(args)...) { /* ... */ }                             \
                private:                                                                                                                                               \
                    CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_IMPL_SYNC_METHOD)                                                                                          \
            };                                                                                                                                                         \
                                                                                                                                                                       \
        }

}
