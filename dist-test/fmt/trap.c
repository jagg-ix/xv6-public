2900 #include "types.h"
2901 #include "defs.h"
2902 #include "param.h"
2903 #include "memlayout.h"
2904 #include "mmu.h"
2905 #include "proc.h"
2906 #include "x86.h"
2907 #include "traps.h"
2908 #include "spinlock.h"
2909 
2910 // Interrupt descriptor table (shared by all CPUs).
2911 struct gatedesc idt[256];
2912 extern uint vectors[];  // in vectors.S: array of 256 entry pointers
2913 struct spinlock tickslock;
2914 uint ticks;
2915 
2916 void
2917 tvinit(void)
2918 {
2919   int i;
2920 
2921   for(i = 0; i < 256; i++)
2922     SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
2923   SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
2924 
2925   initlock(&tickslock, "time");
2926 }
2927 
2928 void
2929 idtinit(void)
2930 {
2931   lidt(idt, sizeof(idt));
2932 }
2933 
2934 void
2935 trap(struct trapframe *tf)
2936 {
2937   if(tf->trapno == T_SYSCALL){
2938     if(proc->killed)
2939       exit();
2940     proc->tf = tf;
2941     syscall();
2942     if(proc->killed)
2943       exit();
2944     return;
2945   }
2946 
2947   switch(tf->trapno){
2948   case T_IRQ0 + IRQ_TIMER:
2949     if(cpu->id == 0){
2950       acquire(&tickslock);
2951       ticks++;
2952       wakeup(&ticks);
2953       release(&tickslock);
2954     }
2955     lapiceoi();
2956     break;
2957   case T_IRQ0 + IRQ_IDE:
2958     ideintr();
2959     lapiceoi();
2960     break;
2961   case T_IRQ0 + IRQ_IDE+1:
2962     // Bochs generates spurious IDE1 interrupts.
2963     break;
2964   case T_IRQ0 + IRQ_KBD:
2965     kbdintr();
2966     lapiceoi();
2967     break;
2968   case T_IRQ0 + IRQ_COM1:
2969     uartintr();
2970     lapiceoi();
2971     break;
2972   case T_IRQ0 + 7:
2973   case T_IRQ0 + IRQ_SPURIOUS:
2974     cprintf("cpu%d: spurious interrupt at %x:%x\n",
2975             cpu->id, tf->cs, tf->eip);
2976     lapiceoi();
2977     break;
2978 
2979   default:
2980     if(proc == 0 || (tf->cs&3) == 0){
2981       // In kernel, it must be our mistake.
2982       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
2983               tf->trapno, cpu->id, tf->eip, rcr2());
2984       panic("trap");
2985     }
2986     // In user space, assume process misbehaved.
2987     cprintf("pid %d %s: trap %d err %d on cpu %d "
2988             "eip 0x%x addr 0x%x--kill proc\n",
2989             proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip,
2990             rcr2());
2991     proc->killed = 1;
2992   }
2993 
2994   // Force process exit if it has been killed and is in user space.
2995   // (If it is still executing in the kernel, let it keep running
2996   // until it gets to the regular system call return.)
2997   if(proc && proc->killed && (tf->cs&3) == DPL_USER)
2998     exit();
2999 
3000   // Force process to give up CPU on clock tick.
3001   // If interrupts were on while locks held, would need to check nlock.
3002   if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
3003     yield();
3004 
3005   // Check if the process has been killed since we yielded
3006   if(proc && proc->killed && (tf->cs&3) == DPL_USER)
3007     exit();
3008 }
3009 
3010 
3011 
3012 
3013 
3014 
3015 
3016 
3017 
3018 
3019 
3020 
3021 
3022 
3023 
3024 
3025 
3026 
3027 
3028 
3029 
3030 
3031 
3032 
3033 
3034 
3035 
3036 
3037 
3038 
3039 
3040 
3041 
3042 
3043 
3044 
3045 
3046 
3047 
3048 
3049 
