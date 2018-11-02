#pragma once

#include <iterator>
#include <utility>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <type_traits>
#include <mesosphere/core/util.h>
#include <mesosphere/interfaces/ISlabAllocated.hpp>

namespace mesosphere
{

template<typename T>
class KLinkedList final {
    private:


    struct List final {
        struct Node final : public ISlabAllocated<Node> {

            struct Link final {
                Link *prev = nullptr;
                Link *next = nullptr;

                Node &parent()
                {
                    return *detail::GetParentFromMember(this, &Node::link);
                }

                const Node &parent() const
                {
                    return *detail::GetParentFromMember(this, &Node::link);
                }

                T &data() { return parent().data; }
                const T &data() const { return parent().data; }
            };

            Link link{};
            T data{};

            Node() = default;
            Node(const Node &other) = default;
            Node(Node &&other) = default;
            explicit Node(const T &data) : ISlabAllocated<Node>(), data{data} {}
            explicit Node(const T &&data) : ISlabAllocated<Node>(), data{data} {}
            template<typename ...Args>
            explicit Node(Args&& ...args) : ISlabAllocated<Node>(), data{std::forward<Args>(args)...} {}
        };

        mutable typename Node::Link link{};
        typename Node::Link *last() const { return link.prev; }
        typename Node::Link *first() const { return link.next; }

        size_t size = 0;
    };

    List list;

    void insert_node_after(typename List::Node::Link *pos, typename List::Node::Link *nd)
    {
        // if pos is last & list is empty, ->next writes to first, etc.
        pos->next->prev = nd;
        nd->prev = pos;
        nd->next = pos->next;
        pos->next = nd;
        ++list.size;
    }

    void insert_node_before(typename List::Node::Link *pos, typename List::Node::Link *nd)
    {
        pos->prev->next = nd;
        nd->prev = pos->prev;
        nd->next = pos;
        pos->prev = nd;
        ++list.size;
    }

    void remove_node(typename List::Node::Link *nd)
    {
        nd->prev->next = nd->next;
        nd->next->prev = nd->prev;
        --list.size;
    }
    public:

    template<bool isConst>
    class Iterator {
        public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename KLinkedList::value_type;
        using difference_type = typename KLinkedList::difference_type;
        using pointer = typename std::conditional<
            isConst,
            typename KLinkedList::const_pointer,
            typename KLinkedList::pointer
        >::type;
        using reference = typename std::conditional<
            isConst,
            typename KLinkedList::const_reference,
            typename KLinkedList::reference
        >::type;

        bool operator==(const Iterator &other) const
        {
            return node == other.node;
        }

        bool operator!=(const Iterator &other) const
        {
            return !(*this == other);
        }

        reference operator*()
        {
            return node->data();
        }

        pointer operator->()
        {
            return &node->data();
        }

        Iterator &operator++()
        {
            node = node->next;
            return *this;
        }

        Iterator &operator--() {
            node = node->prev;
            return *this;
        }

        Iterator &operator++(int) {
            const Iterator v{*this};
            ++(*this);
            return v;
        }

        Iterator &operator--(int) {
            const Iterator v{*this};
            --(*this);
            return v;
        }

        // allow implicit const->non-const
        Iterator(const Iterator<false> &other) : node{other.node} {}

        friend class Iterator<true>;
        Iterator() = default;

        private:
        friend class KLinkedList;

        typename KLinkedList::List::Node::Link *node;

        explicit Iterator(typename KLinkedList::List::Node::Link *node) : node{node} {}
    };

    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using void_pointer = void *;
    using const_void_ptr = const void *;
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    KLinkedList() : list{{&list.link, &list.link}} {};
    explicit KLinkedList(size_t count, const T &value) : KLinkedList()
    {
        for (size_t i = 0; i < count; i++) {
            auto *nd = new typename List::Node{value};
            kassert(nd != nullptr);
            insert_node_after(list.last(), &nd->link);
        }
    }

    explicit KLinkedList(size_t count) : KLinkedList()
    {
        for (size_t i = 0; i < count; i++) {
            auto *nd = new typename List::Node;
            kassert(nd != nullptr);
            insert_node_after(list.last(), &nd->link);
        }
    }

