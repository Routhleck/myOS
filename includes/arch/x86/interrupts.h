#ifndef __ROUTHLECK_INTERRUPTS_H
#define __ROUTHLECK_INTERRUPTS_H

typedef struct {
    unsigned int vector;
    unsigned int err_code;
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
} __attribute__((packed)) isr_param;

void
_asm_isr0();

void
interrupt_handler(isr_param* param);


#endif /* __LUNAIX_INTERRUPTS_H */
