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

#include<stdint.h>

template<typename Peripheral, typename _Base, uintptr_t _offset>
class RegisterAccessor {
  public:
    using Base = _Base;
    static const uintptr_t offset = _offset;

    __attribute__((always_inline)) RegisterAccessor(const Peripheral &p) :
        peripheral(p),
        ptr(reinterpret_cast<volatile Base *>(p.base_addr + _offset)),
        read_value(*ptr),
        write_value(read_value) {
    }

    __attribute__((always_inline)) RegisterAccessor(const Peripheral &p, Base value) :
        peripheral(p),
        ptr(reinterpret_cast<volatile Base *>(p.base_addr + _offset)),
        read_value(value),
        write_value(value) {
    }

    __attribute__((always_inline)) const Peripheral &Commit() {
        *ptr = write_value;
        return peripheral;
    }

    const Peripheral &peripheral;
    volatile Base *ptr;
    Base read_value;
    Base write_value;
};

struct rw1c_marker {};

template<typename Peripheral, typename _Base>
class BitArrayRegisterAccessor {
  public:
    using Base = _Base;

    __attribute__((always_inline)) BitArrayRegisterAccessor(const Peripheral &p, uintptr_t offset) :
        peripheral(p),
        ptr(reinterpret_cast<volatile Base *>(p.base_addr + offset)),
        read_value(*ptr),
        write_value(read_value) {
    }

    __attribute__((always_inline)) BitArrayRegisterAccessor(const Peripheral &p, uintptr_t offset, Base value) :
        peripheral(p),
        ptr(reinterpret_cast<volatile Base *>(p.base_addr + offset)),
        read_value(value),
        write_value(value) {
    }
    
    __attribute__((always_inline)) BitArrayRegisterAccessor(const Peripheral &p, uintptr_t offset, rw1c_marker _marker) :
        peripheral(p),
        ptr(reinterpret_cast<volatile Base *>(p.base_addr + offset)),
        read_value(*ptr),
        write_value(0) {
    }

    class BitAccessor {
        public:
        __attribute__((always_inline)) BitAccessor(BitArrayRegisterAccessor &reg, int idx) :
            reg(reg),
            idx(idx) {
        }

        __attribute__((always_inline)) bool Get() const {
            return (reg.read_value >> idx) & 1;
        }

        __attribute__((always_inline)) BitArrayRegisterAccessor &Set(bool value) {
            reg.write_value&= ~(1 << idx);
            reg.write_value|= (value << idx);
            return reg;
        }
        
        __attribute__((always_inline)) explicit operator bool() const {
            return Get();
        }
        
        __attribute__((always_inline)) BitArrayRegisterAccessor &operator=(bool value) {
            return Set(value);
        }

        __attribute__((always_inline)) BitArrayRegisterAccessor &Clear() {
            return Set(1);
        }
        
        private:
        BitArrayRegisterAccessor &reg;
        int idx;
    };
    
    __attribute__((always_inline)) BitAccessor operator[](int idx) {
        return BitAccessor(*this, idx);
    }
    
    __attribute__((always_inline)) const Peripheral &Commit() {
        *ptr = write_value;
        read_value = write_value;
        return peripheral;
    }

    const Peripheral &peripheral;
    volatile Base *ptr;
    Base read_value;
    Base write_value;
};

// [low, high] is an inclusive range, since that's the way these things are written in the TRM
template<typename RegAcc, uint8_t low, uint8_t high = low, typename Type = typename RegAcc::Base, uint8_t internal_shift = 0>
class RegisterFieldAccessorBase {
  public:
    __attribute__((always_inline)) RegisterFieldAccessorBase(RegAcc &acc) : acc(acc) { }

  protected:
    __attribute__((always_inline)) Type Get() {
        return Type(((this->acc.read_value & mask) >> low) << internal_shift);
    }

    __attribute__((always_inline)) RegAcc &Set(Type value) {
        this->acc.write_value&= ~mask;
        this->acc.write_value|= ((typename RegAcc::Base(value) >> internal_shift) << low) & mask;
        return acc;
    }

    public:
    static const typename RegAcc::Base mask = ((typename RegAcc::Base(2) << (high-low)) - 1) << low;
    protected:
    using FieldType = Type;
    using RegisterAccessor = RegAcc;
    RegAcc &acc;
};