    template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    KLinkedList(InputIt first, InputIt last) : KLinkedList()
    {
        for (InputIt it = first; it != last; ++it) {
            auto *nd = new typename List::Node{*it};
            kassert(nd != nullptr);
            insert_node_after(list.last(), &nd->link);
        }
    }

    void swap(KLinkedList &other) noexcept
    {
        using std::swap; // Enable ADL
        swap(list.link, other.list.link);
        swap(list.size, other.list.size);
        list.first()->prev = list.last()->next = &list.link;
        other.list.first()->prev = other.list.last()->next = &other.list.link;
    }

    KLinkedList(KLinkedList &&other) noexcept : KLinkedList()
    {
        swap(other);
    }
    KLinkedList(std::initializer_list<T> ilist) : KLinkedList(ilist.begin(), ilist.end()) {}

    void clear() noexcept
    {
        typename List::Node::Link *nxt;
        for (typename List::Node::Link *nd = list.first(); nd != &list.link; nd = nxt) {
            nxt = nd->next;
            delete &nd->parent();
        }
        list.size = 0;
    }

    ~KLinkedList()
    {
        clear();
    }

    KLinkedList &operator=(KLinkedList other)
    {
        swap(other);
        return *this;
    }

    KLinkedList &operator=(std::initializer_list<T> ilist)
    {
        KLinkedList tmp{ilist};
        swap(tmp);
        return *this;
    }

    void assign(size_t count, const T &value)
    {
        KLinkedList tmp{count, value};
        swap(tmp);
    }

    template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    void assign(InputIt first, InputIt last)
    {
        KLinkedList tmp{first, last};
        swap(tmp);
    }

    void assign(std::initializer_list<T> ilist)
    {
        KLinkedList tmp{ilist};
        swap(tmp);
    }

    T &front() { return list.first()->data(); }
    const T &front() const { return list.first()->data(); }

    T &back() { return list.last()->data(); }
    const T &back() const { return list.last()->data(); }

    const_iterator cbegin() const noexcept { return const_iterator{list.first()}; }
    const_iterator cend() const noexcept { return const_iterator{&list.link}; }

    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator end() const noexcept { return cend(); }

