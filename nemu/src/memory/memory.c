#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

#define PTXSHFT   12      // Offset of PTX in a linear address
#define PDXSHFT   22      // Offset of PDX in a linear address
#define PDX(va)     (((uint32_t)(va) >> PDXSHFT) & 0x3ff)
#define PTX(va)     (((uint32_t)(va) >> PTXSHFT) & 0x3ff)
#define OFF(va)     ((uint32_t)(va) & 0xfff)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uint32_t)((d) << PDXSHFT | (t) << PTXSHFT | (o)))

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO = is_mmio(addr);
  if(map_NO!=-1){
    return mmio_read(addr, len, map_NO);
  }
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int map_NO = is_mmio(addr);
  if(map_NO!=-1){
    mmio_write(addr, len, data, map_NO);
  }
  else{
    memcpy(guest_to_host(addr), &data, len);
  }
}

paddr_t page_translate(vaddr_t vaddr, bool dirty){
  CR0 cr0 = (CR0)cpu.cr0;
  if(!(cr0.protect_enable && cr0.paging)){
    return vaddr;
  }
  CR3 cr3 = (CR3)cpu.cr3;
  PDE* pde_begin = (PDE*)PTE_ADDR(cr3.val);
  PDE pde = (PDE)paddr_read((paddr_t)(pde_begin + PDX(vaddr)), 4);
  assert(pde.present);
  pde.accessed = 1;

  PTE* pte_begin = (PTE*)PTE_ADDR(pde.val);
  PTE pte = (PTE)paddr_read((paddr_t)(pte_begin + PTX(vaddr)), 4);
  assert(pte.present);
  pte.accessed = 1;
  pte.dirty = dirty;

  return PTE_ADDR(pte.val) | OFF(vaddr);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  vaddr_t end_addr = addr + len - 1;
  if(PTX(addr)!=PTX(end_addr) || PDX(addr)!=PDX(end_addr)){
  // if(PDX(addr)!=PDX(end_addr)){
    // printf("address overflowed at %x\tlen:%d\n", addr, len);
    // assert(0);
    int len1 = 4096 - OFF(addr);
    paddr_t paddr1 = page_translate(addr, false);
    uint32_t low = paddr_read(paddr1, len1);

    int len2 = OFF(end_addr + 1);
    paddr_t paddr2 = page_translate(addr + len1, false);
    uint32_t high = paddr_read(paddr2, len2);

    return high << (len1*8) | low;
  }
  else{
    paddr_t paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  vaddr_t end_addr = addr + len - 1;
  if(PTX(addr)!=PTX(end_addr) || PDX(addr)!=PDX(end_addr)){
  // if(PDX(addr)!=PDX(end_addr)){
    // printf("address overflowed at %x\tlen:%d\n", addr, len);
    // assert(0);
    int len1 = 4096 - OFF(addr);
    paddr_t paddr1 = page_translate(addr, true);
    uint32_t data1 = ((1 << (((4-len1)<<3) + 1)) - 1) & data;
    paddr_write(paddr1, len1, data1);

    int len2 = OFF(end_addr + 1);
    paddr_t paddr2 = page_translate(addr + len1, true);
    uint32_t data2 = data >> ((4-len1)<<3);
    paddr_write(paddr2, len2, data2);
  }
  else{
    paddr_t paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
  }
}
