3100 #include "types.h"
3101 #include "defs.h"
3102 #include "param.h"
3103 #include "memlayout.h"
3104 #include "mmu.h"
3105 #include "proc.h"
3106 #include "x86.h"
3107 #include "syscall.h"
3108 
3109 // User code makes a system call with INT T_SYSCALL.
3110 // System call number in %eax.
3111 // Arguments on the stack, from the user call to the C
3112 // library system call function. The saved user %esp points
3113 // to a saved program counter, and then the first argument.
3114 
3115 // Fetch the int at addr from the current process.
3116 int
3117 fetchint(uint addr, int *ip)
3118 {
3119   if(addr >= proc->sz || addr+4 > proc->sz)
3120     return -1;
3121   *ip = *(int*)(addr);
3122   return 0;
3123 }
3124 
3125 // Fetch the nul-terminated string at addr from the current process.
3126 // Doesn't actually copy the string - just sets *pp to point at it.
3127 // Returns length of string, not including nul.
3128 int
3129 fetchstr(uint addr, char **pp)
3130 {
3131   char *s, *ep;
3132 
3133   if(addr >= proc->sz)
3134     return -1;
3135   *pp = (char*)addr;
3136   ep = (char*)proc->sz;
3137   for(s = *pp; s < ep; s++)
3138     if(*s == 0)
3139       return s - *pp;
3140   return -1;
3141 }
3142 
3143 // Fetch the nth 32-bit system call argument.
3144 int
3145 argint(int n, int *ip)
3146 {
3147   return fetchint(proc->tf->esp + 4 + 4*n, ip);
3148 }
3149 
3150 // Fetch the nth word-sized system call argument as a pointer
3151 // to a block of memory of size n bytes.  Check that the pointer
3152 // lies within the process address space.
3153 int
3154 argptr(int n, char **pp, int size)
3155 {
3156   int i;
3157 
3158   if(argint(n, &i) < 0)
3159     return -1;
3160   if((uint)i >= proc->sz || (uint)i+size > proc->sz)
3161     return -1;
3162   *pp = (char*)i;
3163   return 0;
3164 }
3165 
3166 // Fetch the nth word-sized system call argument as a string pointer.
3167 // Check that the pointer is valid and the string is nul-terminated.
3168 // (There is no shared writable memory, so the string can't change
3169 // between this check and being used by the kernel.)
3170 int
3171 argstr(int n, char **pp)
3172 {
3173   int addr;
3174   if(argint(n, &addr) < 0)
3175     return -1;
3176   return fetchstr(addr, pp);
3177 }
3178 
3179 extern int sys_chdir(void);
3180 extern int sys_close(void);
3181 extern int sys_dup(void);
3182 extern int sys_exec(void);
3183 extern int sys_exit(void);
3184 extern int sys_fork(void);
3185 extern int sys_fstat(void);
3186 extern int sys_getpid(void);
3187 extern int sys_kill(void);
3188 extern int sys_link(void);
3189 extern int sys_mkdir(void);
3190 extern int sys_mknod(void);
3191 extern int sys_open(void);
3192 extern int sys_pipe(void);
3193 extern int sys_read(void);
3194 extern int sys_sbrk(void);
3195 extern int sys_sleep(void);
3196 extern int sys_unlink(void);
3197 extern int sys_wait(void);
3198 extern int sys_write(void);
3199 extern int sys_uptime(void);
3200 static int (*syscalls[])(void) = {
3201 [SYS_fork]    sys_fork,
3202 [SYS_exit]    sys_exit,
3203 [SYS_wait]    sys_wait,
3204 [SYS_pipe]    sys_pipe,
3205 [SYS_read]    sys_read,
3206 [SYS_kill]    sys_kill,
3207 [SYS_exec]    sys_exec,
3208 [SYS_fstat]   sys_fstat,
3209 [SYS_chdir]   sys_chdir,
3210 [SYS_dup]     sys_dup,
3211 [SYS_getpid]  sys_getpid,
3212 [SYS_sbrk]    sys_sbrk,
3213 [SYS_sleep]   sys_sleep,
3214 [SYS_uptime]  sys_uptime,
3215 [SYS_open]    sys_open,
3216 [SYS_write]   sys_write,
3217 [SYS_mknod]   sys_mknod,
3218 [SYS_unlink]  sys_unlink,
3219 [SYS_link]    sys_link,
3220 [SYS_mkdir]   sys_mkdir,
3221 [SYS_close]   sys_close,
3222 };
3223 
3224 void
3225 syscall(void)
3226 {
3227   int num;
3228 
3229   num = proc->tf->eax;
3230   if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
3231     proc->tf->eax = syscalls[num]();
3232   } else {
3233     cprintf("%d %s: unknown sys call %d\n",
3234             proc->pid, proc->name, num);
3235     proc->tf->eax = -1;
3236   }
3237 }
3238 
3239 
3240 
3241 
3242 
3243 
3244 
3245 
3246 
3247 
3248 
3249 
