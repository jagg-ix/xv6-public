6700 // Intel 8259A programmable interrupt controllers.
6701 
6702 #include "types.h"
6703 #include "x86.h"
6704 #include "traps.h"
6705 
6706 // I/O Addresses of the two programmable interrupt controllers
6707 #define IO_PIC1         0x20    // Master (IRQs 0-7)
6708 #define IO_PIC2         0xA0    // Slave (IRQs 8-15)
6709 
6710 #define IRQ_SLAVE       2       // IRQ at which slave connects to master
6711 
6712 // Current IRQ mask.
6713 // Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
6714 static ushort irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);
6715 
6716 static void
6717 picsetmask(ushort mask)
6718 {
6719   irqmask = mask;
6720   outb(IO_PIC1+1, mask);
6721   outb(IO_PIC2+1, mask >> 8);
6722 }
6723 
6724 void
6725 picenable(int irq)
6726 {
6727   picsetmask(irqmask & ~(1<<irq));
6728 }
6729 
6730 // Initialize the 8259A interrupt controllers.
6731 void
6732 picinit(void)
6733 {
6734   // mask all interrupts
6735   outb(IO_PIC1+1, 0xFF);
6736   outb(IO_PIC2+1, 0xFF);
6737 
6738   // Set up master (8259A-1)
6739 
6740   // ICW1:  0001g0hi
6741   //    g:  0 = edge triggering, 1 = level triggering
6742   //    h:  0 = cascaded PICs, 1 = master only
6743   //    i:  0 = no ICW4, 1 = ICW4 required
6744   outb(IO_PIC1, 0x11);
6745 
6746   // ICW2:  Vector offset
6747   outb(IO_PIC1+1, T_IRQ0);
6748 
6749 
6750   // ICW3:  (master PIC) bit mask of IR lines connected to slaves
6751   //        (slave PIC) 3-bit # of slave's connection to master
6752   outb(IO_PIC1+1, 1<<IRQ_SLAVE);
6753 
6754   // ICW4:  000nbmap
6755   //    n:  1 = special fully nested mode
6756   //    b:  1 = buffered mode
6757   //    m:  0 = slave PIC, 1 = master PIC
6758   //      (ignored when b is 0, as the master/slave role
6759   //      can be hardwired).
6760   //    a:  1 = Automatic EOI mode
6761   //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
6762   outb(IO_PIC1+1, 0x3);
6763 
6764   // Set up slave (8259A-2)
6765   outb(IO_PIC2, 0x11);                  // ICW1
6766   outb(IO_PIC2+1, T_IRQ0 + 8);      // ICW2
6767   outb(IO_PIC2+1, IRQ_SLAVE);           // ICW3
6768   // NB Automatic EOI mode doesn't tend to work on the slave.
6769   // Linux source code says it's "to be investigated".
6770   outb(IO_PIC2+1, 0x3);                 // ICW4
6771 
6772   // OCW3:  0ef01prs
6773   //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
6774   //    p:  0 = no polling, 1 = polling mode
6775   //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
6776   outb(IO_PIC1, 0x68);             // clear specific mask
6777   outb(IO_PIC1, 0x0a);             // read IRR by default
6778 
6779   outb(IO_PIC2, 0x68);             // OCW3
6780   outb(IO_PIC2, 0x0a);             // OCW3
6781 
6782   if(irqmask != 0xFFFF)
6783     picsetmask(irqmask);
6784 }
6785 
6786 
6787 
6788 
6789 
6790 
6791 
6792 
6793 
6794 
6795 
6796 
6797 
6798 
6799 