// base accessors

template<typename RegAcc, uint8_t low, uint8_t high = low, typename Type = typename RegAcc::Base, uint8_t internal_shift = 0>
class RegisterRWFieldAccessorBase : public RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift> {
  public:
    __attribute__((always_inline)) RegisterRWFieldAccessorBase(RegAcc &acc) : RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift>(acc) {
    }

    __attribute__((always_inline)) Type Get() {
        return RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift>::Get();
    }

    __attribute__((always_inline)) explicit operator Type() {
        return Get();
    }
    
    __attribute__((always_inline)) RegAcc &Set(Type value) {
        return RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift>::Set(value);
    }
};

template<typename RegAcc, uint8_t low, uint8_t high = low, typename Type = typename RegAcc::Base, uint8_t internal_shift = 0>
class RegisterROFieldAccessorBase : public RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift> {
  public:
    __attribute__((always_inline)) RegisterROFieldAccessorBase(RegAcc &acc) : RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift>(acc) {
    }

    __attribute__((always_inline)) Type Get() {
        return RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift>::Get();
    }
    
    __attribute__((always_inline)) explicit operator Type() {
        return Get();
    }
};

template<typename RegAcc, uint8_t low, uint8_t high = low, typename Type = typename RegAcc::Base, uint8_t internal_shift = 0>
class RegisterWOFieldAccessorBase : public RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift> {
  public:
    __attribute__((always_inline)) RegisterWOFieldAccessorBase(RegAcc &acc) : RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift>(acc) {
    }

    __attribute__((always_inline)) RegAcc &Set(Type value) {
        return RegisterFieldAccessorBase<RegAcc, low, high, Type, internal_shift>::Set(value);
    }
};

// generic accessors

template<typename RegAcc, uint8_t low, uint8_t high = low, typename Type = typename RegAcc::Base, uint8_t internal_shift = 0>
class RegisterRWFieldAccessor : public RegisterRWFieldAccessorBase<RegAcc, low, high, Type, internal_shift> {
  public:
    __attribute__((always_inline)) RegisterRWFieldAccessor(RegAcc &acc) : RegisterRWFieldAccessorBase<RegAcc, low, high, Type, internal_shift>(acc) {
    }
};

template<typename RegAcc, uint8_t low, uint8_t high = low, typename Type = typename RegAcc::Base, uint8_t internal_shift = 0>
class RegisterROFieldAccessor : public RegisterROFieldAccessorBase<RegAcc, low, high, Type, internal_shift> {
  public:
    __attribute__((always_inline)) RegisterROFieldAccessor(RegAcc &acc) : RegisterROFieldAccessorBase<RegAcc, low, high, Type, internal_shift>(acc) {
    }
};

template<typename RegAcc, uint8_t low, uint8_t high = low, typename Type = typename RegAcc::Base, uint8_t internal_shift = 0>
class RegisterWOFieldAccessor : public RegisterWOFieldAccessorBase<RegAcc, low, high, Type, internal_shift> {
  public:
    __attribute__((always_inline)) RegisterWOFieldAccessor(RegAcc &acc) : RegisterWOFieldAccessorBase<RegAcc, low, high, Type, internal_shift>(acc) {
    }
};

// unusual accessors

template<typename RegAcc, uint8_t low>
class RegisterRW1CFieldAccessor : public RegisterFieldAccessorBase<RegAcc, low, low, bool, 0> {
  public:
    __attribute__((always_inline)) RegisterRW1CFieldAccessor(RegAcc &acc) : RegisterFieldAccessorBase<RegAcc, low, low, bool, 0>(acc) {
        acc.write_value&= ~RegisterFieldAccessorBase<RegAcc, low, low, bool>::mask;
    }

    __attribute__((always_inline)) bool Get() {
        return RegisterFieldAccessorBase<RegAcc, low, low, bool>::Get();
    }
    
    __attribute__((always_inline)) operator bool() {
        return RegisterFieldAccessorBase<RegAcc, low, low, bool>::Get();
    }

