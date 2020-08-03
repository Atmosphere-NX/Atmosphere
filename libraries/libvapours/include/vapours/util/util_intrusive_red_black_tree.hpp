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

    struct IntrusiveRedBlackTreeNode {
        NON_COPYABLE(IntrusiveRedBlackTreeNode);
        private:
            RB_ENTRY(IntrusiveRedBlackTreeNode) entry;

            template<class, class, class>
            friend class IntrusiveRedBlackTree;

            template<class, class>
            friend class IntrusiveRedBlackTreeImpl;
        public:
            constexpr IntrusiveRedBlackTreeNode() : entry() { /* ... */}
    };
    static_assert(std::is_literal_type<IntrusiveRedBlackTreeNode>::value);

    template<class T, class Traits>
    class IntrusiveRedBlackTreeImpl {
        NON_COPYABLE(IntrusiveRedBlackTreeImpl);
        protected:
            RB_HEAD(IntrusiveRedBlackTreeRoot, IntrusiveRedBlackTreeNode);
            using RootType = IntrusiveRedBlackTreeRoot;
        protected:
            IntrusiveRedBlackTreeRoot root;
        public:
            template<bool Const>
            class Iterator;

            using value_type             = T;
            using size_type              = size_t;
            using difference_type        = ptrdiff_t;
            using pointer                = T *;
            using const_pointer          = const T *;
            using reference              = T &;
            using const_reference        = const T &;
            using iterator               = Iterator<false>;
            using const_iterator         = Iterator<true>;

            template<bool Const>
            class Iterator {
                public:
                    using iterator_category = std::bidirectional_iterator_tag;
                    using value_type        = typename IntrusiveRedBlackTreeImpl::value_type;
                    using difference_type   = typename IntrusiveRedBlackTreeImpl::difference_type;
                    using pointer           = typename std::conditional<Const, IntrusiveRedBlackTreeImpl::const_pointer, IntrusiveRedBlackTreeImpl::pointer>::type;
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
                        this->node = Traits::GetParent(GetNext(Traits::GetNode(this->node)));
                        return *this;
                    }

                    Iterator &operator--() {
                        this->node = Traits::GetParent(GetPrev(Traits::GetNode(this->node)));
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
            static constexpr inline IntrusiveRedBlackTreeNode *GetNext(IntrusiveRedBlackTreeNode *node) {
                return RB_NEXT(IntrusiveRedBlackTreeRoot, nullptr, node);
            }

            static constexpr inline IntrusiveRedBlackTreeNode const *GetNext(IntrusiveRedBlackTreeNode const *node) {
                return const_cast<const IntrusiveRedBlackTreeNode *>(GetNext(const_cast<IntrusiveRedBlackTreeNode *>(node)));
            }

            static constexpr inline IntrusiveRedBlackTreeNode *GetPrev(IntrusiveRedBlackTreeNode *node) {
                return RB_PREV(IntrusiveRedBlackTreeRoot, nullptr, node);
            }

            static constexpr inline IntrusiveRedBlackTreeNode const *GetPrev(IntrusiveRedBlackTreeNode const *node) {
                return const_cast<const IntrusiveRedBlackTreeNode *>(GetPrev(const_cast<IntrusiveRedBlackTreeNode *>(node)));
            }
        public:
            constexpr ALWAYS_INLINE IntrusiveRedBlackTreeImpl() : root() {
                this->InitializeImpl();
            }

            /* Iterator accessors. */
            iterator begin() {
                return iterator(Traits::GetParent(this->GetMinImpl()));
            }

            const_iterator begin() const {
                return const_iterator(Traits::GetParent(this->GetMinImpl()));
            }

            iterator end() {
                return iterator(Traits::GetParent(static_cast<IntrusiveRedBlackTreeNode *>(nullptr)));
            }

            const_iterator end() const {
                return const_iterator(Traits::GetParent(static_cast<IntrusiveRedBlackTreeNode *>(nullptr)));
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
                return *Traits::GetParent(this->GetMaxImpl());
            }

            const_reference back() const {
                return *Traits::GetParent(this->GetMaxImpl());
            }

            reference front() {
                return *Traits::GetParent(this->GetMinImpl());
            }

            const_reference front() const {
                return *Traits::GetParent(this->GetMinImpl());
            }

            iterator erase(iterator it) {
                auto cur = Traits::GetNode(&*it);
                auto next = Traits::GetParent(GetNext(cur));
                this->RemoveImpl(cur);
                return iterator(next);
            }
    };

    template<class T, class Traits, class Comparator>
    class IntrusiveRedBlackTree : public IntrusiveRedBlackTreeImpl<T, Traits> {
        NON_COPYABLE(IntrusiveRedBlackTree);
        public:
            using ImplType = IntrusiveRedBlackTreeImpl<T, Traits>;

            struct IntrusiveRedBlackTreeRootWithCompare : ImplType::IntrusiveRedBlackTreeRoot{};

            using value_type             = ImplType::value_type;
            using size_type              = ImplType::size_type;
            using difference_type        = ImplType::difference_type;
            using pointer                = ImplType::pointer;
            using const_pointer          = ImplType::const_pointer;
            using reference              = ImplType::reference;
            using const_reference        = ImplType::const_reference;
            using iterator               = ImplType::iterator;
            using const_iterator         = ImplType::const_iterator;
        protected:
            /* Generate static implementations for comparison operations for IntrusiveRedBlackTreeRoot. */
            RB_GENERATE_WITH_COMPARE_STATIC(IntrusiveRedBlackTreeRootWithCompare, IntrusiveRedBlackTreeNode, entry, CompareImpl);
        private:
            static int CompareImpl(const IntrusiveRedBlackTreeNode *lhs, const IntrusiveRedBlackTreeNode *rhs) {
                return Comparator::Compare(*Traits::GetParent(lhs), *Traits::GetParent(rhs));
            }

            /* Define accessors using RB_* functions. */
            IntrusiveRedBlackTreeNode *InsertImpl(IntrusiveRedBlackTreeNode *node) {
                return RB_INSERT(IntrusiveRedBlackTreeRootWithCompare, static_cast<IntrusiveRedBlackTreeRootWithCompare *>(&this->root), node);
            }

            IntrusiveRedBlackTreeNode *FindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_FIND(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->root)), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }

            IntrusiveRedBlackTreeNode *NFindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_NFIND(IntrusiveRedBlackTreeRootWithCompare, const_cast<IntrusiveRedBlackTreeRootWithCompare *>(static_cast<const IntrusiveRedBlackTreeRootWithCompare *>(&this->root)), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }
        public:
            constexpr ALWAYS_INLINE IntrusiveRedBlackTree() : ImplType() { /* ... */ }

            iterator insert(reference ref) {
                this->InsertImpl(Traits::GetNode(&ref));
                return iterator(&ref);
            }

            iterator find(const_reference ref) const {
                return iterator(Traits::GetParent(this->FindImpl(Traits::GetNode(&ref))));
            }

            iterator nfind(const_reference ref) const {
                return iterator(Traits::GetParent(this->NFindImpl(Traits::GetNode(&ref))));
            }
    };

    template<auto T, class Derived = util::impl::GetParentType<T>>
    class IntrusiveRedBlackTreeMemberTraits;

    template<class Parent, IntrusiveRedBlackTreeNode Parent::*Member, class Derived>
    class IntrusiveRedBlackTreeMemberTraits<Member, Derived> {
        public:
            template<class Comparator>
            using TreeType     = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeMemberTraits, Comparator>;
            using TreeTypeImpl = IntrusiveRedBlackTreeImpl<Derived, IntrusiveRedBlackTreeMemberTraits>;
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;
            template<class, class>
            friend class IntrusiveRedBlackTreeImpl;

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
            using TreeTypeImpl = IntrusiveRedBlackTreeImpl<Derived, IntrusiveRedBlackTreeMemberTraitsDeferredAssert>;

            static constexpr bool IsValid() {
                TYPED_STORAGE(Derived) DerivedStorage = {};
                return GetParent(GetNode(GetPointer(DerivedStorage))) == GetPointer(DerivedStorage);
            }
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;
            template<class, class>
            friend class IntrusiveRedBlackTreeImpl;

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
            constexpr ALWAYS_INLINE Derived *GetPrev();
            constexpr ALWAYS_INLINE const Derived *GetPrev() const;

            constexpr ALWAYS_INLINE Derived *GetNext();
            constexpr ALWAYS_INLINE const Derived *GetNext() const;
    };

    template<class Derived>
    class IntrusiveRedBlackTreeBaseTraits {
        public:
            template<class Comparator>
            using TreeType     = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeBaseTraits, Comparator>;
            using TreeTypeImpl = IntrusiveRedBlackTreeImpl<Derived, IntrusiveRedBlackTreeBaseTraits>;
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;
            template<class, class>
            friend class IntrusiveRedBlackTreeImpl;

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

    template<class Derived>
    constexpr Derived *IntrusiveRedBlackTreeBaseNode<Derived>::GetPrev() { using TreeType = typename IntrusiveRedBlackTreeBaseTraits<Derived>::TreeTypeImpl; return static_cast<Derived *>(TreeType::GetPrev(static_cast<Derived *>(this))); }

    template<class Derived>
    constexpr const Derived *IntrusiveRedBlackTreeBaseNode<Derived>::GetPrev() const { using TreeType = typename IntrusiveRedBlackTreeBaseTraits<Derived>::TreeTypeImpl; return static_cast<const Derived *>(TreeType::GetPrev(static_cast<const Derived *>(this))); }

    template<class Derived>
    constexpr Derived *IntrusiveRedBlackTreeBaseNode<Derived>::GetNext() { using TreeType = typename IntrusiveRedBlackTreeBaseTraits<Derived>::TreeTypeImpl; return static_cast<Derived *>(TreeType::GetNext(static_cast<Derived *>(this))); }

    template<class Derived>
    constexpr const Derived *IntrusiveRedBlackTreeBaseNode<Derived>::GetNext() const { using TreeType = typename IntrusiveRedBlackTreeBaseTraits<Derived>::TreeTypeImpl; return static_cast<const Derived *>(TreeType::GetNext(static_cast<const Derived *>(this))); }

}
