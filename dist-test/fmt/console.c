7000 // Console input and output.
7001 // Input is from the keyboard or serial port.
7002 // Output is written to the screen and serial port.
7003 
7004 #include "types.h"
7005 #include "defs.h"
7006 #include "param.h"
7007 #include "traps.h"
7008 #include "spinlock.h"
7009 #include "fs.h"
7010 #include "file.h"
7011 #include "memlayout.h"
7012 #include "mmu.h"
7013 #include "proc.h"
7014 #include "x86.h"
7015 
7016 static void consputc(int);
7017 
7018 static int panicked = 0;
7019 
7020 static struct {
7021   struct spinlock lock;
7022   int locking;
7023 } cons;
7024 
7025 static void
7026 printint(int xx, int base, int sign)
7027 {
7028   static char digits[] = "0123456789abcdef";
7029   char buf[16];
7030   int i;
7031   uint x;
7032 
7033   if(sign && (sign = xx < 0))
7034     x = -xx;
7035   else
7036     x = xx;
7037 
7038   i = 0;
7039   do{
7040     buf[i++] = digits[x % base];
7041   }while((x /= base) != 0);
7042 
7043   if(sign)
7044     buf[i++] = '-';
7045 
7046   while(--i >= 0)
7047     consputc(buf[i]);
7048 }
7049 
7050 // Print to the console. only understands %d, %x, %p, %s.
7051 void
7052 cprintf(char *fmt, ...)
7053 {
7054   int i, c, locking;
7055   uint *argp;
7056   char *s;
7057 
7058   locking = cons.locking;
7059   if(locking)
7060     acquire(&cons.lock);
7061 
7062   if (fmt == 0)
7063     panic("null fmt");
7064 
7065   argp = (uint*)(void*)(&fmt + 1);
7066   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
7067     if(c != '%'){
7068       consputc(c);
7069       continue;
7070     }
7071     c = fmt[++i] & 0xff;
7072     if(c == 0)
7073       break;
7074     switch(c){
7075     case 'd':
7076       printint(*argp++, 10, 1);
7077       break;
7078     case 'x':
7079     case 'p':
7080       printint(*argp++, 16, 0);
7081       break;
7082     case 's':
7083       if((s = (char*)*argp++) == 0)
7084         s = "(null)";
7085       for(; *s; s++)
7086         consputc(*s);
7087       break;
7088     case '%':
7089       consputc('%');
7090       break;
7091     default:
7092       // Print unknown % sequence to draw attention.
7093       consputc('%');
7094       consputc(c);
7095       break;
7096     }
7097   }
7098 
7099 
7100   if(locking)
7101     release(&cons.lock);
7102 }
7103 
7104 void
7105 panic(char *s)
7106 {
7107   int i;
7108   uint pcs[10];
7109 
7110   cli();
7111   cons.locking = 0;
7112   cprintf("cpu%d: panic: ", cpu->id);
7113   cprintf(s);
7114   cprintf("\n");
7115   getcallerpcs(&s, pcs);
7116   for(i=0; i<10; i++)
7117     cprintf(" %p", pcs[i]);
7118   panicked = 1; // freeze other CPU
7119   for(;;)
7120     ;
7121 }
7122 
7123 #define BACKSPACE 0x100
7124 #define CRTPORT 0x3d4
7125 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
7126 
7127 static void
7128 cgaputc(int c)
7129 {
7130   int pos;
7131 
7132   // Cursor position: col + 80*row.
7133   outb(CRTPORT, 14);
7134   pos = inb(CRTPORT+1) << 8;
7135   outb(CRTPORT, 15);
7136   pos |= inb(CRTPORT+1);
7137 
7138   if(c == '\n')
7139     pos += 80 - pos%80;
7140   else if(c == BACKSPACE){
7141     if(pos > 0) --pos;
7142   } else
7143     crt[pos++] = (c&0xff) | 0x0700;  // black on white
7144 
7145   if(pos < 0 || pos > 25*80)
7146     panic("pos under/overflow");
7147 
7148 
7149 
7150   if((pos/80) >= 24){  // Scroll up.
7151     memmove(crt, crt+80, sizeof(crt[0])*23*80);
7152     pos -= 80;
7153     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
7154   }
7155 
7156   outb(CRTPORT, 14);
7157   outb(CRTPORT+1, pos>>8);
7158   outb(CRTPORT, 15);
7159   outb(CRTPORT+1, pos);
7160   crt[pos] = ' ' | 0x0700;
7161 }
7162 
7163 void
7164 consputc(int c)
7165 {
7166   if(panicked){
7167     cli();
7168     for(;;)
7169       ;
7170   }
7171 
7172   if(c == BACKSPACE){
7173     uartputc('\b'); uartputc(' '); uartputc('\b');
7174   } else
7175     uartputc(c);
7176   cgaputc(c);
7177 }
7178 
7179 #define INPUT_BUF 128
7180 struct {
7181   char buf[INPUT_BUF];
7182   uint r;  // Read index
7183   uint w;  // Write index
7184   uint e;  // Edit index
7185 } input;
7186 
7187 #define C(x)  ((x)-'@')  // Control-x
7188 
7189 void
7190 consoleintr(int (*getc)(void))
7191 {
7192   int c, doprocdump = 0;
7193 
7194   acquire(&cons.lock);
7195   while((c = getc()) >= 0){
7196     switch(c){
7197     case C('P'):  // Process listing.
7198       doprocdump = 1;   // procdump() locks cons.lock indirectly; invoke later
7199       break;
7200     case C('U'):  // Kill line.
7201       while(input.e != input.w &&
7202             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
7203         input.e--;
7204         consputc(BACKSPACE);
7205       }
7206       break;
7207     case C('H'): case '\x7f':  // Backspace
7208       if(input.e != input.w){
7209         input.e--;
7210         consputc(BACKSPACE);
7211       }
7212       break;
7213     default:
7214       if(c != 0 && input.e-input.r < INPUT_BUF){
7215         c = (c == '\r') ? '\n' : c;
7216         input.buf[input.e++ % INPUT_BUF] = c;
7217         consputc(c);
7218         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
7219           input.w = input.e;
7220           wakeup(&input.r);
7221         }
7222       }
7223       break;
7224     }
7225   }
7226   release(&cons.lock);
7227   if(doprocdump) {
7228     procdump();  // now call procdump() wo. cons.lock held
7229   }
7230 }
7231 
7232 int
7233 consoleread(struct inode *ip, char *dst, int n)
7234 {
7235   uint target;
7236   int c;
7237 
7238   iunlock(ip);
7239   target = n;
7240   acquire(&cons.lock);
7241   while(n > 0){
7242     while(input.r == input.w){
7243       if(proc->killed){
7244         release(&cons.lock);
7245         ilock(ip);
7246         return -1;
7247       }
7248       sleep(&input.r, &cons.lock);
7249     }
7250     c = input.buf[input.r++ % INPUT_BUF];
7251     if(c == C('D')){  // EOF
7252       if(n < target){
7253         // Save ^D for next time, to make sure
7254         // caller gets a 0-byte result.
7255         input.r--;
7256       }
7257       break;
7258     }
7259     *dst++ = c;
7260     --n;
7261     if(c == '\n')
7262       break;
7263   }
7264   release(&cons.lock);
7265   ilock(ip);
7266 
7267   return target - n;
7268 }
7269 
7270 int
7271 consolewrite(struct inode *ip, char *buf, int n)
7272 {
7273   int i;
7274 
7275   iunlock(ip);
7276   acquire(&cons.lock);
7277   for(i = 0; i < n; i++)
7278     consputc(buf[i] & 0xff);
7279   release(&cons.lock);
7280   ilock(ip);
7281 
7282   return n;
7283 }
7284 
7285 void
7286 consoleinit(void)
7287 {
7288   initlock(&cons.lock, "console");
7289 
7290   devsw[CONSOLE].write = consolewrite;
7291   devsw[CONSOLE].read = consoleread;
7292   cons.locking = 1;
7293 
7294   picenable(IRQ_KBD);
7295   ioapicenable(IRQ_KBD, 0);
7296 }
7297 
7298 
7299 
