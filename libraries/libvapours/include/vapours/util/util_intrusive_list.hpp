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

    #pragma GCC push_options
    #pragma GCC optimize ("-O3")

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

            constexpr ALWAYS_INLINE bool IsLinked() const {
                return this->next != this;
            }
        private:
            ALWAYS_INLINE void LinkPrev(IntrusiveListNode *node) {
                /* We can't link an already linked node. */
                AMS_ASSERT(!node->IsLinked());
                this->SplicePrev(node, node);
            }

            ALWAYS_INLINE void SplicePrev(IntrusiveListNode *first, IntrusiveListNode *last) {
                /* Splice a range into the list. */
                auto last_prev = last->prev;
                first->prev = this->prev;
                this->prev->next = first;
                last_prev->next = this;
                this->prev = last_prev;
            }

            ALWAYS_INLINE void LinkNext(IntrusiveListNode *node) {
                /* We can't link an already linked node. */
                AMS_ASSERT(!node->IsLinked());
                return this->SpliceNext(node, node);
            }

            ALWAYS_INLINE void SpliceNext(IntrusiveListNode *first, IntrusiveListNode *last) {
                /* Splice a range into the list. */
                auto last_prev = last->prev;
                first->prev = this;
                last_prev->next = next;
                this->next->prev = last_prev;
                this->next = first;
            }

            ALWAYS_INLINE void Unlink() {
                this->Unlink(this->next);
            }

            ALWAYS_INLINE void Unlink(IntrusiveListNode *last) {
                /* Unlink a node from a next node. */
                auto last_prev = last->prev;
                this->prev->next = last;
                last->prev = this->prev;
                last_prev->next = this;
                this->prev = last_prev;
            }

            ALWAYS_INLINE IntrusiveListNode *GetPrev() {
                return this->prev;
            }

            ALWAYS_INLINE const IntrusiveListNode *GetPrev() const {
                return this->prev;
            }

            ALWAYS_INLINE IntrusiveListNode *GetNext() {
                return this->next;
            }

            ALWAYS_INLINE const IntrusiveListNode *GetNext() const {
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
                        ALWAYS_INLINE explicit Iterator(pointer n) : node(n) { /* ... */ }

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
                            this->node = this->node->next;
                            return *this;
                        }

                        ALWAYS_INLINE Iterator &operator--() {
                            this->node = this->node->prev;
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

                        ALWAYS_INLINE Iterator<false> GetNonConstIterator() const {
                            return Iterator<false>(const_cast<IntrusiveListImpl::pointer>(this->node));
                        }
                };
            public:
                constexpr ALWAYS_INLINE IntrusiveListImpl() : root_node() { /* ... */ }

                /* Iterator accessors. */
                ALWAYS_INLINE iterator begin() {
                    return iterator(this->root_node.GetNext());
                }

                ALWAYS_INLINE const_iterator begin() const {
                    return const_iterator(this->root_node.GetNext());
                }

                ALWAYS_INLINE iterator end() {
                    return iterator(&this->root_node);
                }

                ALWAYS_INLINE const_iterator end() const {
                    return const_iterator(&this->root_node);
                }

                ALWAYS_INLINE iterator iterator_to(reference v) {
                    /* Only allow iterator_to for values in lists. */
                    AMS_ASSERT(v.IsLinked());
                    return iterator(&v);
                }

                ALWAYS_INLINE const_iterator iterator_to(const_reference v) const {
                    /* Only allow iterator_to for values in lists. */
                    AMS_ASSERT(v.IsLinked());
                    return const_iterator(&v);
                }

                /* Content management. */
                ALWAYS_INLINE bool empty() const {
                    return !this->root_node.IsLinked();
                }

                ALWAYS_INLINE size_type size() const {
                    return static_cast<size_type>(std::distance(this->begin(), this->end()));
                }

                ALWAYS_INLINE reference back() {
                    return *this->root_node.GetPrev();
                }

                ALWAYS_INLINE const_reference back() const {
                    return *this->root_node.GetPrev();
                }

                ALWAYS_INLINE reference front() {
                    return *this->root_node.GetNext();
                }

                ALWAYS_INLINE const_reference front() const {
                    return *this->root_node.GetNext();
                }

                ALWAYS_INLINE void push_back(reference node) {
                    this->root_node.LinkPrev(&node);
                }

                ALWAYS_INLINE void push_front(reference node) {
                    this->root_node.LinkNext(&node);
                }

                ALWAYS_INLINE void pop_back() {
                    this->root_node.GetPrev()->Unlink();
                }

                ALWAYS_INLINE void pop_front() {
                    this->root_node.GetNext()->Unlink();
                }

                ALWAYS_INLINE iterator insert(const_iterator pos, reference node) {
                    pos.GetNonConstIterator()->LinkPrev(&node);
                    return iterator(&node);
                }

                ALWAYS_INLINE void splice(const_iterator pos, IntrusiveListImpl &o) {
                    splice_impl(pos, o.begin(), o.end());
                }

                ALWAYS_INLINE void splice(const_iterator pos, IntrusiveListImpl &o, const_iterator first) {
                    AMS_UNUSED(o);
                    const_iterator last(first);
                    std::advance(last, 1);
                    splice_impl(pos, first, last);
                }

                ALWAYS_INLINE void splice(const_iterator pos, IntrusiveListImpl &o, const_iterator first, const_iterator last) {
                    AMS_UNUSED(o);
                    splice_impl(pos, first, last);
                }

                ALWAYS_INLINE iterator erase(const_iterator pos) {
                    if (pos == this->end()) {
                        return this->end();
                    }
                    iterator it(pos.GetNonConstIterator());
                    (it++)->Unlink();
                    return it;
                }

                ALWAYS_INLINE void clear() {
                    while (!this->empty()) {
                        this->pop_front();
                    }
                }
            private:
                ALWAYS_INLINE void splice_impl(const_iterator _pos, const_iterator _first, const_iterator _last) {
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
                    explicit ALWAYS_INLINE Iterator(ImplIterator it) : iterator(it) { /* ... */ }

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
                        return &Traits::GetParent(*this->iterator);
                    }

                    ALWAYS_INLINE reference operator*() const {
                        return Traits::GetParent(*this->iterator);
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
            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(reference ref) {
                return Traits::GetNode(ref);
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(const_reference ref) {
                return Traits::GetNode(ref);
            }

            static constexpr ALWAYS_INLINE reference GetParent(IntrusiveListNode &node) {
                return Traits::GetParent(node);
            }

            static constexpr ALWAYS_INLINE const_reference GetParent(IntrusiveListNode const &node) {
                return Traits::GetParent(node);
            }
        public:
            constexpr ALWAYS_INLINE IntrusiveList() : impl() { /* ... */ }

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

            ALWAYS_INLINE reverse_iterator rbegin() {
                return reverse_iterator(this->end());
            }

            ALWAYS_INLINE const_reverse_iterator rbegin() const {
                return const_reverse_iterator(this->end());
            }

            ALWAYS_INLINE reverse_iterator rend() {
                return reverse_iterator(this->begin());
            }

            ALWAYS_INLINE const_reverse_iterator rend() const {
                return const_reverse_iterator(this->begin());
            }

            ALWAYS_INLINE const_reverse_iterator crbegin() const {
                return this->rbegin();
            }

            ALWAYS_INLINE const_reverse_iterator crend() const {
                return this->rend();
            }

            ALWAYS_INLINE iterator iterator_to(reference v) {
                return iterator(this->impl.iterator_to(GetNode(v)));
            }

            ALWAYS_INLINE const_iterator iterator_to(const_reference v) const {
                return const_iterator(this->impl.iterator_to(GetNode(v)));
            }

            /* Content management. */
            ALWAYS_INLINE bool empty() const {
                return this->impl.empty();
            }

            ALWAYS_INLINE size_type size() const {
                return this->impl.size();
            }

            ALWAYS_INLINE reference back() {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.back());
            }

            ALWAYS_INLINE const_reference back() const {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.back());
            }

            ALWAYS_INLINE reference front() {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.front());
            }

            ALWAYS_INLINE const_reference front() const {
                AMS_ASSERT(!this->impl.empty());
                return GetParent(this->impl.front());
            }

            ALWAYS_INLINE void push_back(reference ref) {
                this->impl.push_back(GetNode(ref));
            }

            ALWAYS_INLINE void push_front(reference ref) {
                this->impl.push_front(GetNode(ref));
            }

            ALWAYS_INLINE void pop_back() {
                AMS_ASSERT(!this->impl.empty());
                this->impl.pop_back();
            }

            ALWAYS_INLINE void pop_front() {
                AMS_ASSERT(!this->impl.empty());
                this->impl.pop_front();
            }

            ALWAYS_INLINE iterator insert(const_iterator pos, reference ref) {
                return iterator(this->impl.insert(pos.GetImplIterator(), GetNode(ref)));
            }

            ALWAYS_INLINE void splice(const_iterator pos, IntrusiveList &o) {
                this->impl.splice(pos.GetImplIterator(), o.impl);
            }

            ALWAYS_INLINE void splice(const_iterator pos, IntrusiveList &o, const_iterator first) {
                this->impl.splice(pos.GetImplIterator(), o.impl, first.GetImplIterator());
            }

            ALWAYS_INLINE void splice(const_iterator pos, IntrusiveList &o, const_iterator first, const_iterator last) {
                this->impl.splice(pos.GetImplIterator(), o.impl, first.GetImplIterator(), last.GetImplIterator());
            }

            ALWAYS_INLINE iterator erase(const_iterator pos) {
                return iterator(this->impl.erase(pos.GetImplIterator()));
            }

            ALWAYS_INLINE void clear() {
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

            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(Derived &parent) {
                return parent.*Member;
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(Derived const &parent) {
                return parent.*Member;
            }

            static constexpr ALWAYS_INLINE Derived &GetParent(IntrusiveListNode &node) {
                return util::GetParentReference<Member, Derived>(&node);
            }

            static constexpr ALWAYS_INLINE Derived const &GetParent(IntrusiveListNode const &node) {
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

            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(Derived &parent) {
                return parent.*Member;
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(Derived const &parent) {
                return parent.*Member;
            }

            static constexpr ALWAYS_INLINE Derived &GetParent(IntrusiveListNode &node) {
                return util::GetParentReference<Member, Derived>(&node);
            }

            static constexpr ALWAYS_INLINE Derived const &GetParent(IntrusiveListNode const &node) {
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

            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(Derived &parent) {
                return static_cast<IntrusiveListNode &>(parent);
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(Derived const &parent) {
                return static_cast<const IntrusiveListNode &>(parent);
            }

            static constexpr ALWAYS_INLINE Derived &GetParent(IntrusiveListNode &node) {
                return static_cast<Derived &>(node);
            }

            static constexpr ALWAYS_INLINE Derived const &GetParent(IntrusiveListNode const &node) {
                return static_cast<const Derived &>(node);
            }
    };

    #pragma GCC pop_options

}