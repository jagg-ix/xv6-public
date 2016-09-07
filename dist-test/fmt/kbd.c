6950 #include "types.h"
6951 #include "x86.h"
6952 #include "defs.h"
6953 #include "kbd.h"
6954 
6955 int
6956 kbdgetc(void)
6957 {
6958   static uint shift;
6959   static uchar *charcode[4] = {
6960     normalmap, shiftmap, ctlmap, ctlmap
6961   };
6962   uint st, data, c;
6963 
6964   st = inb(KBSTATP);
6965   if((st & KBS_DIB) == 0)
6966     return -1;
6967   data = inb(KBDATAP);
6968 
6969   if(data == 0xE0){
6970     shift |= E0ESC;
6971     return 0;
6972   } else if(data & 0x80){
6973     // Key released
6974     data = (shift & E0ESC ? data : data & 0x7F);
6975     shift &= ~(shiftcode[data] | E0ESC);
6976     return 0;
6977   } else if(shift & E0ESC){
6978     // Last character was an E0 escape; or with 0x80
6979     data |= 0x80;
6980     shift &= ~E0ESC;
6981   }
6982 
6983   shift |= shiftcode[data];
6984   shift ^= togglecode[data];
6985   c = charcode[shift & (CTL | SHIFT)][data];
6986   if(shift & CAPSLOCK){
6987     if('a' <= c && c <= 'z')
6988       c += 'A' - 'a';
6989     else if('A' <= c && c <= 'Z')
6990       c += 'a' - 'A';
6991   }
6992   return c;
6993 }
6994 
6995 void
6996 kbdintr(void)
6997 {
6998   consoleintr(kbdgetc);
6999 }
