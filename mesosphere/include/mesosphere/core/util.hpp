#pragma once

#include <utility>
#include <array>
#include <boost/assert.hpp>

#include <mesosphere/core/types.hpp>

/*
    Boost doesn't provide get_parent_from members for arrays so we have to implement this manually
    for arrays, for gcc at leadt.

    Thanks fincs.
*/

#define kassert(cond) ((void)(cond))

namespace mesosphere
{

namespace
{
template <typename ClassT, typename MemberT>
union __my_offsetof {
    const MemberT ClassT::* ptr;
    iptr offset;
};

// Thanks neobrain
template<typename T, size_t N, typename... Args, size_t... Indexes>
static constexpr std::array<T, N> MakeArrayOfHelper(Args&&... args, std::index_sequence<Indexes...>) {
    // There are two parameter pack expansions here:
    // * The inner expansion is over "t"
    // * The outer expansion is over "Indexes"
    //
    // This function will always be called with sizeof...(Indexes) == N,
    // so the outer expansion generates exactly N copies of the constructor call
    return std::array<T, N> { ((void)Indexes, T { args... })... };
}

// Thanks neobrain
template<typename T, typename F, size_t N, typename... Args, size_t... Indexes>
static constexpr std::array<T, N> MakeArrayWithFactorySequenceOfHelper(Args&&... args, std::index_sequence<Indexes...>) {
    return std::array<T, N> { T { F{}(std::integral_constant<size_t, Indexes>{}), args... }... };
}
}

namespace detail
{

template <typename ClassT, typename MemberT, size_t N>
constexpr ClassT* GetParentFromArrayMember(MemberT* member, size_t index, const MemberT (ClassT::* ptr)[N]) noexcept {
    member -= index;
    return (ClassT*)((iptr)member - __my_offsetof<ClassT,MemberT[N]> { ptr }.offset);
    return nullptr;
}

template <typename ClassT, typename MemberT, size_t N>
constexpr const ClassT* GetParentFromArrayMember(const MemberT* member, size_t index, const MemberT (ClassT::* ptr)[N]) noexcept {
    member -= index;
    return (const ClassT*)((iptr)member - __my_offsetof<ClassT,MemberT[N]> { ptr }.offset);
    return nullptr;
}

template <typename ClassT, typename MemberT>
constexpr ClassT* GetParentFromMember(MemberT* member, const MemberT ClassT::* ptr) noexcept {
    return (ClassT*)((iptr)member - __my_offsetof<ClassT, MemberT> { ptr }.offset);
    return nullptr;
}

template <typename ClassT, typename MemberT>
constexpr const ClassT* GetParentFromMember(const MemberT* member, const MemberT ClassT::* ptr) noexcept {
    return (const ClassT*)((iptr)member - __my_offsetof<ClassT, MemberT> { ptr }.offset);
    return nullptr;
}

template<typename T, size_t N, typename... Args>
constexpr std::array<T, N> MakeArrayOf(Args&&... args) {
    return MakeArrayOfHelper<T, N, Args...>(std::forward<Args>(args)..., std::make_index_sequence<N>{});
}

template<typename T, typename F, size_t N, typename... Args>
constexpr std::array<T, N> MakeArrayWithFactorySequenceOf(Args&&... args) {
    return MakeArrayWithFactorySequenceOfHelper<T, F, N, Args...>(std::forward<Args>(args)..., std::make_index_sequence<N>{});
}

/// Sequence of two distinc powers of 2
constexpr ulong A038444(ulong n)
{
    if (n == 0) {
        return 3;
    }

    ulong v = A038444(n - 1);
    ulong m1 = 1 << (63 - __builtin_clzl(v));
    ulong m2 = 1 << (63 - __builtin_clzl(v&~m1));

    if (m2 << 1 == m1) {
        m2 = 1;
        m1 <<= 1;
    } else {
        m2 <<= 1;
    }

    return m1 | m2;
}

}
}
