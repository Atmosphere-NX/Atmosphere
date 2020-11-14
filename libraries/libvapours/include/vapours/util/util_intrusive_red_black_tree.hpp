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
            constexpr IntrusiveRedBlackTreeNode() : entry() { /* ... */}
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
                        explicit Iterator(pointer n) : node(n) { /* ... */ }

                        bool operator==(const Iterator &rhs) const {
                            return this->node == rhs.node;
                        }

                        bool operator!=(const Iterator &rhs) const {
                            return !(*this == rhs);
                        }

                        pointer operator->() const {
                            return this->node;
                        }

                        reference operator*() const {
                            return *this->node;
                        }

                        Iterator &operator++() {
                            this->node = GetNext(this->node);
                            return *this;
                        }

                        Iterator &operator--() {
                            this->node = GetPrev(this->node);
                            return *this;
                        }

                        Iterator operator++(int) {
                            const Iterator it{*this};
                            ++(*this);
                            return it;
                        }

                        Iterator operator--(int) {
                            const Iterator it{*this};
                            --(*this);
                            return it;
                        }

                        operator Iterator<true>() const {
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

                bool EmptyImpl() const {
                    return RB_EMPTY(&this->root);
                }

                IntrusiveRedBlackTreeNode *GetMinImpl() const {
                    return RB_MIN(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root));
                }

                IntrusiveRedBlackTreeNode *GetMaxImpl() const {
                    return RB_MAX(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root));
                }

                IntrusiveRedBlackTreeNode *RemoveImpl(IntrusiveRedBlackTreeNode *node) {
                    return RB_REMOVE(IntrusiveRedBlackTreeRoot, &this->root, node);
                }
            public:
                static IntrusiveRedBlackTreeNode *GetNext(IntrusiveRedBlackTreeNode *node) {
                    return RB_NEXT(IntrusiveRedBlackTreeRoot, nullptr, node);
                }

                static IntrusiveRedBlackTreeNode *GetPrev(IntrusiveRedBlackTreeNode *node) {
                    return RB_PREV(IntrusiveRedBlackTreeRoot, nullptr, node);
                }

                static ALWAYS_INLINE IntrusiveRedBlackTreeNode const *GetNext(IntrusiveRedBlackTreeNode const *node) {
                    return static_cast<const IntrusiveRedBlackTreeNode *>(GetNext(const_cast<IntrusiveRedBlackTreeNode *>(node)));
                }

                static ALWAYS_INLINE IntrusiveRedBlackTreeNode const *GetPrev(IntrusiveRedBlackTreeNode const *node) {
                    return static_cast<const IntrusiveRedBlackTreeNode *>(GetPrev(const_cast<IntrusiveRedBlackTreeNode *>(node)));
                }
            public:
                constexpr IntrusiveRedBlackTreeImpl() : root() {
                    this->InitializeImpl();
                }

                /* Iterator accessors. */
                iterator begin() {
                    return iterator(this->GetMinImpl());
                }

                const_iterator begin() const {
                    return const_iterator(this->GetMinImpl());
                }

                iterator end() {
                    return iterator(static_cast<IntrusiveRedBlackTreeNode *>(nullptr));
                }

                const_iterator end() const {
                    return const_iterator(static_cast<const IntrusiveRedBlackTreeNode *>(nullptr));
                }

                const_iterator cbegin() const {
                    return this->begin();
                }

                const_iterator cend() const {
                    return this->end();
                }

                iterator iterator_to(reference ref) {
                    return iterator(&ref);
                }

                const_iterator iterator_to(const_reference ref) const {
                    return const_iterator(&ref);
                }

                /* Content management. */
                bool empty() const {
                    return this->EmptyImpl();
                }

                reference back() {
                    return *this->GetMaxImpl();
                }

                const_reference back() const {
                    return *this->GetMaxImpl();
                }

                reference front() {
                    return *this->GetMinImpl();
                }

                const_reference front() const {
                    return *this->GetMinImpl();
                }

                iterator erase(iterator it) {
                    auto cur  = std::addressof(*it);
                    auto next = GetNext(cur);
                    this->RemoveImpl(cur);
                    return iterator(next);
                }
        };

    }

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
                    explicit Iterator(ImplIterator it) : iterator(it) { /* ... */ }

                    explicit Iterator(ImplIterator::pointer p) : iterator(p) { /* ... */ }

                    ImplIterator GetImplIterator() const {
                        return this->iterator;
                    }
                public:
                    bool operator==(const Iterator &rhs) const {
                        return this->iterator == rhs.iterator;
                    }

                    bool operator!=(const Iterator &rhs) const {
                        return !(*this == rhs);
                    }

                    pointer operator->() const {
                        return Traits::GetParent(std::addressof(*this->iterator));
                    }

                    reference operator*() const {
                        return *Traits::GetParent(std::addressof(*this->iterator));
                    }

                    Iterator &operator++() {
                        ++this->iterator;
                        return *this;
                    }

                    Iterator &operator--() {
                        --this->iterator;
                        return *this;
                    }

                    Iterator operator++(int) {
                        const Iterator it{*this};
                        ++this->iterator;
                        return it;
                    }

                    Iterator operator--(int) {
                        const Iterator it{*this};
                        --this->iterator;
                        return it;
                    }

                    operator Iterator<true>() const {
                        return Iterator<true>(this->iterator);
                    }
            };
        private:
            /* Generate static implementations for comparison operations for IntrusiveRedBlackTreeRoot. */
            RB_GENERATE_WITH_COMPARE_STATIC(IntrusiveRedBlackTreeRootWithCompare, IntrusiveRedBlackTreeNode, entry, CompareImpl);
        private:
            static int CompareImpl(const IntrusiveRedBlackTreeNode *lhs, const IntrusiveRedBlackTreeNode *rhs) {
                return Comparator::Compare(*Traits::GetParent(lhs), *Traits::GetParent(rhs));
            }

            /* Define accessors using RB_* functions. */
            IntrusiveRedBlackTreeNode *InsertImpl(IntrusiveRedBlackTreeNode *node) {
                return RB_INSERT(IntrusiveRedBlackTreeRootWithCompare, static_cast<IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root), node);
            }

            IntrusiveRedBlackTreeNode *FindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_FIND(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root)), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }

            IntrusiveRedBlackTreeNode *NFindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_NFIND(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->impl.root)), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }
        public:
            constexpr ALWAYS_INLINE IntrusiveRedBlackTree() : impl() { /* ... */ }

            /* Iterator accessors. */
            iterator begin() {
                return iterator(this->impl.begin());
            }

            const_iterator begin() const {
                return const_iterator(this->impl.begin());
            }

            iterator end() {
                return iterator(this->impl.end());
            }

            const_iterator end() const {
                return const_iterator(this->impl.end());
            }

            const_iterator cbegin() const {
                return this->begin();
            }

            const_iterator cend() const {
                return this->end();
            }

            iterator iterator_to(reference ref) {
                return iterator(this->impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
            }

            const_iterator iterator_to(const_reference ref) const {
                return const_iterator(this->impl.iterator_to(*Traits::GetNode(std::addressof(ref))));
            }

            /* Content management. */
            bool empty() const {
                return this->impl.empty();
            }

            reference back() {
                return *Traits::GetParent(std::addressof(this->impl.back()));
            }

            const_reference back() const {
                return *Traits::GetParent(std::addressof(this->impl.back()));
            }

            reference front() {
                return *Traits::GetParent(std::addressof(this->impl.front()));
            }

            const_reference front() const {
                return *Traits::GetParent(std::addressof(this->impl.front()));
            }

            iterator erase(iterator it) {
                return iterator(this->impl.erase(it.GetImplIterator()));
            }

            iterator insert(reference ref) {
                ImplType::pointer node = Traits::GetNode(std::addressof(ref));
                this->InsertImpl(node);
                return iterator(node);
            }

            iterator find(const_reference ref) const {
                return iterator(this->FindImpl(Traits::GetNode(std::addressof(ref))));
            }

            iterator nfind(const_reference ref) const {
                return iterator(this->NFindImpl(Traits::GetNode(std::addressof(ref))));
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

            static constexpr IntrusiveRedBlackTreeNode *GetNode(Derived *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr IntrusiveRedBlackTreeNode const *GetNode(Derived const *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr Derived *GetParent(IntrusiveRedBlackTreeNode *node) {
                return util::GetParentPointer<Member, Derived>(node);
            }

            static constexpr Derived const *GetParent(IntrusiveRedBlackTreeNode const *node) {
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

            static constexpr IntrusiveRedBlackTreeNode *GetNode(Derived *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr IntrusiveRedBlackTreeNode const *GetNode(Derived const *parent) {
                return std::addressof(parent->*Member);
            }

            static constexpr Derived *GetParent(IntrusiveRedBlackTreeNode *node) {
                return util::GetParentPointer<Member, Derived>(node);
            }

            static constexpr Derived const *GetParent(IntrusiveRedBlackTreeNode const *node) {
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

            static constexpr IntrusiveRedBlackTreeNode *GetNode(Derived *parent) {
                return static_cast<IntrusiveRedBlackTreeNode *>(parent);
            }

            static constexpr IntrusiveRedBlackTreeNode const *GetNode(Derived const *parent) {
                return static_cast<const IntrusiveRedBlackTreeNode *>(parent);
            }

            static constexpr Derived *GetParent(IntrusiveRedBlackTreeNode *node) {
                return static_cast<Derived *>(node);
            }

            static constexpr Derived const *GetParent(IntrusiveRedBlackTreeNode const *node) {
                return static_cast<const Derived *>(node);
            }
    };

}
