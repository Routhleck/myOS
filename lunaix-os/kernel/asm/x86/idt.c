#include <arch/x86/idt.h>
#include <arch/x86/interrupts.h>
#include <stdint.h>

#define IDT_ENTRY 256

uint64_t _idt[IDT_ENTRY];
uint16_t _idt_limit = sizeof(_idt) - 1;

static inline void
_set_idt_entry(uint32_t vector,
               uint16_t seg_selector,
               void (*isr)(),
               uint8_t dpl,
               uint8_t type)
{
    uintptr_t offset = (uintptr_t)isr;
    _idt[vector] = (offset & 0xffff0000) | IDT_ATTR(dpl, type);
    _idt[vector] <<= 32;
    _idt[vector] |= (seg_selector << 16) | (offset & 0x0000ffff);
}

void
_set_idt_intr_entry(uint32_t vector,
                    uint16_t seg_selector,
                    void (*isr)(),
                    uint8_t dpl)
{
    _set_idt_entry(vector, seg_selector, isr, dpl, IDT_INTERRUPT);
}

void
_set_idt_trap_entry(uint32_t vector,
                    uint16_t seg_selector,
                    void (*isr)(),
                    uint8_t dpl)
{
    _set_idt_entry(vector, seg_selector, isr, dpl, IDT_TRAP);
}

void
_init_idt()
{
    // CPU defined interrupts
    _set_idt_intr_entry(FAULT_DIVISION_ERROR, 0x08, _asm_isr0, 0);
    _set_idt_intr_entry(FAULT_GENERAL_PROTECTION, 0x08, _asm_isr13, 0);
    _set_idt_intr_entry(FAULT_PAGE_FAULT, 0x08, _asm_isr14, 0);
    _set_idt_intr_entry(FAULT_STACK_SEG_FAULT, 0x08, _asm_isr12, 0);

    _set_idt_intr_entry(APIC_ERROR_IV, 0x08, _asm_isr250, 0);
    _set_idt_intr_entry(APIC_LINT0_IV, 0x08, _asm_isr251, 0);
    _set_idt_intr_entry(APIC_SPIV_IV, 0x08, _asm_isr252, 0);
    _set_idt_intr_entry(APIC_TIMER_IV, 0x08, _asm_isr253, 0);
    _set_idt_intr_entry(PC_KBD_IV, 0x08, _asm_isr201, 0);

    _set_idt_intr_entry(RTC_TIMER_IV, 0x08, _asm_isr210, 0);

    // system defined interrupts
    _set_idt_intr_entry(LUNAIX_SYS_PANIC, 0x08, _asm_isr32, 0);

    // We make this a non-trap entry, and enable interrupt
    // only when needed!
    _set_idt_intr_entry(LUNAIX_SYS_CALL, 0x08, _asm_isr33, 3);
}