    __attribute__((always_inline)) RegAcc &Clear() {
        return RegisterFieldAccessorBase<RegAcc, low, low, bool>::Set(true);
    }
};

// templated-specialized accessors

template<typename RegAcc, uint8_t low>
class RegisterRWFieldAccessor<RegAcc, low, low> : public RegisterRWFieldAccessorBase<RegAcc, low, low, bool> {
  public:
    __attribute__((always_inline)) RegisterRWFieldAccessor(RegAcc &acc) : RegisterRWFieldAccessorBase<RegAcc, low, low, bool>(acc) { 
    }

    __attribute__((always_inline)) RegAcc &Disable() {
        return RegisterFieldAccessorBase<RegAcc, low, low, bool>::Set(false);
    }

    __attribute__((always_inline)) RegAcc &Enable() {
        return RegisterFieldAccessorBase<RegAcc, low, low, bool>::Set(true);
    }

    __attribute__((always_inline)) operator bool() {
        return RegisterFieldAccessorBase<RegAcc, low, low, bool>::Get();
    }
};

template<typename RegAcc, uint8_t low>
class RegisterROFieldAccessor<RegAcc, low, low> : public RegisterROFieldAccessorBase<RegAcc, low, low, bool> {
  public:
    __attribute__((always_inline)) RegisterROFieldAccessor(RegAcc &acc) : RegisterROFieldAccessorBase<RegAcc, low, low, bool>(acc) { 
    }

    __attribute__((always_inline)) operator bool() {
        return RegisterROFieldAccessorBase<RegAcc, low, low, bool>::Get();
    }
};

#define BEGIN_DEFINE_REGISTER(name, type, offset) \
    class name##_Accessor : public RegisterAccessor<Peripheral, type, offset> { \
      public: \
        using RegSelf = name##_Accessor; \
        __attribute__((always_inline)) name##_Accessor(const Peripheral &p) : RegisterAccessor(p) {} \
        __attribute__((always_inline)) name##_Accessor(const Peripheral &p, type value) : RegisterAccessor(p, value) {}

#define BEGIN_DEFINE_WO_REGISTER(name, type, offset) \
    class name##_Accessor : public RegisterAccessor<Peripheral, type, offset> { \
      public: \
        using RegSelf = name##_Accessor; \
        __attribute__((always_inline)) name##_Accessor(const Peripheral &p) : RegisterAccessor(p, 0) {}

#define DEFINE_RW_FIELD(name, ...) \
    RegisterRWFieldAccessor<RegSelf, __VA_ARGS__> name = RegisterRWFieldAccessor<RegSelf, __VA_ARGS__>(*this);

#define DEFINE_RO_FIELD(name, ...) \
    RegisterROFieldAccessor<RegSelf, __VA_ARGS__> name = RegisterROFieldAccessor<RegSelf, __VA_ARGS__>(*this);

#define DEFINE_WO_FIELD(name, ...) \
    RegisterWOFieldAccessor<RegSelf, __VA_ARGS__> name = RegisterWOFieldAccessor<RegSelf, __VA_ARGS__>(*this);

#define DEFINE_RW1C_FIELD(name, ...) \
    RegisterRW1CFieldAccessor<RegSelf, __VA_ARGS__> name = RegisterRW1CFieldAccessor<RegSelf, __VA_ARGS__>(*this);


#define BEGIN_DEFINE_RO_SYMBOLIC_FIELD(name, low, high) \
    struct name##_Accessor : public RegisterROFieldAccessor<RegSelf, low, high, RegSelf::Base> {\
        using SymSelf = name##_Accessor; \
        __attribute__((always_inline)) name##_Accessor(RegSelf &acc) : RegisterROFieldAccessor(acc) { }

#define BEGIN_DEFINE_RW_SYMBOLIC_FIELD(name, low, high) \
    struct name##_Accessor : public RegisterRWFieldAccessor<RegSelf, low, high, RegSelf::Base> {\
        using SymSelf = name##_Accessor; \
        __attribute__((always_inline)) name##_Accessor(RegSelf &acc) : RegisterRWFieldAccessor(acc) { }

#define DEFINE_RO_SYMBOLIC_VALUE(name, value) \
    __attribute__((always_inline)) bool Is##name() { \
        return Get() == typename SymSelf::FieldType(value); \
    }

