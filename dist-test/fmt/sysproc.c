3250 #include "types.h"
3251 #include "x86.h"
3252 #include "defs.h"
3253 #include "date.h"
3254 #include "param.h"
3255 #include "memlayout.h"
3256 #include "mmu.h"
3257 #include "proc.h"
3258 
3259 int
3260 sys_fork(void)
3261 {
3262   return fork();
3263 }
3264 
3265 int
3266 sys_exit(void)
3267 {
3268   exit();
3269   return 0;  // not reached
3270 }
3271 
3272 int
3273 sys_wait(void)
3274 {
3275   return wait();
3276 }
3277 
3278 int
3279 sys_kill(void)
3280 {
3281   int pid;
3282 
3283   if(argint(0, &pid) < 0)
3284     return -1;
3285   return kill(pid);
3286 }
3287 
3288 int
3289 sys_getpid(void)
3290 {
3291   return proc->pid;
3292 }
3293 
3294 
3295 
3296 
3297 
3298 
3299 
3300 int
3301 sys_sbrk(void)
3302 {
3303   int addr;
3304   int n;
3305 
3306   if(argint(0, &n) < 0)
3307     return -1;
3308   addr = proc->sz;
3309   if(growproc(n) < 0)
3310     return -1;
3311   return addr;
3312 }
3313 
3314 int
3315 sys_sleep(void)
3316 {
3317   int n;
3318   uint ticks0;
3319 
3320   if(argint(0, &n) < 0)
3321     return -1;
3322   acquire(&tickslock);
3323   ticks0 = ticks;
3324   while(ticks - ticks0 < n){
3325     if(proc->killed){
3326       release(&tickslock);
3327       return -1;
3328     }
3329     sleep(&ticks, &tickslock);
3330   }
3331   release(&tickslock);
3332   return 0;
3333 }
3334 
3335 // return how many clock tick interrupts have occurred
3336 // since start.
3337 int
3338 sys_uptime(void)
3339 {
3340   uint xticks;
3341 
3342   acquire(&tickslock);
3343   xticks = ticks;
3344   release(&tickslock);
3345   return xticks;
3346 }
3347 
3348 
3349 
