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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_parent_of_member.hpp>

namespace ams::util {

    /* Forward declare implementation class for Node. */
    namespace impl {

        class IntrusiveListImpl;

    }

    class IntrusiveListNode {
        NON_COPYABLE(IntrusiveListNode);
        private:
            friend class impl::IntrusiveListImpl;

            IntrusiveListNode *prev;
            IntrusiveListNode *next;
        public:
            constexpr IntrusiveListNode() : prev(this), next(this) { /* ... */ }

            constexpr bool IsLinked() const {
                return this->next != this;
            }
        private:
            void LinkPrev(IntrusiveListNode *node) {
                /* We can't link an already linked node. */
                AMS_ASSERT(!node->IsLinked());
                this->SplicePrev(node, node);
            }

            void SplicePrev(IntrusiveListNode *first, IntrusiveListNode *last) {
                /* Splice a range into the list. */
                auto last_prev = last->prev;
                first->prev = this->prev;
                this->prev->next = first;
                last_prev->next = this;
                this->prev = last_prev;
            }

            void LinkNext(IntrusiveListNode *node) {
                /* We can't link an already linked node. */
                AMS_ASSERT(!node->IsLinked());
                return this->SpliceNext(node, node);
            }

            void SpliceNext(IntrusiveListNode *first, IntrusiveListNode *last) {
                /* Splice a range into the list. */
                auto last_prev = last->prev;
                first->prev = this;
                last_prev->next = next;
                this->next->prev = last_prev;
                this->next = first;
            }

            void Unlink() {
                this->Unlink(this->next);
            }

            void Unlink(IntrusiveListNode *last) {
                /* Unlink a node from a next node. */
                auto last_prev = last->prev;
                this->prev->next = last;
                last->prev = this->prev;
                last_prev->next = this;
                this->prev = last_prev;
            }

            IntrusiveListNode *GetPrev() {
                return this->prev;
            }

            const IntrusiveListNode *GetPrev() const {
                return this->prev;
            }

            IntrusiveListNode *GetNext() {
                return this->next;
            }

            const IntrusiveListNode *GetNext() const {
                return this->next;
            }
    };
    static_assert(std::is_literal_type<IntrusiveListNode>::value);

    namespace impl {

        class IntrusiveListImpl {
            NON_COPYABLE(IntrusiveListImpl);
            private:
                IntrusiveListNode root_node;
            public:
                template<bool Const>
                class Iterator;

                using value_type             = IntrusiveListNode;
                using size_type              = size_t;
                using difference_type        = ptrdiff_t;
                using pointer                = value_type *;
                using const_pointer          = const value_type *;
                using reference              = value_type &;
                using const_reference        = const value_type &;
                using iterator               = Iterator<false>;
                using const_iterator         = Iterator<true>;
                using reverse_iterator       = std::reverse_iterator<iterator>;
                using const_reverse_iterator = std::reverse_iterator<const_iterator>;

                template<bool Const>
                class Iterator {
                    public:
                        using iterator_category = std::bidirectional_iterator_tag;
                        using value_type        = typename IntrusiveListImpl::value_type;
                        using difference_type   = typename IntrusiveListImpl::difference_type;
                        using pointer           = typename std::conditional<Const, IntrusiveListImpl::const_pointer, IntrusiveListImpl::pointer>::type;
                        using reference         = typename std::conditional<Const, IntrusiveListImpl::const_reference, IntrusiveListImpl::reference>::type;
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
                            this->node = this->node->next;
                            return *this;
                        }

                        Iterator &operator--() {
                            this->node = this->node->prev;
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

                        Iterator<false> GetNonConstIterator() const {
                            return Iterator<false>(const_cast<IntrusiveListImpl::pointer>(this->node));
                        }
                };
            public:
                constexpr IntrusiveListImpl() : root_node() { /* ... */ }

                /* Iterator accessors. */
                iterator begin() {
                    return iterator(this->root_node.GetNext());
                }

                const_iterator begin() const {
                    return const_iterator(this->root_node.GetNext());
                }

                iterator end() {
                    return iterator(&this->root_node);
                }

                const_iterator end() const {
                    return const_iterator(&this->root_node);
                }

                iterator iterator_to(reference v) {
                    /* Only allow iterator_to for values in lists. */
                    AMS_ASSERT(v.IsLinked());
                    return iterator(&v);
                }

                const_iterator iterator_to(const_reference v) const {
                    /* Only allow iterator_to for values in lists. */
                    AMS_ASSERT(v.IsLinked());
                    return const_iterator(&v);
                }

                /* Content management. */
                bool empty() const {
                    return !this->root_node.IsLinked();
                }

                size_type size() const {
                    return static_cast<size_type>(std::distance(this->begin(), this->end()));
                }

                reference back() {
                    return *this->root_node.GetPrev();
                }