#define DEFINE_RW_SYMBOLIC_VALUE(name, value) \
    __attribute__((always_inline)) RegisterAccessor &Set##name() {\
        return Set(typename SymSelf::FieldType(value)); \
    } \
    __attribute__((always_inline)) bool Is##name() { \
        return Get() == typename SymSelf::FieldType(value); \
    }

#define END_DEFINE_SYMBOLIC_FIELD(name) \
    } name = name##_Accessor(*this);

#define END_DEFINE_REGISTER(name) \
    }; \
    __attribute__((always_inline)) name##_Accessor name() const { \
        return name##_Accessor(*this); \
    } \
    __attribute__((always_inline)) name##_Accessor name(name##_Accessor::Base value) const { \
        return name##_Accessor(*this, value); \
    }

#define END_DEFINE_WO_REGISTER(name) \
    }; \
    __attribute__((always_inline)) name##_Accessor name() const { \
        return name##_Accessor(*this); \
    }

#define DEFINE_BIT_ARRAY_REGISTER(name, type, offset) \
    __attribute__((always_inline)) BitArrayRegisterAccessor<Peripheral, type> name() const { \
        return BitArrayRegisterAccessor<Peripheral, type>(*this, offset); \
    } \
    __attribute__((always_inline)) BitArrayRegisterAccessor<Peripheral, type> name(type value) const { \
        return BitArrayRegisterAccessor<Peripheral, type>(*this, offset, value); \
    }

#define DEFINE_RW1C_BIT_ARRAY_REGISTER(name, type, offset) \
    __attribute__((always_inline)) BitArrayRegisterAccessor<Peripheral, type> name() const { \
        return BitArrayRegisterAccessor<Peripheral, type>(*this, offset, rw1c_marker {}); \
    }

#if (0)
/*
  This can look a little obtuse, so here's an example of how it's meant to be used:
 */

