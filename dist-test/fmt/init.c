7550 // init: The initial user-level program
7551 
7552 #include "types.h"
7553 #include "stat.h"
7554 #include "user.h"
7555 #include "fcntl.h"
7556 
7557 char *argv[] = { "sh", 0 };
7558 
7559 int
7560 main(void)
7561 {
7562   int pid, wpid;
7563 
7564   if(open("console", O_RDWR) < 0){
7565     mknod("console", 1, 1);
7566     open("console", O_RDWR);
7567   }
7568   dup(0);  // stdout
7569   dup(0);  // stderr
7570 
7571   for(;;){
7572     printf(1, "init: starting sh\n");
7573     pid = fork();
7574     if(pid < 0){
7575       printf(1, "init: fork failed\n");
7576       exit();
7577     }
7578     if(pid == 0){
7579       exec("sh", argv);
7580       printf(1, "init: exec sh failed\n");
7581       exit();
7582     }
7583     while((wpid=wait()) >= 0 && wpid != pid)
7584       printf(1, "zombie!\n");
7585   }
7586 }
7587 
7588 
7589 
7590 
7591 
7592 
7593 
7594 
7595 
7596 
7597 
7598 
7599 
