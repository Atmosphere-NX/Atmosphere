#pragma once

#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/core/KLinkedList.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/threading/KMutex.hpp>
#include <cstring>

namespace mesosphere
{

class KObjectRegistry {
    public:

    static KObjectRegistry &GetInstance() { return instance; }

    SharedPtr<KAutoObject> Find(const char *name) const;
    Result Register(SharedPtr<KAutoObject> obj, const char *name);
    Result Unregister(const char *name);

    private:

    struct Node {
        SharedPtr<KAutoObject> obj{};
        char name[12] = {0};

        Node() = default;
        Node(SharedPtr<KAutoObject> &&obj, const char *name) : obj{obj}
        {
            std::strncpy(this->name, name, sizeof(this->name));
        }
    };

    const Node *FindImpl(const char *name) const;
    Node *FindImpl(const char *name);

    KLinkedList<Node> nameNodes{};
    mutable KMutex mutex{};

    static KObjectRegistry instance;
};

}