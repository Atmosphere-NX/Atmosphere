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
#include <freebsd/sys/tree.h>
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_parent_of_member.hpp>

namespace ams::util {

    #pragma GCC push_options
    #pragma GCC optimize ("-O3")

    namespace impl {

        class IntrusiveRedBlackTreeImpl;

    }

    struct IntrusiveRedBlackTreeNode {
        NON_COPYABLE(IntrusiveRedBlackTreeNode);
        private:
            RB_ENTRY(IntrusiveRedBlackTreeNode) entry;

            friend class impl::IntrusiveRedBlackTreeImpl;

            template<class, class, class>
            friend class IntrusiveRedBlackTree;
        public:
            constexpr ALWAYS_INLINE IntrusiveRedBlackTreeNode() : entry() { /* ... */}
    };
    static_assert(std::is_literal_type<IntrusiveRedBlackTreeNode>::value);

    template<class T, class Traits, class Comparator>
    class IntrusiveRedBlackTree;

    namespace impl {

        class IntrusiveRedBlackTreeImpl {
            NON_COPYABLE(IntrusiveRedBlackTreeImpl);
            private:
                template<class, class, class>
                friend class ::ams::util::IntrusiveRedBlackTree;
            private:
                RB_HEAD(IntrusiveRedBlackTreeRoot, IntrusiveRedBlackTreeNode);
                using RootType = IntrusiveRedBlackTreeRoot;
            private:
                IntrusiveRedBlackTreeRoot root;
            public:
                template<bool Const>
                class Iterator;

                using value_type      = IntrusiveRedBlackTreeNode;
                using size_type       = size_t;
                using difference_type = ptrdiff_t;
                using pointer         = value_type *;
                using const_pointer   = const value_type *;
                using reference       = value_type &;
                using const_reference = const value_type &;
                using iterator        = Iterator<false>;
                using const_iterator  = Iterator<true>;

                template<bool Const>
                class Iterator {
                    public:
                        using iterator_category = std::bidirectional_iterator_tag;
                        using value_type        = typename IntrusiveRedBlackTreeImpl::value_type;
                        using difference_type   = typename IntrusiveRedBlackTreeImpl::difference_type;
                        using pointer           = typename std::conditional<Const, IntrusiveRedBlackTreeImpl::const_pointer,   IntrusiveRedBlackTreeImpl::pointer>::type;
                        using reference         = typename std::conditional<Const, IntrusiveRedBlackTreeImpl::const_reference, IntrusiveRedBlackTreeImpl::reference>::type;
                    private:
                        pointer node;
                    public:
                        explicit ALWAYS_INLINE Iterator(pointer n) : node(n) { /* ... */ }

                        ALWAYS_INLINE bool operator==(const Iterator &rhs) const {
                            return this->node == rhs.node;
                        }

                        ALWAYS_INLINE bool operator!=(const Iterator &rhs) const {
                            return !(*this == rhs);
                        }

                        ALWAYS_INLINE pointer operator->() const {
                            return this->node;
                        }

                        ALWAYS_INLINE reference operator*() const {
                            return *this->node;
                        }

                        ALWAYS_INLINE Iterator &operator++() {
                            this->node = GetNext(this->node);
                            return *this;
                        }

                        ALWAYS_INLINE Iterator &operator--() {
                            this->node = GetPrev(this->node);
                            return *this;
                        }

                        ALWAYS_INLINE Iterator operator++(int) {
                            const Iterator it{*this};
                            ++(*this);
                            return it;
                        }

                        ALWAYS_INLINE Iterator operator--(int) {
                            const Iterator it{*this};
                            --(*this);
                            return it;
                        }

                        ALWAYS_INLINE operator Iterator<true>() const {
                            return Iterator<true>(this->node);
                        }
                };
            protected:
                /* Generate static implementations for non-comparison operations for IntrusiveRedBlackTreeRoot. */
                RB_GENERATE_WITHOUT_COMPARE_STATIC(IntrusiveRedBlackTreeRoot, IntrusiveRedBlackTreeNode, entry);
            private:
                /* Define accessors using RB_* functions. */
                constexpr ALWAYS_INLINE void InitializeImpl() {
                    RB_INIT(&this->root);
                }

