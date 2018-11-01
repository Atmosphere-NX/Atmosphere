#pragma once

#include <iterator>
#include <utility>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <type_traits>
#include <mesosphere/interfaces/ISlabAllocated.hpp>

namespace mesosphere
{

template<typename T>
class KLinkedList final {
    private:

    struct List {
        Node *last;
        Node *first;
        size_t size = 0;
    };

    List list;

    struct Node final : public ISlabAllocated<Node> {
        //friend class KLinkedList;
        Node *prev = nullptr;
        Node *next = nullptr;
        T data{};

        Node() = default;
        Node(const Node &other) = default;
        Node(Node &&other) = default;
        explicit Node(const T &data) : Node(), data{other} {}
        explicit Node(const T &&data) : Node(), data{other} {}
        template<typename ...Args>
        explicit Node(Args&& ...args) : Node(), data{std::forward<Args>(args)...} {}
    };


    void insert_node_after(Node *pos, Node *nd)
    {
        // if pos is last & list is empty, ->next writes to first, etc.
        pos->next->prev = nd;
        nd->prev = pos;
        nd->next = pos->next;
        pos->next = nd;
        ++list.size;
    }

    void insert_node_before(Node *pos, Node *nd)
    {
        pos->prev->next = nd;
        nd->prev = pos->prev;
        nd->next = pos;
        pos->prev = nd;
        ++list.size;
    }

    void remove_node(Node *nd)
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
            return node->data;
        }

        pointer operator->()
        {
            return &node->data;
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

        KLinkedList::Node *node;

        explicit Iterator(KLinkedList::Node *node) : node{node} {}
    };

    using pointer = T *;
    using const_pointer = const T *;
    using void_pointer = void *;
    using const_void_ptr = const void *;
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    KLinkedList() : list{(Node *)&list, (Node *)&list} {};
    explicit KLinkedList(size_t count, const T &value) : KLinkedList()
    {
        for (size_t i = 0; i < count; i++) {
            Node *nd = new Node{value};
            kassert(nd != nullptr);
            insert_node_after(list.last, nd);
        }
    }

    explicit KLinkedList(size_t count) : KLinkedList()
    {
        for (size_t i = 0; i < count; i++) {
            Node *nd = new Node;
            kassert(nd != nullptr);
            insert_node_after(list.last, nd);
        }
    }

    template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    explicit KLinkedList(InputIt first, InputIt last) : KLinkedList()
    {
        for (InputIt it = first; it != last; ++it) {
            Node *nd = new Node{*it};
            kassert(nd != nullptr);
            insert_node_after(list.last, nd);
        }
    }

    void swap(KLinkedList &other) noexcept
    {
        using std::swap; // Enable ADL
        swap(list.first, other.list.first);
        swap(list.last, other.list.last);
        swap(list.size, other.list.size);
    }

    KLinkedList(KLinkedList &&other) noexcept : KLinkedList()
    {
        swap(other);
    }
    KLinkedList(std::initializer_list<T> ilist) : KLinkedList(ilist.begin(), ilist.end()) {}

    void clear() noexcept
    {
        Node *nxt;
        for (Node nd = list.first; nd != (Node *)&list; nd = nxt) {
            nxt = nd->next;
            delete nd;
        }
        list.size = 0;
    }

    ~KLinkedList()
    {
        clear();
    }

    KLinkedList &operator=(KLinkedList other)
    {
        swap(*this, other);
        return *this;
    }

    KLinkedList &operator=(std::initializer_list<T> ilist)
    {
        KLinkedList tmp{ilist};
        swap(*this, tmp);
        return *this;
    }

    void assign(size_t count, const T &value)
    {
        KLinkedList tmp{count, value};
        swap(*this, tmp);
    }

    template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    void assign(InputIt first, InputIt last)
    {
        KLinkedList tmp{first, last};
        swap(*this, tmp);
    }

    void assign(std::initializer_list<T> ilist)
    {
        KLinkedList tmp{ilist};
        swap(*this, tmp);
    }

    T &front() { return list.first->data; }
    const T &front() const { return list.first->data; }

    T &back() { return list.last->data; }
    const T &back(); const { return list.last->data; }

    const_iterator cbegin() const noexcept { return const_iterator{list.first}; }
    const_iterator cend() const noexcept { return const_iterator{(Node *)&list}; }

    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator end() const noexcept { return cend(); }

