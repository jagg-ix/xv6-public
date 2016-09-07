8150 #include "asm.h"
8151 #include "memlayout.h"
8152 #include "mmu.h"
8153 
8154 # Start the first CPU: switch to 32-bit protected mode, jump into C.
8155 # The BIOS loads this code from the first sector of the hard disk into
8156 # memory at physical address 0x7c00 and starts executing in real mode
8157 # with %cs=0 %ip=7c00.
8158 
8159 .code16                       # Assemble for 16-bit mode
8160 .globl start
8161 start:
8162   cli                         # BIOS enabled interrupts; disable
8163 
8164   # Zero data segment registers DS, ES, and SS.
8165   xorw    %ax,%ax             # Set %ax to zero
8166   movw    %ax,%ds             # -> Data Segment
8167   movw    %ax,%es             # -> Extra Segment
8168   movw    %ax,%ss             # -> Stack Segment
8169 
8170   # Physical address line A20 is tied to zero so that the first PCs
8171   # with 2 MB would run software that assumed 1 MB.  Undo that.
8172 seta20.1:
8173   inb     $0x64,%al               # Wait for not busy
8174   testb   $0x2,%al
8175   jnz     seta20.1
8176 
8177   movb    $0xd1,%al               # 0xd1 -> port 0x64
8178   outb    %al,$0x64
8179 
8180 seta20.2:
8181   inb     $0x64,%al               # Wait for not busy
8182   testb   $0x2,%al
8183   jnz     seta20.2
8184 
8185   movb    $0xdf,%al               # 0xdf -> port 0x60
8186   outb    %al,$0x60
8187 
8188   # Switch from real to protected mode.  Use a bootstrap GDT that makes
8189   # virtual addresses map directly to physical addresses so that the
8190   # effective memory map doesn't change during the transition.
8191   lgdt    gdtdesc
8192   movl    %cr0, %eax
8193   orl     $CR0_PE, %eax
8194   movl    %eax, %cr0
8195 
8196   # Complete transition to 32-bit protected mode by using long jmp
8197   # to reload %cs and %eip.  The segment descriptors are set up with no
8198   # translation, so that the mapping is still the identity mapping.
8199   ljmp    $(SEG_KCODE<<3), $start32
8200 .code32  # Tell assembler to generate 32-bit code now.
8201 start32:
8202   # Set up the protected-mode data segment registers
8203   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
8204   movw    %ax, %ds                # -> DS: Data Segment
8205   movw    %ax, %es                # -> ES: Extra Segment
8206   movw    %ax, %ss                # -> SS: Stack Segment
8207   movw    $0, %ax                 # Zero segments not ready for use
8208   movw    %ax, %fs                # -> FS
8209   movw    %ax, %gs                # -> GS
8210 
8211   # Set up the stack pointer and call into C.
8212   movl    $start, %esp
8213   call    bootmain
8214 
8215   # If bootmain returns (it shouldn't), trigger a Bochs
8216   # breakpoint if running under Bochs, then loop.
8217   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
8218   movw    %ax, %dx
8219   outw    %ax, %dx
8220   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
8221   outw    %ax, %dx
8222 spin:
8223   jmp     spin
8224 
8225 # Bootstrap GDT
8226 .p2align 2                                # force 4 byte alignment
8227 gdt:
8228   SEG_NULLASM                             # null seg
8229   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
8230   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
8231 
8232 gdtdesc:
8233   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
8234   .long   gdt                             # address gdt
8235 
8236 
8237 
8238 
8239 
8240 
8241 
8242 
8243 
8244 
8245 
8246 
8247 
8248 
8249 
