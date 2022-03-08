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
#include <vapours/svc/svc_common.hpp>
#include <vapours/svc/svc_types.hpp>
#include <vapours/svc/svc_definition_macro.hpp>

#define AMS_SVC_KERN_INPUT_HANDLER(TYPE, NAME)  TYPE NAME
#define AMS_SVC_KERN_OUTPUT_HANDLER(TYPE, NAME) TYPE *NAME
#define AMS_SVC_KERN_INPTR_HANDLER(TYPE, NAME)  ::ams::kern::svc::KUserPointer<const TYPE *> NAME
#define AMS_SVC_KERN_OUTPTR_HANDLER(TYPE, NAME) ::ams::kern::svc::KUserPointer<TYPE *> NAME

#define AMS_SVC_USER_INPUT_HANDLER(TYPE, NAME)  TYPE NAME
#define AMS_SVC_USER_OUTPUT_HANDLER(TYPE, NAME) TYPE *NAME
#define AMS_SVC_USER_INPTR_HANDLER(TYPE, NAME)  ::ams::svc::UserPointer<const TYPE *> NAME
#define AMS_SVC_USER_OUTPTR_HANDLER(TYPE, NAME) ::ams::svc::UserPointer<TYPE *> NAME

#define AMS_SVC_FOREACH_USER_DEFINITION(HANDLER, NAMESPACE) AMS_SVC_FOREACH_DEFINITION_IMPL(HANDLER, NAMESPACE, AMS_SVC_USER_INPUT_HANDLER, AMS_SVC_USER_OUTPUT_HANDLER, AMS_SVC_USER_INPTR_HANDLER, AMS_SVC_USER_OUTPTR_HANDLER)
#define AMS_SVC_FOREACH_KERN_DEFINITION(HANDLER, NAMESPACE) AMS_SVC_FOREACH_DEFINITION_IMPL(HANDLER, NAMESPACE, AMS_SVC_KERN_INPUT_HANDLER, AMS_SVC_KERN_OUTPUT_HANDLER, AMS_SVC_KERN_INPTR_HANDLER, AMS_SVC_KERN_OUTPTR_HANDLER)

#define AMS_SVC_DECLARE_FUNCTION_PROTOTYPE(ID, RETURN_TYPE, NAME, ...) \
    RETURN_TYPE NAME(__VA_ARGS__);

namespace ams::svc {

    #define AMS_SVC_DEFINE_ID_ENUM_MEMBER(ID, RETURN_TYPE, NAME, ...) \
    SvcId_##NAME = ID,

    enum SvcId : u32 {
        AMS_SVC_FOREACH_KERN_DEFINITION(AMS_SVC_DEFINE_ID_ENUM_MEMBER, _)
    };

    #undef AMS_SVC_DEFINE_ID_ENUM_MEMBER

}

#ifdef ATMOSPHERE_IS_STRATOSPHERE

namespace ams::svc {

    namespace aarch64::lp64 {

        AMS_SVC_FOREACH_USER_DEFINITION(AMS_SVC_DECLARE_FUNCTION_PROTOTYPE, lp64)

    }

    namespace aarch64::ilp32 {

        AMS_SVC_FOREACH_USER_DEFINITION(AMS_SVC_DECLARE_FUNCTION_PROTOTYPE, ilp32)

    }

    namespace aarch32 {

        AMS_SVC_FOREACH_USER_DEFINITION(AMS_SVC_DECLARE_FUNCTION_PROTOTYPE, ilp32)

    }

}

/* NOTE: Change this to 1 to test the SVC definitions for user-pointer validity. */
#if 0
namespace ams::svc::test {

    namespace impl {

        template<typename... Ts>
        struct Validator {
            private:
                std::array<bool, sizeof...(Ts)> m_valid;
            public:
                constexpr Validator(Ts... args) : m_valid{static_cast<bool>(args)...} { /* ... */ }

                constexpr bool IsValid() const {
                    for (size_t i = 0; i < sizeof...(Ts); i++) {
                        if (!m_valid[i]) {
                            return false;
                        }
                    }
                    return true;
                }
        };

    }


    #define AMS_SVC_TEST_EMPTY_HANDLER(TYPE, NAME)  true
    #define AMS_SVC_TEST_INPTR_HANDLER(TYPE, NAME)  (sizeof(::ams::svc::UserPointer<const TYPE *>) == sizeof(uintptr_t) && std::is_trivially_destructible<::ams::svc::UserPointer<const TYPE *>>::value)
    #define AMS_SVC_TEST_OUTPTR_HANDLER(TYPE, NAME) (sizeof(::ams::svc::UserPointer<TYPE *>) == sizeof(uintptr_t) && std::is_trivially_destructible<::ams::svc::UserPointer<TYPE *>>::value)

    #define AMS_SVC_TEST_VERIFY_USER_POINTERS(ID, RETURN_TYPE, NAME, ...) \
        static_assert(impl::Validator(__VA_ARGS__).IsValid(), "Invalid User Pointer in svc::" #NAME);

    AMS_SVC_FOREACH_DEFINITION_IMPL(AMS_SVC_TEST_VERIFY_USER_POINTERS, lp64,  AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_INPTR_HANDLER, AMS_SVC_TEST_OUTPTR_HANDLER);
    AMS_SVC_FOREACH_DEFINITION_IMPL(AMS_SVC_TEST_VERIFY_USER_POINTERS, ilp32, AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_INPTR_HANDLER, AMS_SVC_TEST_OUTPTR_HANDLER);

    #undef  AMS_SVC_TEST_VERIFY_USER_POINTERS
    #undef  AMS_SVC_TEST_INPTR_HANDLER
    #undef  AMS_SVC_TEST_OUTPTR_HANDLER
    #undef  AMS_SVC_TEST_EMPTY_HANDLER

}
#endif

#endif /* ATMOSPHERE_IS_STRATOSPHERE */