    iterator begin() noexcept { return iterator{list.first}; }
    iterator end() noexcept { return iterator{(Node *)&list}; }

    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{list.last}; }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator{(Node *)&list}; }

    const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    const_reverse_iterator rend() const noexcept { return crend(); }

    reverse_iterator rbegin() const noexcept { return reverse_iterator{list.last}; }
    reverse_iterator rend() const noexcept { return reverse_iterator{(Node *)&list}; }

    KLinkedList(const KLinkedList &other) : KLinkedList(other.cbegin(), other.cend()) {}

    constexpr size_t size() const noexcept { return list.size; }
    constexpr bool empty() const noexcept { return list.size == 0; }

    iterator insert(const_iterator pos, const T &value)
    {
        Node *nd = new Node{value};
        kassert(nd != nullptr);
        insert_node_after(pos.node, nd);
    }

    iterator insert(const_iterator pos, T &&value)
    {
        Node *nd = new Node{value};
        kassert(nd != nullptr);
        insert_node_before(pos.node, nd);
        return iterator{nd};
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
        Node *f = nullptr;
        Node *p = pos.node->prev;
        for(InputIt it = first; it != last; ++it) {
            Node *nd = new Node{value};
            kassert(nd != nullptr);
            insert_node_after(p, nd);
            p = nd;
            f = f == nullptr ? nd : f;
        }

        return iterator{f};
    }

    template<typename ...Args>
    iterator emplace(const_iterator pos, Args &&...args)
    {
        Node *nd = new Node{std::forward<Args>(args)...};
        kassert(nd != nullptr);
        insert_node_before(pos.node, nd);
        return iterator{nd};
    }

    iterator erase(const_iterator pos)
    {
        iterator ret{pos->next};
        remove_node(pos.node);
        delete pos.node;
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
        Node *nd = new Node{value};
        kassert(nd != nullptr);
        insert_node_after(list.last, nd);
    }

    void push_back(T &&value)
    {
        Node *nd = new Node{value};
        kassert(nd != nullptr);
        insert_node_after(list.last, nd);
    }

    template< class... Args>
    T &emplace_back(Args&&... args)
    {
        Node *nd = new Node{std::forward<Args>(args)...};
        kassert(nd != nullptr);
        insert_node_after(list.last, nd);
        return nd->data;
    }

    void pop_back()
    {
        remove_node(list.last);
        delete list.last;
    }

    void push_front(const T &value)
    {
        Node *nd = new Node{value};
        kassert(nd != nullptr);
        insert_node_before(list.first, nd);
    }

    void push_front(T &&value)
    {
        Node *nd = new Node{value};
        kassert(nd != nullptr);
        insert_node_before(list.first, nd);
    }

    template< class... Args>
    T &emplace_front(Args&&... args)
    {
        Node *nd = new Node{std::forward<Args>(args)...};
        kassert(nd != nullptr);
        insert_node_before(list.first, nd);
        return nd->data;
    }

    void pop_front()
    {
        remove_node(list.first);
    }

    void resize(size_t count)
    {
        if (count < list.size) {
            while (count < list.size) {
                pop_back();
            }
        } else {
            for (size_t i = 0; i < count - list.size; i++) {
                Node *nd = new Node;
                kassert(nd != nullptr);
                insert_node_after(list.last, nd);
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
            for (size_t i = 0; i < count - list.size; i++) {
                Node *nd = new Node{value};
                kassert(nd != nullptr);
                insert_node_after(list.last, nd);
            }
        }
    }

    void splice(const_iterator pos, KLinkedList &&other)
    {
        Node *before = pos.node->prev;
        Node *after = pos.node->next;
        before->next = other.list.first;
        before->next->prev = before;
        after->prev = other.list.last;
        after->prev->next = after;

        list.size += other.list.size;
        other.list.size = 0;
        other.list.first = other.list.last = (Node *)&other.list;
    }

    void splice(const_iterator pos, KLinkedList &&other, const_iterator it)
    {
        other.remove_node(it.node);
        insert_node_before(pos.node, it.node);
    }

    void splice(const_iterator pos, KLinkedList &&other, const_iterator first, const_iterator last)
    {
        if (*this == other) {
            Node *before = pos.node->prev;
            Node *after = pos.node->next;
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
    void merge(KLinkedList &other, Compare p)
    {
        Node hd{};
        Node *cur = &hd;
        size_t n = 0;
        while (list.size > 0 && other.list.size > 0) {
            if (p(list.first, other.list.first)) {
                cur->next = list.first;
                remove_node(list.first);
            } else {
                cur->next = other.list.first;
                other.remove_node(other.list.first);
            }
            cur->next->prev = cur;
            cur = cur->next;
            n++;
        }

        // Steal the remaining elements
        if (list.size > 0) {
            cur->next = list.first;
            list.first->prev  = cur;
            n += list.size;
        } else if (other.list.size > 0) {
            cur->next = other.list.first;
            other.list.first->prev  = cur;
            n += other.list.size;
        }

        // Reset the other list to put it in a valid state
        other.list.first = other.list.last = (Node *)other.list;
        other.list.size = 0;

        // Finally, normalize the result and assign it to this
        list.first = hd.next;
        list.last = cur;
        list.size = n;
        list.first->prev = list.last->next = (Node *)list;
    }

    void merge(KLinkedList &other)
    {
        merge(other, std::less{});
    }

    template<typename Compare>
    void merge(KLinkedList &&other, Compare p)
    {
        KLinkedList &o = other;
        merge(o, p);
    }

    void merge(KLinkedList &&other)
    {
        merge(other, std::less{});
    }

    void reverse() noexcept
    {
        Node hd{};
        Node *cur = &hd;
        size_t n = 0;

        while (list.size > 0) {
            cur->next = list.last;
            remove_node(list.last);
            cur->next->prev = cur;
            n++;
            cur = cur->next;
        }

        list.first = hd.next;
        list.last = cur;
        list.size = n;
        list.first->prev = list.last->next = (Node *)list;
    }


    template<typename BinaryPredicate>
    size_t unique(BinaryPredicate p)
    {
        Node *nxt;
        size_t n = 0;
        for (Node *nd = list.first; nd != (Node *)list; nd = nxt) {
           nxt = nd->next;
           for (Node *nd2 = nxt; nd2 != (Node *)list && p(nd->data, nd2->data); nd2 = nxt) {
               nxt = nd2->next;
               remove_node(nd2);
               delete nd2;
               ++n;
           }
        }

        return n;
    }

    size_t unique()
    {
        return unique(std::equal_to);
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
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::less{});
    }

    friend bool operator>(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::greater{});
    }

    friend bool operator<=(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::less_equal{});
    }

    friend bool operator>=(const KLinkedList &lhs, const KLinkedList &rhs)
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::greater_equal{});
    }

    friend void swap(KLinkedList &lhs, KLinkedList &rhs)
    {
        lhs.swap(rhs);
    }

};

}
