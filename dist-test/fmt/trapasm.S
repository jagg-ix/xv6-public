2850 #include "mmu.h"
2851 
2852   # vectors.S sends all traps here.
2853 .globl alltraps
2854 alltraps:
2855   # Build trap frame.
2856   pushl %ds
2857   pushl %es
2858   pushl %fs
2859   pushl %gs
2860   pushal
2861 
2862   # Set up data and per-cpu segments.
2863   movw $(SEG_KDATA<<3), %ax
2864   movw %ax, %ds
2865   movw %ax, %es
2866   movw $(SEG_KCPU<<3), %ax
2867   movw %ax, %fs
2868   movw %ax, %gs
2869 
2870   # Call trap(tf), where tf=%esp
2871   pushl %esp
2872   call trap
2873   addl $4, %esp
2874 
2875   # Return falls through to trapret...
2876 .globl trapret
2877 trapret:
2878   popal
2879   popl %gs
2880   popl %fs
2881   popl %es
2882   popl %ds
2883   addl $0x8, %esp  # trapno and errcode
2884   iret
2885 
2886 
2887 
2888 
2889 
2890 
2891 
2892 
2893 
2894 
2895 
2896 
2897 
2898 
2899 
