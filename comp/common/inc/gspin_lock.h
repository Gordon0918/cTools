/*
spin lock geo media co.Ltd.
mickey add
*/

#ifndef __GSPIN_LOCK_H__
#define __GSPIN_LOCK_H__
#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>

#define __WEAK_LLSC_MB        "       sync    \n"

#define smp_llsc_mb()    __asm__ __volatile__(__WEAK_LLSC_MB : : :"memory")

#define __sync()    __asm__ __volatile__(            \
                    ".set    push\n\t"        \
                    ".set    noreorder\n\t"        \
                    ".set    mips2\n\t"        \
                    "sync\n\t"            \
                    ".set    pop"            \
                    : : : "memory")



typedef union {
    /*
     * bits  0..15 : serving_now
     * bits 16..31 : ticket
     */
    uint32_t lock;
    struct {
        uint16_t ticket;
        uint16_t serving_now;
    } h;
} gsmp_spinlock_t;

static inline void gsmp_spin_lock(gsmp_spinlock_t *lock)
{

    int my_ticket;
    int tmp;
    int inc = 0x10000;

    __asm__ __volatile__ (
    "    .set push        # gsmp_spin_lock    \n"
    "    .set noreorder                    \n"
    "                            \n"
    "1:    ll    %[ticket], %[ticket_ptr]        \n"
    "    addu    %[my_ticket], %[ticket], %[inc]        \n"
    "    sc    %[my_ticket], %[ticket_ptr]        \n"
    "    beqz    %[my_ticket], 1b            \n"
    "     srl    %[my_ticket], %[ticket], 16        \n"
    "    andi    %[ticket], %[ticket], 0xffff        \n"
    "    andi    %[my_ticket], %[my_ticket], 0xffff    \n"
    "    bne    %[ticket], %[my_ticket], 4f        \n"
    "     subu    %[ticket], %[my_ticket], %[ticket]    \n"
    "2:                            \n"
    "    .subsection 2                    \n"
    "4:    andi    %[ticket], %[ticket], 0x1fff        \n"
    "    sll    %[ticket], 5                \n"
    "                            \n"
    "6:    bnez    %[ticket], 6b                \n"
    "     subu    %[ticket], 1                \n"
    "                            \n"
    "    lhu    %[ticket], %[serving_now_ptr]        \n"
    "    beq    %[ticket], %[my_ticket], 2b        \n"
    "     subu    %[ticket], %[my_ticket], %[ticket]    \n"
    "    b    4b                    \n"
    "     subu    %[ticket], %[ticket], 1            \n"
    "    .previous                    \n"
    "    .set pop                    \n"
    : [ticket_ptr] "+m" (lock->lock),
      [serving_now_ptr] "+m" (lock->h.serving_now),
      [ticket] "=&r" (tmp),
      [my_ticket] "=&r" (my_ticket)
    : [inc] "r" (inc));

    smp_llsc_mb();
}



static inline void gsmp_spin_unlock(gsmp_spinlock_t *lock){

    unsigned int serving_now = lock->h.serving_now + 1;

    __sync();
    
    lock->h.serving_now = (uint16_t)serving_now;
    
    __sync();    
}


static inline void gsmp_spin_lock_init(gsmp_spinlock_t *lock)
{
    lock->lock = 0;
}

#ifdef __cplusplus
}
#endif

#endif /*__GSPIN_LOCK_H__*/


