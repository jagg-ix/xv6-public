2750 // x86 trap and interrupt constants.
2751 
2752 // Processor-defined:
2753 #define T_DIVIDE         0      // divide error
2754 #define T_DEBUG          1      // debug exception
2755 #define T_NMI            2      // non-maskable interrupt
2756 #define T_BRKPT          3      // breakpoint
2757 #define T_OFLOW          4      // overflow
2758 #define T_BOUND          5      // bounds check
2759 #define T_ILLOP          6      // illegal opcode
2760 #define T_DEVICE         7      // device not available
2761 #define T_DBLFLT         8      // double fault
2762 // #define T_COPROC      9      // reserved (not used since 486)
2763 #define T_TSS           10      // invalid task switch segment
2764 #define T_SEGNP         11      // segment not present
2765 #define T_STACK         12      // stack exception
2766 #define T_GPFLT         13      // general protection fault
2767 #define T_PGFLT         14      // page fault
2768 // #define T_RES        15      // reserved
2769 #define T_FPERR         16      // floating point error
2770 #define T_ALIGN         17      // aligment check
2771 #define T_MCHK          18      // machine check
2772 #define T_SIMDERR       19      // SIMD floating point error
2773 
2774 // These are arbitrarily chosen, but with care not to overlap
2775 // processor defined exceptions or interrupt vectors.
2776 #define T_SYSCALL       64      // system call
2777 #define T_DEFAULT      500      // catchall
2778 
2779 #define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ
2780 
2781 #define IRQ_TIMER        0
2782 #define IRQ_KBD          1
2783 #define IRQ_COM1         4
2784 #define IRQ_IDE         14
2785 #define IRQ_ERROR       19
2786 #define IRQ_SPURIOUS    31
2787 
2788 
2789 
2790 
2791 
2792 
2793 
2794 
2795 
2796 
2797 
2798 
2799 
