#pragma once

#include <mesosphere/core/util.hpp>
#include <atomic>

namespace mesosphere
{

template<typename T>
class KSlabStack {
    public:
    using pointer = T *;
    using const_pointer = const T *;
    using void_pointer = void *;
    using const_void_ptr = const void *;
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    private:
    struct Node {
        Node *next;
    };

    std::atomic<Node *> head;
    public:

    void push(T *data) noexcept
    {
        Node *newHead = (Node *)data;
        Node *oldHead = head.load();
        do {
            newHead->next = oldHead;
        } while(!head.compare_exchange_weak(oldHead, newHead));
    }

    T *pop() noexcept
    {
        Node *newHead;
        Node *oldHead = head.load();
        if (oldHead == nullptr) {
            return nullptr;
        } else {
            do {
                newHead = oldHead == nullptr ? oldHead : oldHead->next;
            } while(!head.compare_exchange_weak(oldHead, newHead));

            return (T *)oldHead;
        }
    }

    KSlabStack() noexcept = default;

    // Not reentrant (unlike NN's init function)
    void initialize(void *buffer, size_t size) noexcept
    {
        T *ar = (T *)buffer;
        if (size == 0) {
            return;
        }

        Node *ndlast = (Node *)&ar[size - 1];
        ndlast->next = nullptr;

        for (size_t i = 0; i < size - 1; i++) {
            Node *nd = (Node *)&ar[i];
            Node *ndnext = (Node *)&ar[i + 1];
            nd->next = ndnext;
        }

        Node *ndfirst = (Node *)&ar[0];
        head.store(ndfirst);
    }

    KSlabStack(void *buffer, size_t size) { initialize(buffer, size); }
};

}