namespace t210 {

const struct T_XUSB_DEV_XHCI {
    /* This field needs to have this name and this type, since it is used by the *_DEFINE_REGISTER macros. */
    /* However, it is allowed to be non-static, in case you have multiple instances of the same kind of peripheral. */
    static const uintptr_t base_addr = 0x7000d000;

    /* This using needs to have this name, since it is used by the macros. */
    using Peripheral = T_XUSB_DEV_XHCI;

    /* You can use your own types with register fields, as long as they can be casted to and from integer types. */
    enum class LinkSpeed {
        /* ... */
    };

    /* BEGIN_DEFINE_REGISTER(name, base type, offset from periphal base)
       Defines a register. */
    BEGIN_DEFINE_REGISTER(EREPLO, uint32_t, 0x28)
        /* DEFINE_RW_FIELD(name, lo, [hi], [type], [internal shift])
           Internal shift is used for fields that store values without low bits.
           Here, we want to be able to access a pointer directly, but the low 4 bits
           belong to a different field, so the internal shift automatically shifts
           reads and writes for us. */
        DEFINE_RW_FIELD(ADDRLO, 4, 31, uintptr_t, 4)
        DEFINE_RW_FIELD(SEGI, 1)
        DEFINE_RW_FIELD(ECS, 0)
    END_DEFINE_REGISTER(EREPLO)

    BEGIN_DEFINE_REGISTER(PORTSC, uint32_t, 0x3c)
        /* DEFINE_RO_FIELD(name, lo, [hi], [base type], [internal shift])
           Same as DEFINE_RW_FIELD except it creates a different accessor type
           that does not have methods for writing. */
        DEFINE_RO_FIELD(WPR, 30)
        DEFINE_RO_FIELD(RSVD4, 28, 29)
        
        /* DEFINE_RW1C_FIELD(name, lo)
           Defines a bit that is cleared when 1 is written to it, so we
           need to be sure to not write it back if we don't mean to. Will not
           write back unless you call .Clear() on it.
           
           Forced to be a single-bit boolean type. */
        DEFINE_RW1C_FIELD(CEC, 23)
        DEFINE_RW1C_FIELD(PLC, 22)
        DEFINE_RW1C_FIELD(PRC, 21)
        DEFINE_RO_FIELD(RSVD3, 20)
        DEFINE_RW1C_FIELD(WRC, 19)
        DEFINE_RW1C_FIELD(CSC, 17)
        DEFINE_RW_FIELD(LWS, 16)
        DEFINE_RO_FIELD(RSVD2, 15)
        
        /* BEGIN_DEFINE_RO_SYMBOLIC_FIELD(name, lo, hi)
           Defines an enumerated field without writer accessor methods. */
        BEGIN_DEFINE_RO_SYMBOLIC_FIELD(PS, 10, 13)
            /* Defines an enumerated value without writer accessor methods. */
            DEFINE_RO_SYMBOLIC_VALUE(UNDEFINED, 0)
            DEFINE_RO_SYMBOLIC_VALUE(FS, 1)
            DEFINE_RO_SYMBOLIC_VALUE(LS, 2)
            DEFINE_RO_SYMBOLIC_VALUE(HS, 3)
            DEFINE_RO_SYMBOLIC_VALUE(SS, 4)
        END_DEFINE_SYMBOLIC_FIELD(PS)
        
        DEFINE_RO_FIELD(LANE_POLARITY_VALUE, 9)
        
        BEGIN_DEFINE_RW_SYMBOLIC_FIELD(PLS, 5, 8)
            DEFINE_RW_SYMBOLIC_VALUE(U0, 0)
            DEFINE_RW_SYMBOLIC_VALUE(U1, 1)
            DEFINE_RW_SYMBOLIC_VALUE(U2, 2)
            DEFINE_RW_SYMBOLIC_VALUE(U3, 3)
            DEFINE_RW_SYMBOLIC_VALUE(DISABLED, 4)
            DEFINE_RW_SYMBOLIC_VALUE(RXDETECT, 5)
            DEFINE_RW_SYMBOLIC_VALUE(INACTIVE, 6)
            DEFINE_RW_SYMBOLIC_VALUE(POLLING, 7)
            DEFINE_RW_SYMBOLIC_VALUE(RECOVERY, 8)
            DEFINE_RW_SYMBOLIC_VALUE(HOTRESET, 9)
            DEFINE_RW_SYMBOLIC_VALUE(COMPLIANCE, 0xa)
            DEFINE_RW_SYMBOLIC_VALUE(LOOPBACK, 0xb)
            DEFINE_RW_SYMBOLIC_VALUE(RESUME, 0xf)
        END_DEFINE_SYMBOLIC_FIELD(PLS)
        
        DEFINE_RO_FIELD(PR, 4)
        DEFINE_RW_FIELD(LANE_POLARITY_OVRD_VALUE, 3)
        DEFINE_RW_FIELD(LANE_POLARITY_OVRD, 2)
        DEFINE_RW_FIELD(PED, 1)
        DEFINE_RW_FIELD(CCS, 0)
    END_DEFINE_REGISTER(PORTSC)
} T_XUSB_DEV_XHCI;

void chaining_example() {
    t210::CLK_RST_CONTROLLER
        .CLK_OUT_ENB_W_0()
            .CLK_ENB_XUSB().Enable()
            .Commit()
        .RST_DEVICES_W_0()
            .SWR_XUSB_SS_RST().Set(0)
            .Commit();
    
    t210::XUSB_PADCTL
        .USB2_PAD_MUX_0()
            .USB2_OTG_PAD_PORT0().SetXUSB()
            .USB2_BIAS_PAD().SetXUSB()
            .Commit();
}

    void rw1c_example() {
    auto portsc = t210::XUSB_DEV.PORTSC();

    if (portsc.CEC) {
        portsc.CEC.Clear();
    }

    portsc.PED.Set(true);

    portsc.Commit();
}

uintptr_t internal_shift_example() {
    t210::XUSB_DEV
        .EREPLO()
            .ADDRLO.Set((uintptr_t) &internal_shift_example)
            .Commit();

    return t210::XUSB_DEV.EREPLO().ADDRLO.Get();
}

void no_read_example() {
    t210::XUSB_DEV
        .EREPLO(0) // does not read from register- instead assumes it contains 0
            .ADDRLO.Set((uintptr_t) &no_read_example)
            .Commit();
}
    
#endif
