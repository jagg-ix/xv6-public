7350 // Intel 8250 serial port (UART).
7351 
7352 #include "types.h"
7353 #include "defs.h"
7354 #include "param.h"
7355 #include "traps.h"
7356 #include "spinlock.h"
7357 #include "fs.h"
7358 #include "file.h"
7359 #include "mmu.h"
7360 #include "proc.h"
7361 #include "x86.h"
7362 
7363 #define COM1    0x3f8
7364 
7365 static int uart;    // is there a uart?
7366 
7367 void
7368 uartinit(void)
7369 {
7370   char *p;
7371 
7372   // Turn off the FIFO
7373   outb(COM1+2, 0);
7374 
7375   // 9600 baud, 8 data bits, 1 stop bit, parity off.
7376   outb(COM1+3, 0x80);    // Unlock divisor
7377   outb(COM1+0, 115200/9600);
7378   outb(COM1+1, 0);
7379   outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
7380   outb(COM1+4, 0);
7381   outb(COM1+1, 0x01);    // Enable receive interrupts.
7382 
7383   // If status is 0xFF, no serial port.
7384   if(inb(COM1+5) == 0xFF)
7385     return;
7386   uart = 1;
7387 
7388   // Acknowledge pre-existing interrupt conditions;
7389   // enable interrupts.
7390   inb(COM1+2);
7391   inb(COM1+0);
7392   picenable(IRQ_COM1);
7393   ioapicenable(IRQ_COM1, 0);
7394 
7395   // Announce that we're here.
7396   for(p="xv6...\n"; *p; p++)
7397     uartputc(*p);
7398 }
7399 
7400 void
7401 uartputc(int c)
7402 {
7403   int i;
7404 
7405   if(!uart)
7406     return;
7407   for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
7408     microdelay(10);
7409   outb(COM1+0, c);
7410 }
7411 
7412 static int
7413 uartgetc(void)
7414 {
7415   if(!uart)
7416     return -1;
7417   if(!(inb(COM1+5) & 0x01))
7418     return -1;
7419   return inb(COM1+0);
7420 }
7421 
7422 void
7423 uartintr(void)
7424 {
7425   consoleintr(uartgetc);
7426 }
7427 
7428 
7429 
7430 
7431 
7432 
7433 
7434 
7435 
7436 
7437 
7438 
7439 
7440 
7441 
7442 
7443 
7444 
7445 
7446 
7447 
7448 
7449 
