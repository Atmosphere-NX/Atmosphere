#pragma once

#include <mesosphere/core/util.hpp>
#include <atomic>
#include <type_traits>

#define MESOSPHERE_AUTO_OBJECT_TRAITS(BaseId, DerivedId)\
using BaseClass = K##BaseId ;\
static constexpr KAutoObject::TypeId typeId = KAutoObject::TypeId::DerivedId;\
virtual ushort GetClassToken() const\
{\
    return KAutoObject::GenerateClassToken<K##DerivedId >();\
}\

namespace mesosphere
{

// Foward declarations for intrusive_ptr
class KProcess;
class KResourceLimit;
class KThread;

void intrusive_ptr_add_ref(KProcess *obj);
void intrusive_ptr_release(KProcess *obj);

void intrusive_ptr_add_ref(KResourceLimit *obj);
void intrusive_ptr_release(KResourceLimit *obj);

class KAutoObject {
    public:

    /// Class token for polymorphic type checking
    virtual ushort GetClassToken() const
    {
        return 0;
    }

    /// Comparison key for KObjectAllocator
    virtual u64 GetComparisonKey() const
    {
        return (u64)(uiptr)this;
    }

    /// Is alive (checked for deletion)
    virtual bool IsAlive() const = 0;

    /// Virtual destructor
    virtual ~KAutoObject();


    /// Check if the offset is base class of T or T itself
    template<typename T>
    bool IsInstanceOf() const
    {
        ushort btoken = GenerateClassToken<T>();
        ushort dtoken = GetClassToken();

        return (dtoken & btoken) == btoken;
    }

    // Explicitely disable copy and move, and add default ctor
    KAutoObject() = default;
    KAutoObject(const KAutoObject &) = delete;
    KAutoObject(KAutoObject &&) = delete;
    KAutoObject &operator=(const KAutoObject &) = delete;
    KAutoObject &operator=(KAutoObject &&) = delete;

    /// Type order as found in official kernel
    enum class TypeId : ushort {
        AutoObject = 0,
        SynchronizationObject,
        ReadableEvent,

        FinalClassesMin = 3,

        InterruptEvent = 3,
        Debug,
        ClientSession,
        Thread,
        Process,
        Session,
        ServerPort,
        ResourceLimit,
        SharedMemory,
        LightClientSession,
        ServerSession,
        LightSession,
        Event,
        LightServerSession,
        DeviceAddressSpace,
        ClientPort,
        Port,
        WritableEvent,
        TransferMemory,
        SessionRequest,
        CodeMemory, // JIT

        Max = CodeMemory + 1,
    };

    private:
    std::atomic<ulong> referenceCount{0}; // official kernel has u32 for this
    friend void intrusive_ptr_add_ref(KAutoObject *obj);
    friend void intrusive_ptr_release(KAutoObject *obj);

    protected:

    template<typename T>
    static constexpr ushort GenerateClassToken()
    {
        /* The token follows these following properties:
            * Multiple inheritance is not supported
            * (BaseToken & DerivedToken) == BaseToken
            * The token for KAutoObject is 0
            * Not-final classes have a token of (1 << (typeid - 1))
            * Final derived classes have a unique token part of Seq[typeid - DerivedClassMin] | 0x100,
            where Seq is (in base 2) 11, 101, 110, 1001, 1010, and so on...
        */
       if constexpr (std::is_same_v<T, KAutoObject>) {
           return 0;
       } else if constexpr (!std::is_final_v<T>) {
           static_assert(T::typeId >= TypeId::SynchronizationObject && T::typeId < TypeId::FinalClassesMin, "Invalid type ID!");
           return (1 << ((ushort)T::typeId - 1)) | GenerateClassToken<typename T::BaseClass>();
       } else {
           static_assert(T::typeId >= TypeId::FinalClassesMin && T::typeId < TypeId::Max, "Invalid type ID!");
           ushort off = (ushort)T::typeId - (ushort)TypeId::FinalClassesMin;
           return ((ushort)detail::A038444(off) << 9) | 0x100u | GenerateClassToken<typename T::BaseClass>();
       }
    }
};

inline void intrusive_ptr_add_ref(KAutoObject *obj)
{
    ulong oldval = obj->referenceCount.fetch_add(1);
    kassert(oldval + 1 != 0);
}

inline void intrusive_ptr_release(KAutoObject *obj)
{
    ulong oldval = obj->referenceCount.fetch_sub(1);
    if (oldval - 1 == 0) {
        delete obj;
    }
}

template <typename T>
inline SharedPtr<T> DynamicObjectCast(SharedPtr<KAutoObject> object) {
    if (object != nullptr && object->IsInstanceOf<T>()) {
        return boost::static_pointer_cast<T>(object);
    }
    return nullptr;
}

}