    iterator begin() noexcept { return iterator{list.first()}; }
    iterator end() noexcept { return iterator{&list.link}; }

    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{list.last()}; }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator{&list.link}; }

    const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    const_reverse_iterator rend() const noexcept { return crend(); }

    reverse_iterator rbegin() noexcept { return reverse_iterator{list.last()}; }
    reverse_iterator rend() noexcept { return reverse_iterator{&list.link}; }

    KLinkedList(const KLinkedList &other) : KLinkedList(other.cbegin(), other.cend()) {}

    constexpr size_t size() const noexcept { return list.size; }
    constexpr bool empty() const noexcept { return list.size == 0; }

    iterator insert(const_iterator pos, const T &value)
    {
        auto *nd = new typename List::Node{value};
        kassert(nd != nullptr);
        insert_node_after(pos.node, &nd->link);
    }

    iterator insert(const_iterator pos, T &&value)
    {
        auto *nd = new typename List::Node{value};
        kassert(nd != nullptr);
        insert_node_before(pos.node, &nd->link);
        return iterator{&nd->link};
    }

    iterator insert(const_iterator pos, size_t count, const T &value)
    {
        iterator ret = pos;
        for (size_t i = 0; i < count; i++) {
            ret = insert(ret, value);
        }
        return ret;
    }

    template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        // Note: our list definition allows --begin() to be well defined
        typename List::Node::Link *f = nullptr;
        typename List::Node::Link *p = pos.node->prev;
        for(InputIt it = first; it != last; ++it) {
            auto *nd = new typename List::Node{*it};
            kassert(nd != nullptr);
            insert_node_after(p, &nd->link);
            p = &nd->link;
            f = f == nullptr ? p : f;
        }

        return iterator{f};
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist)
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template<typename ...Args>
    iterator emplace(const_iterator pos, Args &&...args)
    {
        auto *nd = new typename List::Node{std::forward<Args>(args)...};
        kassert(nd != nullptr);
        insert_node_before(pos.node, &nd->link);
        return iterator{&nd->link};
    }

    iterator erase(const_iterator pos)
    {
        iterator ret{pos.node->next};
        remove_node(pos.node);
        delete &pos.node->parent();
        return ret;
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        const_iterator next;
        for (const_iterator it = first; it != last; it = next) {
            next = erase(it);
        }
        return iterator{next.node};
    }

    void push_back(const T &value)
    {
        auto *nd = new typename List::Node{value};
        kassert(nd != nullptr);
        insert_node_after(list.last(), &nd->link);
    }

    void push_back(T &&value)
    {
        auto *nd = new typename List::Node{value};
        kassert(nd != nullptr);
        insert_node_after(list.last(), &nd->link);
    }

    template< class... Args>
    T &emplace_back(Args&&... args)
    {
        auto *nd = new typename List::Node{std::forward<Args>(args)...};
        kassert(nd != nullptr);
        insert_node_after(list.last(), &nd->link);
        return nd->data;
    }

    void pop_back()
    {
        auto *nd = list.last();
        remove_node(nd);
        delete &nd->parent();
    }

    void push_front(const T &value)
    {
        auto *nd = new typename List::Node{value};
        kassert(nd != nullptr);
        insert_node_before(list.first(), &nd->link);
    }

    void push_front(T &&value)
    {
        auto *nd = new typename List::Node{value};
        kassert(nd != nullptr);
        insert_node_before(list.first(), &nd->link);
    }

    template< class... Args>
    T &emplace_front(Args&&... args)
    {
        auto *nd = new typename List::Node{std::forward<Args>(args)...};
        kassert(nd != nullptr);
        insert_node_before(list.first(), &nd->link);
        return nd->data;
    }

    void pop_front()
    {
        auto *nd = list.first();
        remove_node(nd);
        delete &nd->parent();
    }

    void resize(size_t count)
    {
        if (count < list.size) {
            while (count < list.size) {
                pop_back();
            }
        } else {
            size_t s = list.size;
            for (size_t i = 0; i < count - s; i++) {
                auto *nd = new typename List::Node;
                kassert(nd != nullptr);
                insert_node_after(list.last(), &nd->link);
            }
        }
    }

    void resize(size_t count, const T &value)
    {
        if (count < list.size) {
            while (count < list.size) {
                pop_back();
            }
        } else {
            size_t s = list.size;
            for (size_t i = 0; i < count - s; i++) {
                auto *nd = new typename List::Node{value};
                kassert(nd != nullptr);
                insert_node_after(list.last(), &nd->link);
            }
        }
    }

    void splice(const_iterator pos, KLinkedList &&other)
    {
        //auto *current = pos.node;
        auto *before = pos.node->prev;
        auto *after = pos.node; //current == &list.link ? current : pos.node->next;
        before->next = other.list.first();
        before->next->prev = before;
        after->prev = other.list.last();
        after->prev->next = after;

        list.size += other.list.size;
        other.list.size = 0;
        other.list.link.prev = other.list.link.next = &other.list.link;
    }

    void splice(const_iterator pos, KLinkedList &other)
    {
        splice(pos, std::move(other));
    }

    void splice(const_iterator pos, KLinkedList &&other, const_iterator it)
    {
        other.remove_node(it.node);
        insert_node_before(pos.node, it.node);
    }

    void splice(const_iterator pos, KLinkedList &other, const_iterator it)
    {
        splice(pos, std::move(other), it);
    }

    void splice(const_iterator pos, KLinkedList &&other, const_iterator first, const_iterator last)
    {
        if (*this == other) {
            auto *current = pos.node;
            auto *before = pos.node->prev;
            auto *after = current == &list.link ? current : pos.node->next;
            before->next = first.node;
            before->next->prev = before;
            after->prev = last.node;
            after->prev->next = after;
        } else {
            // Note: the first, last version can't be O(1) because std::distance is O(n)...
            const_iterator next;
            for (const_iterator it = first; it != last; it = next) {
                next = it;
                ++next;
                splice(pos, other, it);
                ++pos;
            }
        }
    }

    void splice(const_iterator pos, KLinkedList &other, const_iterator first, const_iterator last)
    {
        splice(pos, std::move(other), first, last);
    }

    size_t remove(const T &value)
    {
        size_t n = 0;
        const_iterator next;
        for (const_iterator it = cbegin(); it != cend(); ) {
            if (*it == value) {
                it = erase(it);
                ++n;
            } else {
                ++it;
            }
        }
        return n;
    }

    template<typename UnaryPredicate>
    size_t remove_if(UnaryPredicate p)
    {
        const_iterator next;
        size_t n = 0;
        for (const_iterator it = cbegin(); it != cend(); ) {
            if (p(*it)) {
                it = erase(it);
                ++n;
            } else {
                ++it;
            }
        }
        return n;
    }

    template<typename Compare>
    void merge(KLinkedList &&other, Compare p)
    {
        typename List::Node::Link hd{};
        auto *cur = &hd;
        size_t n = 0;
        while (list.size > 0 && other.list.size > 0) {
            if (p(list.first()->data(), other.list.first()->data())) {
                cur->next = list.first();
                remove_node(list.first());
            } else {
                cur->next = other.list.first();
                other.remove_node(other.list.first());
            }
            cur->next->prev = cur;
            cur = cur->next;
            n++;
        }

        // Steal the remaining elements
        if (list.size > 0) {
            cur->next = list.first();
            list.first()->prev = cur;
            cur = list.last();
            n += list.size;
        } else if (other.list.size > 0) {
            cur->next = other.list.first();
            other.list.first()->prev  = cur;
            cur = other.list.last();
            n += other.list.size;
        }


        // Reset the other list to put it in a valid state
        other.list.link.prev = other.list.link.next = &other.list.link;
        other.list.size = 0;

        // Finally, normalize the result and assign it to this
        list.link.next = hd.next;
        list.link.prev = cur;
        list.size = n;
        list.first()->prev = list.last()->next = &list.link;
    }

    void merge(KLinkedList &&other)
    {
        merge(other, std::less<T>{});
    }

    template<typename Compare>
    void merge(KLinkedList &other, Compare p)
    {
        merge(std::move(other), p);
    }

    void merge(KLinkedList &other)
    {
        merge(other, std::less<T>{});
    }

    void reverse() noexcept
    {
        typename List::Node::Link hd{};
        auto *cur = &hd;
        size_t n = 0;

        while (list.size > 0) {
            cur->next = list.last();
            remove_node(list.last());
            cur->next->prev = cur;
            n++;
            cur = cur->next;
        }

        list.link.next = hd.next;
        list.link.prev = cur;
        list.size = n;
        list.first()->prev = list.last()->next = &list.link;
    }


    template<typename BinaryPredicate>
    size_t unique(BinaryPredicate p)
    {
        typename List::Node::Link *nxt;
        size_t n = 0;
        for (auto *nd = list.first(); nd != &list.link; nd = nxt) {
           nxt = nd->next;
           for (auto *nd2 = nxt; nd2 != &list.link && p(nd->data(), nd2->data()); nd2 = nxt) {
               nxt = nd2->next;
               remove_node(nd2);
               delete &nd2->parent();
               ++n;
           }
        }

        return n;
    }

    size_t unique()
    {
        return unique(std::equal_to<T>{});
    }

    // sort: a PITA to implement and not needed anyway

    friend bool operator==(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
    }

    friend bool operator!=(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::less<T>{});
    }

    friend bool operator>(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::greater<T>{});
    }

    friend bool operator<=(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::less_equal<T>{});
    }

    friend bool operator>=(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::greater_equal<T>{});
    }

    friend void swap(KLinkedList &lhs, KLinkedList &rhs)
    {
        lhs.swap(rhs);
    }

};

template<typename InputIt>
KLinkedList(InputIt b, InputIt e) -> KLinkedList<typename std::iterator_traits<InputIt>::value_type>;

}