                const_reference back() const {
                    return *this->root_node.GetPrev();
                }

                reference front() {
                    return *this->root_node.GetNext();
                }

                const_reference front() const {
                    return *this->root_node.GetNext();
                }

                void push_back(reference node) {
                    this->root_node.LinkPrev(&node);
                }

                void push_front(reference node) {
                    this->root_node.LinkNext(&node);
                }

                void pop_back() {
                    this->root_node.GetPrev()->Unlink();
                }

                void pop_front() {
                    this->root_node.GetNext()->Unlink();
                }

                iterator insert(const_iterator pos, reference node) {
                    pos.GetNonConstIterator()->LinkPrev(&node);
                    return iterator(&node);
                }

                void splice(const_iterator pos, IntrusiveListImpl &o) {
                    splice_impl(pos, o.begin(), o.end());
                }

                void splice(const_iterator pos, IntrusiveListImpl &o, const_iterator first) {
                    const_iterator last(first);
                    std::advance(last, 1);
                    splice_impl(pos, first, last);
                }

                void splice(const_iterator pos, IntrusiveListImpl &o, const_iterator first, const_iterator last) {
                    splice_impl(pos, first, last);
                }

                iterator erase(const_iterator pos) {
                    if (pos == this->end()) {
                        return this->end();
                    }
                    iterator it(pos.GetNonConstIterator());
                    (it++)->Unlink();
                    return it;
                }

                void clear() {
                    while (!this->empty()) {
                        this->pop_front();
                    }
                }
            private:
                void splice_impl(const_iterator _pos, const_iterator _first, const_iterator _last) {
                    if (_first == _last) {
                        return;
                    }
                    iterator pos(_pos.GetNonConstIterator());
                    iterator first(_first.GetNonConstIterator());
                    iterator last(_last.GetNonConstIterator());
                    first->Unlink(&*last);
                    pos->SplicePrev(&*first, &*first);
                }

        };

    }

    template<class T, class Traits>
    class IntrusiveList {
        NON_COPYABLE(IntrusiveList);
        private:
            impl::IntrusiveListImpl impl;
        public:
            template<bool Const>
            class Iterator;

            using value_type             = T;
            using size_type              = size_t;
            using difference_type        = ptrdiff_t;
            using pointer                = value_type *;
            using const_pointer          = const value_type *;
            using reference              = value_type &;
            using const_reference        = const value_type &;
            using iterator               = Iterator<false>;
            using const_iterator         = Iterator<true>;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            template<bool Const>
            class Iterator {
                public:
                    friend class ams::util::IntrusiveList<T, Traits>;

                    using ImplIterator = typename std::conditional<Const, ams::util::impl::IntrusiveListImpl::const_iterator, ams::util::impl::IntrusiveListImpl::iterator>::type;

                    using iterator_category = std::bidirectional_iterator_tag;
                    using value_type        = typename IntrusiveList::value_type;
                    using difference_type   = typename IntrusiveList::difference_type;
                    using pointer           = typename std::conditional<Const, IntrusiveList::const_pointer, IntrusiveList::pointer>::type;
                    using reference         = typename std::conditional<Const, IntrusiveList::const_reference, IntrusiveList::reference>::type;
                private:
                    ImplIterator iterator;
                private:
                    explicit Iterator(ImplIterator it) : iterator(it) { /* ... */ }

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
                        return &Traits::GetParent(*this->iterator);
                    }

                    reference operator*() const {
                        return Traits::GetParent(*this->iterator);
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
            static constexpr IntrusiveListNode &GetNode(reference ref) {
                return Traits::GetNode(ref);
            }

            static constexpr IntrusiveListNode const &GetNode(const_reference ref) {
                return Traits::GetNode(ref);
            }

            static constexpr reference GetParent(IntrusiveListNode &node) {
                return Traits::GetParent(node);
            }

            static constexpr const_reference GetParent(IntrusiveListNode const &node) {
                return Traits::GetParent(node);
            }
        public:
            constexpr IntrusiveList() : impl() { /* ... */ }

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

            reverse_iterator rbegin() {
                return reverse_iterator(this->end());
            }

            const_reverse_iterator rbegin() const {
                return const_reverse_iterator(this->end());
            }

            reverse_iterator rend() {
                return reverse_iterator(this->begin());
            }

            const_reverse_iterator rend() const {
                return const_reverse_iterator(this->begin());
            }

            const_reverse_iterator crbegin() const {
                return this->rbegin();
            }

            const_reverse_iterator crend() const {
                return this->rend();
            }

            iterator iterator_to(reference v) {
                return iterator(this->impl.iterator_to(GetNode(v)));
            }

            const_iterator iterator_to(const_reference v) const {
                return const_iterator(this->impl.iterator_to(GetNode(v)));
            }

            /* Content management. */
            bool empty() const {
                return this->impl.empty();
            }

            size_type size() const {
                return this->impl.size();
            }

            reference back() {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.back());
            }

