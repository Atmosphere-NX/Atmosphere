#include <mesosphere/core/KObjectRegistry.hpp>
#include <utility>

namespace mesosphere
{

KObjectRegistry KObjectRegistry::instance{};

const KObjectRegistry::Node *KObjectRegistry::FindImpl(const char *name) const
{
    auto it = std::find_if(
        nameNodes.cbegin(),
        nameNodes.cend(),
        [name](const Node &nd) {
            return std::strncmp(name, nd.name, sizeof(nd.name)) == 0;
        }
    );

    return it == nameNodes.cend() ? nullptr : &*it;
}

KObjectRegistry::Node *KObjectRegistry::FindImpl(const char *name)
{
    auto it = std::find_if(
        nameNodes.begin(),
        nameNodes.end(),
        [name](const Node &nd) {
            return std::strncmp(name, nd.name, sizeof(nd.name)) == 0;
        }
    );

    return it == nameNodes.end() ? nullptr : &*it;
}

SharedPtr<KAutoObject> KObjectRegistry::Find(const char *name) const
{
    std::scoped_lock guard{mutex};
    const Node *node = FindImpl(name);
    return node == nullptr ? nullptr : node->obj;
}


Result KObjectRegistry::Register(SharedPtr<KAutoObject> obj, const char *name)
{
    std::scoped_lock guard{mutex};
    Node *node = FindImpl(name);
    if (node != nullptr) {
        // Name already exists, just replace the auto object.
        node->obj = std::move(obj);
        return ResultSuccess();
    } else {
        if (nameNodes.emplace_back_or_fail(std::move(obj), name) == nullptr) {
            return ResultKernelInvalidState();
        }
        return ResultSuccess();
    }
}

Result KObjectRegistry::Unregister(const char *name)
{
    std::scoped_lock guard{mutex};

    auto it = std::find_if(
        nameNodes.cbegin(),
        nameNodes.cend(),
        [name](const Node &nd) {
            return std::strncmp(name, nd.name, sizeof(nd.name)) == 0;
        }
    );

    if (it == nameNodes.cend()) {
        return ResultKernelNotFound();
    } else {
        nameNodes.erase(it);
        return ResultSuccess();
    }
}

}