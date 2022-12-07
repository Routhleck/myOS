#include <arch/x86/interrupts.h>
#include <arch/x86/tss.h>
#include <hal/apic.h>
#include <hal/cpu.h>
#include <lunaix/mm/page.h>
#include <lunaix/mm/vmm.h>
#include <lunaix/process.h>
#include <lunaix/sched.h>
#include <lunaix/syslog.h>
#include <lunaix/tty/tty.h>

LOG_MODULE("intr")

static int_subscriber subscribers[256];

static int_subscriber fallback = (int_subscriber)0;

void
intr_subscribe(const uint8_t vector, int_subscriber subscriber)
{
    subscribers[vector] = subscriber;
}

void
intr_unsubscribe(const uint8_t vector, int_subscriber subscriber)
{
    if (subscribers[vector] == subscriber) {
        subscribers[vector] = (int_subscriber)0;
    }
}

void
intr_set_fallback_handler(int_subscriber subscribers)
{
    fallback = subscribers;
}

extern x86_page_table* __kernel_ptd;

void
intr_handler(isr_param* param)
{
    __current->intr_ctx = *param;

    isr_param* lparam = &__current->intr_ctx;

    if (lparam->vector <= 255) {
        int_subscriber subscriber = subscribers[lparam->vector];
        if (subscriber) {
            subscriber(param);
            goto done;
        }
    }

    if (fallback) {
        fallback(lparam);
        goto done;
    }

    kprint_panic("INT %u: (%x) [%p: %p] Unknown",
                 lparam->vector,
                 lparam->err_code,
                 lparam->cs,
                 lparam->eip);

done:
    // for all external interrupts except the spurious interrupt
    //  this is required by Intel Manual Vol.3A, section 10.8.1 & 10.8.5
    if (lparam->vector >= EX_INTERRUPT_BEGIN &&
        lparam->vector != APIC_SPIV_IV) {
        apic_done_servicing();
    }

    return;
}