            const_reference back() const {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.back());
            }

            reference front() {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.front());
            }

            const_reference front() const {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.front());
            }

            void push_back(reference ref) {
                this->impl.push_back(GetNode(ref));
            }

            void push_front(reference ref) {
                this->impl.push_front(GetNode(ref));
            }

            void pop_back() {
                AMS_ASSERT(!this->impl.empty());
                this->impl.pop_back();
            }

            void pop_front() {
                AMS_ASSERT(!this->impl.empty());
                this->impl.pop_front();
            }

            iterator insert(const_iterator pos, reference ref) {
                return iterator(this->impl.insert(pos.GetImplIterator(), GetNode(ref)));
            }

            void splice(const_iterator pos, IntrusiveList &o) {
                this->impl.splice(pos.GetImplIterator(), o.impl);
            }

            void splice(const_iterator pos, IntrusiveList &o, const_iterator first) {
                this->impl.splice(pos.GetImplIterator(), o.impl, first.GetImplIterator());
            }

            void splice(const_iterator pos, IntrusiveList &o, const_iterator first, const_iterator last) {
                this->impl.splice(pos.GetImplIterator(), o.impl, first.GetImplIterator(), last.GetImplIterator());
            }

            iterator erase(const_iterator pos) {
                return iterator(this->impl.erase(pos.GetImplIterator()));
            }

            void clear() {
                this->impl.clear();
            }
    };

    template<auto T, class Derived = util::impl::GetParentType<T>>
    class IntrusiveListMemberTraits;

    template<class Parent, IntrusiveListNode Parent::*Member, class Derived>
    class IntrusiveListMemberTraits<Member, Derived> {
        public:
            using ListType = IntrusiveList<Derived, IntrusiveListMemberTraits>;
        private:
            friend class IntrusiveList<Derived, IntrusiveListMemberTraits>;

            static constexpr IntrusiveListNode &GetNode(Derived &parent) {
                return parent.*Member;
            }

            static constexpr IntrusiveListNode const &GetNode(Derived const &parent) {
                return parent.*Member;
            }

            static constexpr Derived &GetParent(IntrusiveListNode &node) {
                return util::GetParentReference<Member, Derived>(&node);
            }

            static constexpr Derived const &GetParent(IntrusiveListNode const &node) {
                return util::GetParentReference<Member, Derived>(&node);
            }
        private:
            static constexpr TYPED_STORAGE(Derived) DerivedStorage = {};
            static_assert(std::addressof(GetParent(GetNode(GetReference(DerivedStorage)))) == GetPointer(DerivedStorage));
    };

    template<auto T, class Derived = util::impl::GetParentType<T>>
    class IntrusiveListMemberTraitsDeferredAssert;

    template<class Parent, IntrusiveListNode Parent::*Member, class Derived>
    class IntrusiveListMemberTraitsDeferredAssert<Member, Derived> {
        public:
            using ListType = IntrusiveList<Derived, IntrusiveListMemberTraitsDeferredAssert>;

            static constexpr bool IsValid() {
                TYPED_STORAGE(Derived) DerivedStorage = {};
                return std::addressof(GetParent(GetNode(GetReference(DerivedStorage)))) == GetPointer(DerivedStorage);
            }
        private:
            friend class IntrusiveList<Derived, IntrusiveListMemberTraitsDeferredAssert>;

            static constexpr IntrusiveListNode &GetNode(Derived &parent) {
                return parent.*Member;
            }

            static constexpr IntrusiveListNode const &GetNode(Derived const &parent) {
                return parent.*Member;
            }

            static constexpr Derived &GetParent(IntrusiveListNode &node) {
                return util::GetParentReference<Member, Derived>(&node);
            }

            static constexpr Derived const &GetParent(IntrusiveListNode const &node) {
                return util::GetParentReference<Member, Derived>(&node);
            }
    };

    template<class Derived>
    class IntrusiveListBaseNode : public IntrusiveListNode{};

    template<class Derived>
    class IntrusiveListBaseTraits {
        public:
            using ListType = IntrusiveList<Derived, IntrusiveListBaseTraits>;
        private:
            friend class IntrusiveList<Derived, IntrusiveListBaseTraits>;

            static constexpr IntrusiveListNode &GetNode(Derived &parent) {
                return static_cast<IntrusiveListNode &>(parent);
            }

            static constexpr IntrusiveListNode const &GetNode(Derived const &parent) {
                return static_cast<const IntrusiveListNode &>(parent);
            }

            static constexpr Derived &GetParent(IntrusiveListNode &node) {
                return static_cast<Derived &>(node);
            }

            static constexpr Derived const &GetParent(IntrusiveListNode const &node) {
                return static_cast<const Derived &>(node);
            }
    };

}