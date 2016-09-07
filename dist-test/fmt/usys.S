7500 #include "syscall.h"
7501 #include "traps.h"
7502 
7503 #define SYSCALL(name) \
7504   .globl name; \
7505   name: \
7506     movl $SYS_ ## name, %eax; \
7507     int $T_SYSCALL; \
7508     ret
7509 
7510 SYSCALL(fork)
7511 SYSCALL(exit)
7512 SYSCALL(wait)
7513 SYSCALL(pipe)
7514 SYSCALL(read)
7515 SYSCALL(write)
7516 SYSCALL(close)
7517 SYSCALL(kill)
7518 SYSCALL(exec)
7519 SYSCALL(open)
7520 SYSCALL(mknod)
7521 SYSCALL(unlink)
7522 SYSCALL(fstat)
7523 SYSCALL(link)
7524 SYSCALL(mkdir)
7525 SYSCALL(chdir)
7526 SYSCALL(dup)
7527 SYSCALL(getpid)
7528 SYSCALL(sbrk)
7529 SYSCALL(sleep)
7530 SYSCALL(uptime)
7531 
7532 
7533 
7534 
7535 
7536 
7537 
7538 
7539 
7540 
7541 
7542 
7543 
7544 
7545 
7546 
7547 
7548 
7549 
