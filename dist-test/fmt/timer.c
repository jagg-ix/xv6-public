7300 // Intel 8253/8254/82C54 Programmable Interval Timer (PIT).
7301 // Only used on uniprocessors;
7302 // SMP machines use the local APIC timer.
7303 
7304 #include "types.h"
7305 #include "defs.h"
7306 #include "traps.h"
7307 #include "x86.h"
7308 
7309 #define IO_TIMER1       0x040           // 8253 Timer #1
7310 
7311 // Frequency of all three count-down timers;
7312 // (TIMER_FREQ/freq) is the appropriate count
7313 // to generate a frequency of freq Hz.
7314 
7315 #define TIMER_FREQ      1193182
7316 #define TIMER_DIV(x)    ((TIMER_FREQ+(x)/2)/(x))
7317 
7318 #define TIMER_MODE      (IO_TIMER1 + 3) // timer mode port
7319 #define TIMER_SEL0      0x00    // select counter 0
7320 #define TIMER_RATEGEN   0x04    // mode 2, rate generator
7321 #define TIMER_16BIT     0x30    // r/w counter 16 bits, LSB first
7322 
7323 void
7324 timerinit(void)
7325 {
7326   // Interrupt 100 times/sec.
7327   outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
7328   outb(IO_TIMER1, TIMER_DIV(100) % 256);
7329   outb(IO_TIMER1, TIMER_DIV(100) / 256);
7330   picenable(IRQ_TIMER);
7331 }
7332 
7333 
7334 
7335 
7336 
7337 
7338 
7339 
7340 
7341 
7342 
7343 
7344 
7345 
7346 
7347 
7348 
7349 
