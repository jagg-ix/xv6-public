1100 #include "asm.h"
1101 #include "memlayout.h"
1102 #include "mmu.h"
1103 
1104 # Each non-boot CPU ("AP") is started up in response to a STARTUP
1105 # IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
1106 # Specification says that the AP will start in real mode with CS:IP
1107 # set to XY00:0000, where XY is an 8-bit value sent with the
1108 # STARTUP. Thus this code must start at a 4096-byte boundary.
1109 #
1110 # Because this code sets DS to zero, it must sit
1111 # at an address in the low 2^16 bytes.
1112 #
1113 # Startothers (in main.c) sends the STARTUPs one at a time.
1114 # It copies this code (start) at 0x7000.  It puts the address of
1115 # a newly allocated per-core stack in start-4,the address of the
1116 # place to jump to (mpenter) in start-8, and the physical address
1117 # of entrypgdir in start-12.
1118 #
1119 # This code is identical to bootasm.S except:
1120 #   - it does not need to enable A20
1121 #   - it uses the address at start-4, start-8, and start-12
1122 
1123 .code16
1124 .globl start
1125 start:
1126   cli
1127 
1128   xorw    %ax,%ax
1129   movw    %ax,%ds
1130   movw    %ax,%es
1131   movw    %ax,%ss
1132 
1133   lgdt    gdtdesc
1134   movl    %cr0, %eax
1135   orl     $CR0_PE, %eax
1136   movl    %eax, %cr0
1137 
1138   ljmpl    $(SEG_KCODE<<3), $(start32)
1139 
1140 .code32
1141 start32:
1142   movw    $(SEG_KDATA<<3), %ax
1143   movw    %ax, %ds
1144   movw    %ax, %es
1145   movw    %ax, %ss
1146   movw    $0, %ax
1147   movw    %ax, %fs
1148   movw    %ax, %gs
1149 
1150   # Turn on page size extension for 4Mbyte pages
1151   movl    %cr4, %eax
1152   orl     $(CR4_PSE), %eax
1153   movl    %eax, %cr4
1154   # Use enterpgdir as our initial page table
1155   movl    (start-12), %eax
1156   movl    %eax, %cr3
1157   # Turn on paging.
1158   movl    %cr0, %eax
1159   orl     $(CR0_PE|CR0_PG|CR0_WP), %eax
1160   movl    %eax, %cr0
1161 
1162   # Switch to the stack allocated by startothers()
1163   movl    (start-4), %esp
1164   # Call mpenter()
1165   call	 *(start-8)
1166 
1167   movw    $0x8a00, %ax
1168   movw    %ax, %dx
1169   outw    %ax, %dx
1170   movw    $0x8ae0, %ax
1171   outw    %ax, %dx
1172 spin:
1173   jmp     spin
1174 
1175 .p2align 2
1176 gdt:
1177   SEG_NULLASM
1178   SEG_ASM(STA_X|STA_R, 0, 0xffffffff)
1179   SEG_ASM(STA_W, 0, 0xffffffff)
1180 
1181 
1182 gdtdesc:
1183   .word   (gdtdesc - gdt - 1)
1184   .long   gdt
1185 
1186 
1187 
1188 
1189 
1190 
1191 
1192 
1193 
1194 
1195 
1196 
1197 
1198 
1199 
