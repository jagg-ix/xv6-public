6350 // The local APIC manages internal (non-I/O) interrupts.
6351 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
6352 
6353 #include "types.h"
6354 #include "defs.h"
6355 #include "date.h"
6356 #include "memlayout.h"
6357 #include "traps.h"
6358 #include "mmu.h"
6359 #include "x86.h"
6360 
6361 // Local APIC registers, divided by 4 for use as uint[] indices.
6362 #define ID      (0x0020/4)   // ID
6363 #define VER     (0x0030/4)   // Version
6364 #define TPR     (0x0080/4)   // Task Priority
6365 #define EOI     (0x00B0/4)   // EOI
6366 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
6367   #define ENABLE     0x00000100   // Unit Enable
6368 #define ESR     (0x0280/4)   // Error Status
6369 #define ICRLO   (0x0300/4)   // Interrupt Command
6370   #define INIT       0x00000500   // INIT/RESET
6371   #define STARTUP    0x00000600   // Startup IPI
6372   #define DELIVS     0x00001000   // Delivery status
6373   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
6374   #define DEASSERT   0x00000000
6375   #define LEVEL      0x00008000   // Level triggered
6376   #define BCAST      0x00080000   // Send to all APICs, including self.
6377   #define BUSY       0x00001000
6378   #define FIXED      0x00000000
6379 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
6380 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
6381   #define X1         0x0000000B   // divide counts by 1
6382   #define PERIODIC   0x00020000   // Periodic
6383 #define PCINT   (0x0340/4)   // Performance Counter LVT
6384 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
6385 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
6386 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
6387   #define MASKED     0x00010000   // Interrupt masked
6388 #define TICR    (0x0380/4)   // Timer Initial Count
6389 #define TCCR    (0x0390/4)   // Timer Current Count
6390 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
6391 
6392 volatile uint *lapic;  // Initialized in mp.c
6393 
6394 static void
6395 lapicw(int index, int value)
6396 {
6397   lapic[index] = value;
6398   lapic[ID];  // wait for write to finish, by reading
6399 }
6400 void
6401 lapicinit(void)
6402 {
6403   if(!lapic)
6404     return;
6405 
6406   // Enable local APIC; set spurious interrupt vector.
6407   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
6408 
6409   // The timer repeatedly counts down at bus frequency
6410   // from lapic[TICR] and then issues an interrupt.
6411   // If xv6 cared more about precise timekeeping,
6412   // TICR would be calibrated using an external time source.
6413   lapicw(TDCR, X1);
6414   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
6415   lapicw(TICR, 10000000);
6416 
6417   // Disable logical interrupt lines.
6418   lapicw(LINT0, MASKED);
6419   lapicw(LINT1, MASKED);
6420 
6421   // Disable performance counter overflow interrupts
6422   // on machines that provide that interrupt entry.
6423   if(((lapic[VER]>>16) & 0xFF) >= 4)
6424     lapicw(PCINT, MASKED);
6425 
6426   // Map error interrupt to IRQ_ERROR.
6427   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
6428 
6429   // Clear error status register (requires back-to-back writes).
6430   lapicw(ESR, 0);
6431   lapicw(ESR, 0);
6432 
6433   // Ack any outstanding interrupts.
6434   lapicw(EOI, 0);
6435 
6436   // Send an Init Level De-Assert to synchronise arbitration ID's.
6437   lapicw(ICRHI, 0);
6438   lapicw(ICRLO, BCAST | INIT | LEVEL);
6439   while(lapic[ICRLO] & DELIVS)
6440     ;
6441 
6442   // Enable interrupts on the APIC (but not on the processor).
6443   lapicw(TPR, 0);
6444 }
6445 
6446 
6447 
6448 
6449 
6450 int
6451 cpunum(void)
6452 {
6453   // Cannot call cpu when interrupts are enabled:
6454   // result not guaranteed to last long enough to be used!
6455   // Would prefer to panic but even printing is chancy here:
6456   // almost everything, including cprintf and panic, calls cpu,
6457   // often indirectly through acquire and release.
6458   if(readeflags()&FL_IF){
6459     static int n;
6460     if(n++ == 0)
6461       cprintf("cpu called from %x with interrupts enabled\n",
6462         __builtin_return_address(0));
6463   }
6464 
6465   if(lapic)
6466     return lapic[ID]>>24;
6467   return 0;
6468 }
6469 
6470 // Acknowledge interrupt.
6471 void
6472 lapiceoi(void)
6473 {
6474   if(lapic)
6475     lapicw(EOI, 0);
6476 }
6477 
6478 // Spin for a given number of microseconds.
6479 // On real hardware would want to tune this dynamically.
6480 void
6481 microdelay(int us)
6482 {
6483 }
6484 
6485 #define CMOS_PORT    0x70
6486 #define CMOS_RETURN  0x71
6487 
6488 // Start additional processor running entry code at addr.
6489 // See Appendix B of MultiProcessor Specification.
6490 void
6491 lapicstartap(uchar apicid, uint addr)
6492 {
6493   int i;
6494   ushort *wrv;
6495 
6496   // "The BSP must initialize CMOS shutdown code to 0AH
6497   // and the warm reset vector (DWORD based at 40:67) to point at
6498   // the AP startup code prior to the [universal startup algorithm]."
6499   outb(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
6500   outb(CMOS_PORT+1, 0x0A);
6501   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
6502   wrv[0] = 0;
6503   wrv[1] = addr >> 4;
6504 
6505   // "Universal startup algorithm."
6506   // Send INIT (level-triggered) interrupt to reset other CPU.
6507   lapicw(ICRHI, apicid<<24);
6508   lapicw(ICRLO, INIT | LEVEL | ASSERT);
6509   microdelay(200);
6510   lapicw(ICRLO, INIT | LEVEL);
6511   microdelay(100);    // should be 10ms, but too slow in Bochs!
6512 
6513   // Send startup IPI (twice!) to enter code.
6514   // Regular hardware is supposed to only accept a STARTUP
6515   // when it is in the halted state due to an INIT.  So the second
6516   // should be ignored, but it is part of the official Intel algorithm.
6517   // Bochs complains about the second one.  Too bad for Bochs.
6518   for(i = 0; i < 2; i++){
6519     lapicw(ICRHI, apicid<<24);
6520     lapicw(ICRLO, STARTUP | (addr>>12));
6521     microdelay(200);
6522   }
6523 }
6524 
6525 #define CMOS_STATA   0x0a
6526 #define CMOS_STATB   0x0b
6527 #define CMOS_UIP    (1 << 7)        // RTC update in progress
6528 
6529 #define SECS    0x00
6530 #define MINS    0x02
6531 #define HOURS   0x04
6532 #define DAY     0x07
6533 #define MONTH   0x08
6534 #define YEAR    0x09
6535 
6536 static uint cmos_read(uint reg)
6537 {
6538   outb(CMOS_PORT,  reg);
6539   microdelay(200);
6540 
6541   return inb(CMOS_RETURN);
6542 }
6543 
6544 
6545 
6546 
6547 
6548 
6549 
6550 static void fill_rtcdate(struct rtcdate *r)
6551 {
6552   r->second = cmos_read(SECS);
6553   r->minute = cmos_read(MINS);
6554   r->hour   = cmos_read(HOURS);
6555   r->day    = cmos_read(DAY);
6556   r->month  = cmos_read(MONTH);
6557   r->year   = cmos_read(YEAR);
6558 }
6559 
6560 // qemu seems to use 24-hour GWT and the values are BCD encoded
6561 void cmostime(struct rtcdate *r)
6562 {
6563   struct rtcdate t1, t2;
6564   int sb, bcd;
6565 
6566   sb = cmos_read(CMOS_STATB);
6567 
6568   bcd = (sb & (1 << 2)) == 0;
6569 
6570   // make sure CMOS doesn't modify time while we read it
6571   for (;;) {
6572     fill_rtcdate(&t1);
6573     if (cmos_read(CMOS_STATA) & CMOS_UIP)
6574         continue;
6575     fill_rtcdate(&t2);
6576     if (memcmp(&t1, &t2, sizeof(t1)) == 0)
6577       break;
6578   }
6579 
6580   // convert
6581   if (bcd) {
6582 #define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
6583     CONV(second);
6584     CONV(minute);
6585     CONV(hour  );
6586     CONV(day   );
6587     CONV(month );
6588     CONV(year  );
6589 #undef     CONV
6590   }
6591 
6592   *r = t1;
6593   r->year += 2000;
6594 }
6595 
6596 
6597 
6598 
6599 
