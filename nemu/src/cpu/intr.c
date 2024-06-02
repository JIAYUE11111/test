#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  // while(cpu.eflags.IF == 0);
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  // STEP1: Save status (EFLAGS, CS, EIP)
  //zip eflags into uint32
  cpu.eflags.IF = 0;  //close intervene for clock
  memcpy(&t0, &cpu.eflags, sizeof(cpu.eflags));
  rtl_push(&t0);


  rtl_push(&cpu.cs);
  rtl_li(&t0, ret_addr);
  rtl_push(&t0);
  // Log("%x\n", cpu.idtr.base);

  // STEP2: Get IDT index
  vaddr_t addr = cpu.idtr.base + NO * 8;

  // STEP3: Get offset from gate descriptor
  uint16_t high = vaddr_read(addr + 6, 2);
  uint16_t low = vaddr_read(addr, 2);
  decoding.jmp_eip = ((uint32_t)high<<16) + low;
  decoding.is_jmp = 1;
}

void dev_raise_intr() {
  cpu.INTR = true;
}
