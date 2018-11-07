#pragma once

#include <mutex>

namespace mesosphere
{
inline namespace arch
{
inline namespace arm64
{

// This largely uses the Linux kernel spinlock code, which is more efficient than Nintendo's (serializing two u16s into an u32).
class KSpinLock {

    private:

    struct alignas(4) Ticket {
        u16 owner, next;
    };

    Ticket ticket;

    public:

    bool try_lock()
    {
        u32 tmp;
        Ticket lockval;

        asm volatile(
            "   prfm    pstl1strm, %2\n"
            "1: ldaxr   %w0, %2\n"
            "   eor     %w1, %w0, %w0, ror #16\n"
            "   cbnz    %w1, 2f\n"
            "   add     %w0, %w0, %3\n"
            "   stxr    %w1, %w0, %2\n"
            "   cbnz    %w1, 1b\n"
            "2:"
            : "=&r" (lockval), "=&r" (tmp), "+Q" (ticket)
            : "I" (1 << 16)
            : "memory"
        );

        return !tmp;
    }

    void lock()
    {
        u32 tmp;
        Ticket lockval, newval;

        asm volatile(
            // Atomically increment the next ticket.
        "   prfm    pstl1strm, %3\n"
        "1: ldaxr   %w0, %3\n"
        "   add     %w1, %w0, %w5\n"
        "   stxr    %w2, %w1, %3\n"
        "   cbnz    %w2, 1b\n"


            // Did we get the lock?
        "   eor %w1, %w0, %w0, ror #16\n"
        "   cbz %w1, 3f\n"
            /*
                No: spin on the owner. Send a local event to avoid missing an
                unlock before the exclusive load.
            */
        "   sevl\n"
        "2: wfe\n"
        "   ldaxrh  %w2, %4\n"
        "   eor     %w1, %w2, %w0, lsr #16\n"
        "   cbnz    %w1, 2b\n"
            // We got the lock. Critical section starts here.
        "3:"
            : "=&r" (lockval), "=&r" (newval), "=&r" (tmp), "+Q" (*&ticket)
            : "Q" (ticket.owner), "I" (1 << 16)
            : "memory"
        );
    }

    void unlock()
    {
        u64 tmp;
        asm volatile(
            "   ldrh    %w1, %0\n"
            "   add     %w1, %w1, #1\n"
            "   stlrh   %w1, %0"
            : "=Q" (ticket.owner), "=&r" (tmp)
            :
            : "memory"
        );

    }
    KSpinLock() = default;
    KSpinLock(const KSpinLock &) = delete;
    KSpinLock(KSpinLock &&) = delete;
    KSpinLock &operator=(const KSpinLock &) = delete;
    KSpinLock &operator=(KSpinLock &&) = delete;
};

}
}
}
