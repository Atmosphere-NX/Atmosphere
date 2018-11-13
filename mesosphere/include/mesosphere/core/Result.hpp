#pragma once

#include <mesosphere/core/types.hpp>

namespace mesosphere
{

enum class ResultModule : uint {
    None = 0,
    Kernel = 1,
    /* Other modules not included. */
};
    
class ResultHelper {
    public:
    using BaseType = uint;
    static constexpr BaseType SuccessValue = BaseType();
    static constexpr BaseType ModuleBits = 9;
    static constexpr BaseType DescriptionBits = 13;
    
    template<ResultModule module, BaseType description>
    struct MakeResult : public std::integral_constant<BaseType, ((static_cast<BaseType>(module)) | (description << ModuleBits))> {
        static_assert(static_cast<BaseType>(module) < 1 << (ModuleBits + 1), "Invalid Module");
        static_assert(description < 1 << (DescriptionBits + 1), "Invalid Description");
    };
    
    static constexpr ResultModule GetModule(BaseType value) {
        return static_cast<ResultModule>(value & ~(~BaseType() << ModuleBits));
    }
    
    static constexpr BaseType GetDescription(BaseType value) {
        return ((value >> ModuleBits) & ~(~BaseType() << DescriptionBits));
    }
};
    

/* Use CRTP for Results. */
template<typename Self>
class ResultBase {
    public:
    using BaseType = typename ResultHelper::BaseType;
    static constexpr BaseType SuccessValue = ResultHelper::SuccessValue;
    
    constexpr bool IsSuccess() const { return static_cast<const Self *>(this)->GetValue() == SuccessValue; }
    constexpr bool IsFailure() const { return !IsSuccess(); }
    constexpr operator bool() const { return IsSuccess(); }
    constexpr bool operator !() const { return IsFailure(); }
    
    constexpr ResultModule GetModule() const { return static_cast<const Self *>(this)->GetValue(); }
    constexpr BaseType GetDescription() const { return static_cast<const Self *>(this)->GetValue(); }
};

/* Actual result type. */
class Result final : public ResultBase<Result> {
    public:
    using BaseType = typename ResultBase<Result>::BaseType;
    static constexpr BaseType SuccessValue = ResultBase<Result>::SuccessValue;
    
    constexpr Result() : value(SuccessValue) {}
    
    constexpr BaseType GetValue() const { return this->value; }
    constexpr bool operator==(const Result &other) const { return value == other.value; }
    constexpr bool operator!=(const Result &other) const { return value != other.value; }
    
    static constexpr Result MakeResult(BaseType v) { return Result(v); }
    
    private:
    BaseType value;
    constexpr explicit Result(BaseType v) : value(v) {}
};

static_assert(sizeof(Result) == sizeof(Result::BaseType), "Bad Result definition!");

/* Successful result class. */
class ResultSuccess final : public ResultBase<ResultSuccess> {
    public:
    using BaseType = typename ResultBase<ResultSuccess>::BaseType;
    static constexpr BaseType SuccessValue = ResultBase<ResultSuccess>::SuccessValue;
    
    constexpr operator Result() { return Result::MakeResult(SuccessValue); }
    constexpr BaseType GetValue() const { return SuccessValue; }
};

/* Error result class. */
template<ResultModule module, ResultHelper::BaseType description> 
class ResultError : public ResultBase<ResultError<module, description>> {
    public:
    using BaseType = typename ResultBase<ResultError<module, description>>::BaseType;
    static constexpr BaseType SuccessValue = ResultBase<ResultError<module, description>>::SuccessValue;
    
    static constexpr BaseType Value = ResultHelper::MakeResult<module, description>::value;
    static_assert(Value != SuccessValue, "Invalid ResultError");
    
    constexpr operator Result() { return Result::MakeResult(Value); }
    constexpr BaseType GetValue() const { return Value; }
};

#define DEFINE_RESULT(module, name, description) class Result##module##name final : public ResultError<ResultModule::module, description> {}

DEFINE_RESULT(Kernel, OutOfSessions,                7);

DEFINE_RESULT(Kernel, InvalidCapabilityDescriptor,  14);

DEFINE_RESULT(Kernel, NotImplemented,               33);
DEFINE_RESULT(Kernel, ThreadTerminating,            59);

DEFINE_RESULT(Kernel, OutOfDebugEvents,             70);

DEFINE_RESULT(Kernel, InvalidSize,                  101);
DEFINE_RESULT(Kernel, InvalidAddress,               102);
DEFINE_RESULT(Kernel, ResourceExhausted,            103);
DEFINE_RESULT(Kernel, OutOfMemory,                  104);
DEFINE_RESULT(Kernel, OutOfHandles,                 105);
DEFINE_RESULT(Kernel, InvalidMemoryState,           106);
DEFINE_RESULT(Kernel, InvalidMemoryPermissions,     108);
DEFINE_RESULT(Kernel, InvalidMemoryRange,           110);
DEFINE_RESULT(Kernel, InvalidPriority,              112);
DEFINE_RESULT(Kernel, InvalidCoreId,                113);
DEFINE_RESULT(Kernel, InvalidHandle,                114);
DEFINE_RESULT(Kernel, InvalidUserBuffer,            115);
DEFINE_RESULT(Kernel, InvalidCombination,           116);
DEFINE_RESULT(Kernel, TimedOut,                     117);
DEFINE_RESULT(Kernel, Cancelled,                    118);
DEFINE_RESULT(Kernel, OutOfRange,                   119);
DEFINE_RESULT(Kernel, InvalidEnumValue,             120);
DEFINE_RESULT(Kernel, NotFound,                     121);
DEFINE_RESULT(Kernel, AlreadyExists,                122);
DEFINE_RESULT(Kernel, ConnectionClosed,             123);
DEFINE_RESULT(Kernel, UnhandledUserInterrupt,       124);
DEFINE_RESULT(Kernel, InvalidState,                 125);
DEFINE_RESULT(Kernel, ReservedValue,                126);
DEFINE_RESULT(Kernel, InvalidHwBreakpoint,          127);
DEFINE_RESULT(Kernel, FatalUserException,           128);
DEFINE_RESULT(Kernel, OwnedByAnotherProcess,        129);
DEFINE_RESULT(Kernel, ConnectionRefused,            131);
DEFINE_RESULT(Kernel, OutOfResource,                132);

DEFINE_RESULT(Kernel, IpcMapFailed,                 259);
DEFINE_RESULT(Kernel, IpcCmdbufTooSmall,            260);

DEFINE_RESULT(Kernel, NotDebugged,                  520);

}