                ALWAYS_INLINE bool EmptyImpl() const {
                    return RB_EMPTY(&this->root);
                }

                ALWAYS_INLINE IntrusiveRedBlackTreeNode *GetMinImpl() const {
                    return RB_MIN(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root));
                }

                ALWAYS_INLINE IntrusiveRedBlackTreeNode *GetMaxImpl() const {
                    return RB_MAX(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root));
                }

                ALWAYS_INLINE IntrusiveRedBlackTreeNode *RemoveImpl(IntrusiveRedBlackTreeNode *node) {
                    return RB_REMOVE(IntrusiveRedBlackTreeRoot, &this->root, node);
                }
            public:
                static ALWAYS_INLINE IntrusiveRedBlackTreeNode *GetNext(IntrusiveRedBlackTreeNode *node) {
                    return RB_NEXT(IntrusiveRedBlackTreeRoot, nullptr, node);
                }

                static ALWAYS_INLINE IntrusiveRedBlackTreeNode *GetPrev(IntrusiveRedBlackTreeNode *node) {
                    return RB_PREV(IntrusiveRedBlackTreeRoot, nullptr, node);
                }

                static ALWAYS_INLINE IntrusiveRedBlackTreeNode const *GetNext(IntrusiveRedBlackTreeNode const *node) {
                    return static_cast<const IntrusiveRedBlackTreeNode *>(GetNext(const_cast<IntrusiveRedBlackTreeNode *>(node)));
                }

                static ALWAYS_INLINE IntrusiveRedBlackTreeNode const *GetPrev(IntrusiveRedBlackTreeNode const *node) {
                    return static_cast<const IntrusiveRedBlackTreeNode *>(GetPrev(const_cast<IntrusiveRedBlackTreeNode *>(node)));
                }
            public:
                ALWAYS_INLINE constexpr IntrusiveRedBlackTreeImpl() : root() {
                    this->InitializeImpl();
                }

                /* Iterator accessors. */
                ALWAYS_INLINE iterator begin() {
                    return iterator(this->GetMinImpl());
                }

                ALWAYS_INLINE const_iterator begin() const {
                    return const_iterator(this->GetMinImpl());
                }

                ALWAYS_INLINE iterator end() {
                    return iterator(static_cast<IntrusiveRedBlackTreeNode *>(nullptr));
                }

                ALWAYS_INLINE const_iterator end() const {
                    return const_iterator(static_cast<const IntrusiveRedBlackTreeNode *>(nullptr));
                }

                ALWAYS_INLINE const_iterator cbegin() const {
                    return this->begin();
                }

                ALWAYS_INLINE const_iterator cend() const {
                    return this->end();
                }

                ALWAYS_INLINE iterator iterator_to(reference ref) {
                    return iterator(&ref);
                }

                ALWAYS_INLINE const_iterator iterator_to(const_reference ref) const {
                    return const_iterator(&ref);
                }

                /* Content management. */
                ALWAYS_INLINE bool empty() const {
                    return this->EmptyImpl();
                }

                ALWAYS_INLINE reference back() {
                    return *this->GetMaxImpl();
                }

                ALWAYS_INLINE const_reference back() const {
                    return *this->GetMaxImpl();
                }

                ALWAYS_INLINE reference front() {
                    return *this->GetMinImpl();
                }

                ALWAYS_INLINE const_reference front() const {
                    return *this->GetMinImpl();
                }

