7450 # Initial process execs /init.
7451 
7452 #include "syscall.h"
7453 #include "traps.h"
7454 
7455 
7456 # exec(init, argv)
7457 .globl start
7458 start:
7459   pushl $argv
7460   pushl $init
7461   pushl $0  // where caller pc would be
7462   movl $SYS_exec, %eax
7463   int $T_SYSCALL
7464 
7465 # for(;;) exit();
7466 exit:
7467   movl $SYS_exit, %eax
7468   int $T_SYSCALL
7469   jmp exit
7470 
7471 # char init[] = "/init\0";
7472 init:
7473   .string "/init\0"
7474 
7475 # char *argv[] = { init, 0 };
7476 .p2align 2
7477 argv:
7478   .long init
7479   .long 0
7480 
7481 
7482 
7483 
7484 
7485 
7486 
7487 
7488 
7489 
7490 
7491 
7492 
7493 
7494 
7495 
7496 
7497 
7498 
7499 
