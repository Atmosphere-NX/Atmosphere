/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <iterator>
#include <switch.h>
#include "../defines.hpp"

#include "util_parent_of_member.hpp"

namespace sts::util {

    struct IntrusiveRedBlackTreeNode {
        NON_COPYABLE(IntrusiveRedBlackTreeNode);
        private:
            RB_ENTRY(IntrusiveRedBlackTreeNode) entry;

            template<class, class, class>
            friend class IntrusiveRedBlackTree;
        public:
            IntrusiveRedBlackTreeNode() { /* ... */}
    };

    template<class T, class Traits, class Comparator>
    class IntrusiveRedBlackTree {
        NON_COPYABLE(IntrusiveRedBlackTree);
        private:
            RB_HEAD(IntrusiveRedBlackTreeRoot, IntrusiveRedBlackTreeNode);

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
                    using value_type        = typename IntrusiveRedBlackTree::value_type;
                    using difference_type   = typename IntrusiveRedBlackTree::difference_type;
                    using pointer           = typename std::conditional<Const, IntrusiveRedBlackTree::const_pointer, IntrusiveRedBlackTree::pointer>::type;
                    using reference         = typename std::conditional<Const, IntrusiveRedBlackTree::const_reference, IntrusiveRedBlackTree::reference>::type;
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
        private:
            static int CompareImpl(const IntrusiveRedBlackTreeNode *lhs, const IntrusiveRedBlackTreeNode *rhs) {
                return Comparator::Compare(*Traits::GetParent(lhs), *Traits::GetParent(rhs));
            }

            /* Generate static implementations for IntrusiveRedBlackTreeRoot. */
            RB_GENERATE_STATIC(IntrusiveRedBlackTreeRoot, IntrusiveRedBlackTreeNode, entry, CompareImpl);

            static constexpr inline IntrusiveRedBlackTreeNode *GetNext(IntrusiveRedBlackTreeNode *node) {
                return RB_NEXT(IntrusiveRedBlackTreeRoot, nullptr, node);
            }

            static constexpr inline IntrusiveRedBlackTreeNode const *GetNext(IntrusiveRedBlackTreeNode const *node) {
                return const_cast<const IntrusiveRedBlackTreeNode *>(GetNext(const_cast<IntrusiveRedBlackTreeNode *>(node)));
            }

            static constexpr inline IntrusiveRedBlackTreeNode *GetPrev(IntrusiveRedBlackTreeNode *node) {
                return RB_NEXT(IntrusiveRedBlackTreeRoot, nullptr, node);
            }

            static constexpr inline IntrusiveRedBlackTreeNode const *GetPrev(IntrusiveRedBlackTreeNode const *node) {
                return const_cast<const IntrusiveRedBlackTreeNode *>(GetPrev(const_cast<IntrusiveRedBlackTreeNode *>(node)));
            }

            /* Define accessors using RB_* functions. */
            void InitializeImpl() {
                RB_INIT(&this->root);
            }

            bool EmptyImpl() const {
                return RB_EMPTY(&this->root);
            }

            IntrusiveRedBlackTreeNode *GetMinImpl() const {
                return RB_MIN(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root));
            }

            IntrusiveRedBlackTreeNode *GetMaxImpl() const {
                return RB_MIN(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root));
            }

            IntrusiveRedBlackTreeNode *InsertImpl(IntrusiveRedBlackTreeNode *node) {
                return RB_INSERT(IntrusiveRedBlackTreeRoot, &this->root, node);
            }

            IntrusiveRedBlackTreeNode *RemoveImpl(IntrusiveRedBlackTreeNode *node) {
                return RB_REMOVE(IntrusiveRedBlackTreeRoot, &this->root, node);
            }

            IntrusiveRedBlackTreeNode *FindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_FIND(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }

            IntrusiveRedBlackTreeNode *NFindImpl(IntrusiveRedBlackTreeNode const *node) const {
                return RB_NFIND(IntrusiveRedBlackTreeRoot, const_cast<IntrusiveRedBlackTreeRoot *>(&this->root), const_cast<IntrusiveRedBlackTreeNode *>(node));
            }

        public:
            IntrusiveRedBlackTree() {
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
                return Traits::GetParent(this->GetMaxImpl());
            }

            const_reference back() const {
                return Traits::GetParent(this->GetMaxImpl());
            }

            reference front() {
                return Traits::GetParent(this->GetMinImpl());
            }

            const_reference front() const {
                return Traits::GetParent(this->GetMinImpl());
            }

            iterator insert(reference ref) {
                this->InsertImpl(Traits::GetNode(&ref));
                return iterator(&ref);
            }

            iterator erase(iterator it) {
                auto cur = Traits::GetNode(&*it);
                auto next = Traits::GetParent(GetNext(cur));
                this->RemoveImpl(cur);
                return iterator(next);
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
            using ListType = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeMemberTraits, Comparator>;
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;

            static constexpr IntrusiveRedBlackTreeNode *GetNode(Derived *parent) {
                return &(parent->*Member);
            }

            static constexpr IntrusiveRedBlackTreeNode const *GetNode(Derived const *parent) {
                return &(parent->*Member);
            }

            static constexpr Derived *GetParent(IntrusiveRedBlackTreeNode *node) {
                return static_cast<Derived *>(util::GetParentPointer<Member>(node));
            }

            static constexpr Derived const *GetParent(IntrusiveRedBlackTreeNode const *node) {
                return static_cast<const Derived *>(util::GetParentPointer<Member>(node));
            }
    };

    template<class Derived>
    class IntrusiveRedBlackTreeBaseNode : public IntrusiveRedBlackTreeNode{};

    template<class Derived>
    class IntrusiveRedBlackTreeBaseTraits {
        public:
            template<class Comparator>
            using ListType = IntrusiveRedBlackTree<Derived, IntrusiveRedBlackTreeBaseTraits, Comparator>;
        private:
            template<class, class, class>
            friend class IntrusiveRedBlackTree;

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