                ALWAYS_INLINE iterator erase(iterator it) {
                    auto cur  = std::addressof(*it);
                    auto next = GetNext(cur);
                    this->RemoveImpl(cur);
                    return iterator(next);
                }
        };

    }

    template<typename T>
    concept HasLightCompareType = requires {
        { std::is_same<typename T::LightCompareType, void>::value } -> std::convertible_to<bool>;
    };

    namespace impl {

        template<typename T, typename Default>
        consteval auto *GetLightCompareType() {
            if constexpr (HasLightCompareType<T>) {
                return static_cast<typename T::LightCompareType *>(nullptr);
            } else {
                return static_cast<Default *>(nullptr);
            }
        }

    }

    template<typename T, typename Default>
    using LightCompareType = typename std::remove_pointer<decltype(impl::GetLightCompareType<T, Default>())>::type;

    template<class T, class Traits, class Comparator>
    class IntrusiveRedBlackTree {
        NON_COPYABLE(IntrusiveRedBlackTree);
        public:
            using ImplType = impl::IntrusiveRedBlackTreeImpl;
        private:
            ImplType impl;
        public:
            struct IntrusiveRedBlackTreeRootWithCompare : ImplType::IntrusiveRedBlackTreeRoot{};

            template<bool Const>
            class Iterator;

            using value_type      = T;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;
            using pointer         = T *;
            using const_pointer   = const T *;
            using reference       = T &;
            using const_reference = const T &;
            using iterator        = Iterator<false>;
            using const_iterator  = Iterator<true>;

            using light_value_type      = LightCompareType<Comparator, value_type>;
            using const_light_pointer   = const light_value_type *;
            using const_light_reference = const light_value_type &;

            template<bool Const>
            class Iterator {
                public:
                    friend class IntrusiveRedBlackTree<T, Traits, Comparator>;

                    using ImplIterator = typename std::conditional<Const, ImplType::const_iterator, ImplType::iterator>::type;

                    using iterator_category = std::bidirectional_iterator_tag;
                    using value_type        = typename IntrusiveRedBlackTree::value_type;
                    using difference_type   = typename IntrusiveRedBlackTree::difference_type;
                    using pointer           = typename std::conditional<Const, IntrusiveRedBlackTree::const_pointer,   IntrusiveRedBlackTree::pointer>::type;
                    using reference         = typename std::conditional<Const, IntrusiveRedBlackTree::const_reference, IntrusiveRedBlackTree::reference>::type;
                private:
                    ImplIterator iterator;
                private:
                    explicit ALWAYS_INLINE Iterator(ImplIterator it) : iterator(it) { /* ... */ }

                    explicit ALWAYS_INLINE Iterator(ImplIterator::pointer p) : iterator(p) { /* ... */ }

                    ALWAYS_INLINE ImplIterator GetImplIterator() const {
                        return this->iterator;
                    }
                public:
                    ALWAYS_INLINE bool operator==(const Iterator &rhs) const {
                        return this->iterator == rhs.iterator;
                    }

                    ALWAYS_INLINE bool operator!=(const Iterator &rhs) const {
                        return !(*this == rhs);
                    }

                    ALWAYS_INLINE pointer operator->() const {
                        return Traits::GetParent(std::addressof(*this->iterator));
                    }

                    ALWAYS_INLINE reference operator*() const {
                        return *Traits::GetParent(std::addressof(*this->iterator));
                    }

                    ALWAYS_INLINE Iterator &operator++() {
                        ++this->iterator;
                        return *this;
                    }

                    ALWAYS_INLINE Iterator &operator--() {
                        --this->iterator;
                        return *this;
                    }

                    ALWAYS_INLINE Iterator operator++(int) {
                        const Iterator it{*this};
                        ++this->iterator;
                        return it;
                    }

                    ALWAYS_INLINE Iterator operator--(int) {
                        const Iterator it{*this};
                        --this->iterator;
                        return it;
                    }

                    ALWAYS_INLINE operator Iterator<true>() const {
                        return Iterator<true>(this->iterator);
                    }
            };
        private:
            /* Generate static implementations for comparison operations for IntrusiveRedBlackTreeRoot. */
            RB_GENERATE_WITH_COMPARE_STATIC(IntrusiveRedBlackTreeRootWithCompare, IntrusiveRedBlackTreeNode, entry, CompareImpl, LightCompareImpl);
        private:
            static ALWAYS_INLINE int CompareImpl(const IntrusiveRedBlackTreeNode *lhs, const IntrusiveRedBlackTreeNode *rhs) {
                return Comparator::Compare(*Traits::GetParent(lhs), *Traits::GetParent(rhs));
            }

            static ALWAYS_INLINE int LightCompareImpl(const void *elm, const IntrusiveRedBlackTreeNode *rhs) {
                return Comparator::Compare(*static_cast<const_light_pointer>(elm), *Traits::GetParent(rhs));
            }

            /* Define accessors using RB_* functions. */
            ALWAYS_INLINE IntrusiveRedBlackTreeNode *InsertImpl(IntrusiveRedBlackTreeNode *node) {
                return RB_INSERT(IntrusiveRedBlackTreeRootWithCompare, static_cast<IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root), node);
            }

            ALWAYS_INLINE IntrusiveRedBlackTreeNode *FindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_FIND(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root)), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }

            ALWAYS_INLINE IntrusiveRedBlackTreeNode *NFindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_NFIND(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root)), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }

            ALWAYS_INLINE IntrusiveRedBlackTreeNode *FindLightImpl(const_light_pointer lelm) const {
                return RB_FIND_LIGHT(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root)), static_cast<const void *>(lelm));
            }

            ALWAYS_INLINE IntrusiveRedBlackTreeNode *NFindLightImpl(const_light_pointer lelm) const {
                return RB_NFIND_LIGHT(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root)), static_cast<const void *>(lelm));
            }
        public:
            constexpr ALWAYS_INLINE IntrusiveRedBlackTree() : impl() { /* ... */ }

            /* Iterator accessors. */
            ALWAYS_INLINE iterator begin() {
                return iterator(this->impl.begin());
            }

            ALWAYS_INLINE const_iterator begin() const {
                return const_iterator(this->impl.begin());
            }

            ALWAYS_INLINE iterator end() {
                return iterator(this->impl.end());
            }

            ALWAYS_INLINE const_iterator end() const {
                return const_iterator(this->impl.end());
            }

            ALWAYS_INLINE const_iterator cbegin() const {
                return this->begin();
            }

            ALWAYS_INLINE const_iterator cend() const {
                return this->end();
            }

            ALWAYS_INLINE iterator iterator_to(reference ref) {
                return iterator(this->impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
            }

            ALWAYS_INLINE const_iterator iterator_to(const_reference ref) const {
                return const_iterator(this->impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
            }

            /* Content management. */
            ALWAYS_INLINE bool empty() const {
                return this->impl.empty();
            }

            ALWAYS_INLINE reference back() {
                return *Traits::GetParent(std::addressof(this->impl.back()));
            }

            ALWAYS_INLINE const_reference back() const {
                return *Traits::GetParent(std::addressof(this->impl.back()));
            }

            ALWAYS_INLINE reference front() {
                return *Traits::GetParent(std::addressof(this->impl.front()));
            }

            ALWAYS_INLINE const_reference front() const {
                return *Traits::GetParent(std::addressof(this->impl.front()));
            }

            ALWAYS_INLINE iterator erase(iterator it) {
                return iterator(this->impl.erase(it.GetImplIterator()));
            }

            ALWAYS_INLINE iterator insert(reference ref) {
                ImplType::pointer node = Traits::GetNode(std::addressof(ref));
                this->InsertImpl(node);
                return iterator(node);
            }

            ALWAYS_INLINE iterator find(const_reference ref) const {
                return iterator(this->FindImpl(Traits::GetNode(std::addressof(ref))));
            }

            ALWAYS_INLINE iterator nfind(const_reference ref) const {
                return iterator(this->NFindImpl(Traits::GetNode(std::addressof(ref))));
            }

            ALWAYS_INLINE iterator find_light(const_light_reference ref) const {
                return iterator(this->FindLightImpl(std::addressof(ref)));
            }

            ALWAYS_INLINE iterator nfind_light(const_light_reference ref) const {
                return iterator(this->NFindLightImpl(std::addressof(ref)));
            }
    };

    template<auto T, class Derived = util::impl::GetParentType<T>>
    class IntrusiveRedBlackTreeMemberTraits;

    template<class Parent, IntrusiveRedBlackTreeNode Parent::*Member, class Derived>
    class IntrusiveRedBlackTreeMemberTraits<Member, Derived> {
        public:
            template<class Comparator>
            using TreeType     = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeMemberTraits, Comparator>;
            using TreeTypeImpl = impl::IntrusiveRedBlackTreeImpl;
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;

            friend class impl::IntrusiveRedBlackTreeImpl;

            static constexpr ALWAYS_INLINE IntrusiveRedBlackTreeNode *GetNode(Derived *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr ALWAYS_INLINE IntrusiveRedBlackTreeNode const *GetNode(Derived const *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr ALWAYS_INLINE Derived *GetParent(IntrusiveRedBlackTreeNode *node) {
                return util::GetParentPointer<Member, Derived>(node);
            }

            static constexpr ALWAYS_INLINE Derived const *GetParent(IntrusiveRedBlackTreeNode const *node) {
                return util::GetParentPointer<Member, Derived>(node);
            }
        private:
            static constexpr TYPED_STORAGE(Derived) DerivedStorage = {};
            static_assert(GetParent(GetNode(GetPointer(DerivedStorage))) == GetPointer(DerivedStorage));
    };

    template<auto T, class Derived = util::impl::GetParentType<T>>
    class IntrusiveRedBlackTreeMemberTraitsDeferredAssert;

    template<class Parent, IntrusiveRedBlackTreeNode Parent::*Member, class Derived>
    class IntrusiveRedBlackTreeMemberTraitsDeferredAssert<Member, Derived> {
        public:
            template<class Comparator>
            using TreeType     = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeMemberTraitsDeferredAssert, Comparator>;
            using TreeTypeImpl = impl::IntrusiveRedBlackTreeImpl;

            static constexpr bool IsValid() {
                TYPED_STORAGE(Derived) DerivedStorage = {};
                return GetParent(GetNode(GetPointer(DerivedStorage))) == GetPointer(DerivedStorage);
            }
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;

            friend class impl::IntrusiveRedBlackTreeImpl;

            static constexpr ALWAYS_INLINE IntrusiveRedBlackTreeNode *GetNode(Derived *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr ALWAYS_INLINE IntrusiveRedBlackTreeNode const *GetNode(Derived const *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr ALWAYS_INLINE Derived *GetParent(IntrusiveRedBlackTreeNode *node) {
                return util::GetParentPointer<Member, Derived>(node);
            }

            static constexpr ALWAYS_INLINE Derived const *GetParent(IntrusiveRedBlackTreeNode const *node) {
                return util::GetParentPointer<Member, Derived>(node);
            }
    };

    template<class Derived>
    class IntrusiveRedBlackTreeBaseNode : public IntrusiveRedBlackTreeNode {
        public:
            constexpr ALWAYS_INLINE Derived *GetPrev()             { return static_cast<      Derived *>(impl::IntrusiveRedBlackTreeImpl::GetPrev(this)); }
            constexpr ALWAYS_INLINE const Derived *GetPrev() const { return static_cast<const Derived *>(impl::IntrusiveRedBlackTreeImpl::GetPrev(this)); }

            constexpr ALWAYS_INLINE Derived *GetNext()             { return static_cast<      Derived *>(impl::IntrusiveRedBlackTreeImpl::GetNext(this)); }
            constexpr ALWAYS_INLINE const Derived *GetNext() const { return static_cast<const Derived *>(impl::IntrusiveRedBlackTreeImpl::GetNext(this)); }
    };

    template<class Derived>
    class IntrusiveRedBlackTreeBaseTraits {
        public:
            template<class Comparator>
            using TreeType     = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeBaseTraits, Comparator>;
            using TreeTypeImpl = impl::IntrusiveRedBlackTreeImpl;
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;

            friend class impl::IntrusiveRedBlackTreeImpl;

            static constexpr ALWAYS_INLINE IntrusiveRedBlackTreeNode *GetNode(Derived *parent) {
                return static_cast<IntrusiveRedBlackTreeNode *>(parent);
            }

            static constexpr ALWAYS_INLINE IntrusiveRedBlackTreeNode const *GetNode(Derived const *parent) {
                return static_cast<const IntrusiveRedBlackTreeNode *>(parent);
            }

            static constexpr ALWAYS_INLINE Derived *GetParent(IntrusiveRedBlackTreeNode *node) {
                return static_cast<Derived *>(node);
            }

            static constexpr ALWAYS_INLINE Derived const *GetParent(IntrusiveRedBlackTreeNode const *node) {
                return static_cast<const Derived *>(node);
            }
    };

    #pragma GCC pop_options

}
