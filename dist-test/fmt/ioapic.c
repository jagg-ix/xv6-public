6600 // The I/O APIC manages hardware interrupts for an SMP system.
6601 // http://www.intel.com/design/chipsets/datashts/29056601.pdf
6602 // See also picirq.c.
6603 
6604 #include "types.h"
6605 #include "defs.h"
6606 #include "traps.h"
6607 
6608 #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
6609 
6610 #define REG_ID     0x00  // Register index: ID
6611 #define REG_VER    0x01  // Register index: version
6612 #define REG_TABLE  0x10  // Redirection table base
6613 
6614 // The redirection table starts at REG_TABLE and uses
6615 // two registers to configure each interrupt.
6616 // The first (low) register in a pair contains configuration bits.
6617 // The second (high) register contains a bitmask telling which
6618 // CPUs can serve that interrupt.
6619 #define INT_DISABLED   0x00010000  // Interrupt disabled
6620 #define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
6621 #define INT_ACTIVELOW  0x00002000  // Active low (vs high)
6622 #define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
6623 
6624 volatile struct ioapic *ioapic;
6625 
6626 // IO APIC MMIO structure: write reg, then read or write data.
6627 struct ioapic {
6628   uint reg;
6629   uint pad[3];
6630   uint data;
6631 };
6632 
6633 static uint
6634 ioapicread(int reg)
6635 {
6636   ioapic->reg = reg;
6637   return ioapic->data;
6638 }
6639 
6640 static void
6641 ioapicwrite(int reg, uint data)
6642 {
6643   ioapic->reg = reg;
6644   ioapic->data = data;
6645 }
6646 
6647 
6648 
6649 
6650 void
6651 ioapicinit(void)
6652 {
6653   int i, id, maxintr;
6654 
6655   if(!ismp)
6656     return;
6657 
6658   ioapic = (volatile struct ioapic*)IOAPIC;
6659   maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
6660   id = ioapicread(REG_ID) >> 24;
6661   if(id != ioapicid)
6662     cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
6663 
6664   // Mark all interrupts edge-triggered, active high, disabled,
6665   // and not routed to any CPUs.
6666   for(i = 0; i <= maxintr; i++){
6667     ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
6668     ioapicwrite(REG_TABLE+2*i+1, 0);
6669   }
6670 }
6671 
6672 void
6673 ioapicenable(int irq, int cpunum)
6674 {
6675   if(!ismp)
6676     return;
6677 
6678   // Mark interrupt edge-triggered, active high,
6679   // enabled, and routed to the given cpunum,
6680   // which happens to be that cpu's APIC ID.
6681   ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
6682   ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
6683 }
6684 
6685 
6686 
6687 
6688 
6689 
6690 
6691 
6692 
6693 
6694 
6695 
6696 
6697 
6698 
6699 
