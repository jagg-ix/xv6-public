8250 // Boot loader.
8251 //
8252 // Part of the boot block, along with bootasm.S, which calls bootmain().
8253 // bootasm.S has put the processor into protected 32-bit mode.
8254 // bootmain() loads an ELF kernel image from the disk starting at
8255 // sector 1 and then jumps to the kernel entry routine.
8256 
8257 #include "types.h"
8258 #include "elf.h"
8259 #include "x86.h"
8260 #include "memlayout.h"
8261 
8262 #define SECTSIZE  512
8263 
8264 void readseg(uchar*, uint, uint);
8265 
8266 void
8267 bootmain(void)
8268 {
8269   struct elfhdr *elf;
8270   struct proghdr *ph, *eph;
8271   void (*entry)(void);
8272   uchar* pa;
8273 
8274   elf = (struct elfhdr*)0x10000;  // scratch space
8275 
8276   // Read 1st page off disk
8277   readseg((uchar*)elf, 4096, 0);
8278 
8279   // Is this an ELF executable?
8280   if(elf->magic != ELF_MAGIC)
8281     return;  // let bootasm.S handle error
8282 
8283   // Load each program segment (ignores ph flags).
8284   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
8285   eph = ph + elf->phnum;
8286   for(; ph < eph; ph++){
8287     pa = (uchar*)ph->paddr;
8288     readseg(pa, ph->filesz, ph->off);
8289     if(ph->memsz > ph->filesz)
8290       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
8291   }
8292 
8293   // Call the entry point from the ELF header.
8294   // Does not return!
8295   entry = (void(*)(void))(elf->entry);
8296   entry();
8297 }
8298 
8299 
8300 void
8301 waitdisk(void)
8302 {
8303   // Wait for disk ready.
8304   while((inb(0x1F7) & 0xC0) != 0x40)
8305     ;
8306 }
8307 
8308 // Read a single sector at offset into dst.
8309 void
8310 readsect(void *dst, uint offset)
8311 {
8312   // Issue command.
8313   waitdisk();
8314   outb(0x1F2, 1);   // count = 1
8315   outb(0x1F3, offset);
8316   outb(0x1F4, offset >> 8);
8317   outb(0x1F5, offset >> 16);
8318   outb(0x1F6, (offset >> 24) | 0xE0);
8319   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
8320 
8321   // Read data.
8322   waitdisk();
8323   insl(0x1F0, dst, SECTSIZE/4);
8324 }
8325 
8326 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
8327 // Might copy more than asked.
8328 void
8329 readseg(uchar* pa, uint count, uint offset)
8330 {
8331   uchar* epa;
8332 
8333   epa = pa + count;
8334 
8335   // Round down to sector boundary.
8336   pa -= offset % SECTSIZE;
8337 
8338   // Translate from bytes to sectors; kernel starts at sector 1.
8339   offset = (offset / SECTSIZE) + 1;
8340 
8341   // If this is too slow, we could read lots of sectors at a time.
8342   // We'd write more to memory than asked, but it doesn't matter --
8343   // we load in increasing order.
8344   for(; pa < epa; pa += SECTSIZE, offset++)
8345     readsect(pa, offset);
8346 }
8347 
8348 
8349 
