2100 #include "types.h"
2101 #include "defs.h"
2102 #include "param.h"
2103 #include "memlayout.h"
2104 #include "mmu.h"
2105 #include "x86.h"
2106 #include "proc.h"
2107 #include "spinlock.h"
2108 
2109 struct {
2110   struct spinlock lock;
2111   struct proc proc[NPROC];
2112 } ptable;
2113 
2114 static struct proc *initproc;
2115 
2116 int nextpid = 1;
2117 extern void forkret(void);
2118 extern void trapret(void);
2119 
2120 static void wakeup1(void *chan);
2121 
2122 void
2123 pinit(void)
2124 {
2125   initlock(&ptable.lock, "ptable");
2126 }
2127 
2128 // Look in the process table for an UNUSED proc.
2129 // If found, change state to EMBRYO and initialize
2130 // state required to run in the kernel.
2131 // Otherwise return 0.
2132 static struct proc*
2133 allocproc(void)
2134 {
2135   struct proc *p;
2136   char *sp;
2137 
2138   acquire(&ptable.lock);
2139   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2140     if(p->state == UNUSED)
2141       goto found;
2142   release(&ptable.lock);
2143   return 0;
2144 
2145 found:
2146   p->state = EMBRYO;
2147   p->pid = nextpid++;
2148   release(&ptable.lock);
2149 
2150   // Allocate kernel stack.
2151   if((p->kstack = kalloc()) == 0){
2152     p->state = UNUSED;
2153     return 0;
2154   }
2155   sp = p->kstack + KSTACKSIZE;
2156 
2157   // Leave room for trap frame.
2158   sp -= sizeof *p->tf;
2159   p->tf = (struct trapframe*)sp;
2160 
2161   // Set up new context to start executing at forkret,
2162   // which returns to trapret.
2163   sp -= 4;
2164   *(uint*)sp = (uint)trapret;
2165 
2166   sp -= sizeof *p->context;
2167   p->context = (struct context*)sp;
2168   memset(p->context, 0, sizeof *p->context);
2169   p->context->eip = (uint)forkret;
2170 
2171   return p;
2172 }
2173 
2174 // Set up first user process.
2175 void
2176 userinit(void)
2177 {
2178   struct proc *p;
2179   extern char _binary_initcode_start[], _binary_initcode_size[];
2180 
2181   p = allocproc();
2182   initproc = p;
2183   if((p->pgdir = setupkvm()) == 0)
2184     panic("userinit: out of memory?");
2185   inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
2186   p->sz = PGSIZE;
2187   memset(p->tf, 0, sizeof(*p->tf));
2188   p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
2189   p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
2190   p->tf->es = p->tf->ds;
2191   p->tf->ss = p->tf->ds;
2192   p->tf->eflags = FL_IF;
2193   p->tf->esp = PGSIZE;
2194   p->tf->eip = 0;  // beginning of initcode.S
2195 
2196   safestrcpy(p->name, "initcode", sizeof(p->name));
2197   p->cwd = namei("/");
2198 
2199 
2200   p->state = RUNNABLE;
2201 }
2202 
2203 // Grow current process's memory by n bytes.
2204 // Return 0 on success, -1 on failure.
2205 int
2206 growproc(int n)
2207 {
2208   uint sz;
2209 
2210   sz = proc->sz;
2211   if(n > 0){
2212     if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
2213       return -1;
2214   } else if(n < 0){
2215     if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
2216       return -1;
2217   }
2218   proc->sz = sz;
2219   switchuvm(proc);
2220   return 0;
2221 }
2222 
2223 // Create a new process copying p as the parent.
2224 // Sets up stack to return as if from system call.
2225 // Caller must set state of returned proc to RUNNABLE.
2226 int
2227 fork(void)
2228 {
2229   int i, pid;
2230   struct proc *np;
2231 
2232   // Allocate process.
2233   if((np = allocproc()) == 0)
2234     return -1;
2235 
2236   // Copy process state from p.
2237   if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
2238     kfree(np->kstack);
2239     np->kstack = 0;
2240     np->state = UNUSED;
2241     return -1;
2242   }
2243   np->sz = proc->sz;
2244   np->parent = proc;
2245   *np->tf = *proc->tf;
2246 
2247   // Clear %eax so that fork returns 0 in the child.
2248   np->tf->eax = 0;
2249 
2250   for(i = 0; i < NOFILE; i++)
2251     if(proc->ofile[i])
2252       np->ofile[i] = filedup(proc->ofile[i]);
2253   np->cwd = idup(proc->cwd);
2254 
2255   safestrcpy(np->name, proc->name, sizeof(proc->name));
2256 
2257   pid = np->pid;
2258 
2259   // lock to force the compiler to emit the np->state write last.
2260   acquire(&ptable.lock);
2261   np->state = RUNNABLE;
2262   release(&ptable.lock);
2263 
2264   return pid;
2265 }
2266 
2267 // Exit the current process.  Does not return.
2268 // An exited process remains in the zombie state
2269 // until its parent calls wait() to find out it exited.
2270 void
2271 exit(void)
2272 {
2273   struct proc *p;
2274   int fd;
2275 
2276   if(proc == initproc)
2277     panic("init exiting");
2278 
2279   // Close all open files.
2280   for(fd = 0; fd < NOFILE; fd++){
2281     if(proc->ofile[fd]){
2282       fileclose(proc->ofile[fd]);
2283       proc->ofile[fd] = 0;
2284     }
2285   }
2286 
2287   begin_op();
2288   iput(proc->cwd);
2289   end_op();
2290   proc->cwd = 0;
2291 
2292   acquire(&ptable.lock);
2293 
2294   // Parent might be sleeping in wait().
2295   wakeup1(proc->parent);
2296 
2297 
2298 
2299 
2300   // Pass abandoned children to init.
2301   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2302     if(p->parent == proc){
2303       p->parent = initproc;
2304       if(p->state == ZOMBIE)
2305         wakeup1(initproc);
2306     }
2307   }
2308 
2309   // Jump into the scheduler, never to return.
2310   proc->state = ZOMBIE;
2311   sched();
2312   panic("zombie exit");
2313 }
2314 
2315 // Wait for a child process to exit and return its pid.
2316 // Return -1 if this process has no children.
2317 int
2318 wait(void)
2319 {
2320   struct proc *p;
2321   int havekids, pid;
2322 
2323   acquire(&ptable.lock);
2324   for(;;){
2325     // Scan through table looking for zombie children.
2326     havekids = 0;
2327     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2328       if(p->parent != proc)
2329         continue;
2330       havekids = 1;
2331       if(p->state == ZOMBIE){
2332         // Found one.
2333         pid = p->pid;
2334         kfree(p->kstack);
2335         p->kstack = 0;
2336         freevm(p->pgdir);
2337         p->state = UNUSED;
2338         p->pid = 0;
2339         p->parent = 0;
2340         p->name[0] = 0;
2341         p->killed = 0;
2342         release(&ptable.lock);
2343         return pid;
2344       }
2345     }
2346 
2347 
2348 
2349 
2350     // No point waiting if we don't have any children.
2351     if(!havekids || proc->killed){
2352       release(&ptable.lock);
2353       return -1;
2354     }
2355 
2356     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
2357     sleep(proc, &ptable.lock);  //DOC: wait-sleep
2358   }
2359 }
2360 
2361 // Per-CPU process scheduler.
2362 // Each CPU calls scheduler() after setting itself up.
2363 // Scheduler never returns.  It loops, doing:
2364 //  - choose a process to run
2365 //  - swtch to start running that process
2366 //  - eventually that process transfers control
2367 //      via swtch back to the scheduler.
2368 void
2369 scheduler(void)
2370 {
2371   struct proc *p;
2372 
2373   for(;;){
2374     // Enable interrupts on this processor.
2375     sti();
2376 
2377     // Loop over process table looking for process to run.
2378     acquire(&ptable.lock);
2379     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2380       if(p->state != RUNNABLE)
2381         continue;
2382 
2383       // Switch to chosen process.  It is the process's job
2384       // to release ptable.lock and then reacquire it
2385       // before jumping back to us.
2386       proc = p;
2387       switchuvm(p);
2388       p->state = RUNNING;
2389       swtch(&cpu->scheduler, proc->context);
2390       switchkvm();
2391 
2392       // Process is done running for now.
2393       // It should have changed its p->state before coming back.
2394       proc = 0;
2395     }
2396     release(&ptable.lock);
2397 
2398   }
2399 }
2400 // Enter scheduler.  Must hold only ptable.lock
2401 // and have changed proc->state.
2402 void
2403 sched(void)
2404 {
2405   int intena;
2406 
2407   if(!holding(&ptable.lock))
2408     panic("sched ptable.lock");
2409   if(cpu->ncli != 1)
2410     panic("sched locks");
2411   if(proc->state == RUNNING)
2412     panic("sched running");
2413   if(readeflags()&FL_IF)
2414     panic("sched interruptible");
2415   intena = cpu->intena;
2416   swtch(&proc->context, cpu->scheduler);
2417   cpu->intena = intena;
2418 }
2419 
2420 // Give up the CPU for one scheduling round.
2421 void
2422 yield(void)
2423 {
2424   acquire(&ptable.lock);  //DOC: yieldlock
2425   proc->state = RUNNABLE;
2426   sched();
2427   release(&ptable.lock);
2428 }
2429 
2430 // A fork child's very first scheduling by scheduler()
2431 // will swtch here.  "Return" to user space.
2432 void
2433 forkret(void)
2434 {
2435   static int first = 1;
2436   // Still holding ptable.lock from scheduler.
2437   release(&ptable.lock);
2438 
2439   if (first) {
2440     // Some initialization functions must be run in the context
2441     // of a regular process (e.g., they call sleep), and thus cannot
2442     // be run from main().
2443     first = 0;
2444     iinit(ROOTDEV);
2445     initlog(ROOTDEV);
2446   }
2447 
2448   // Return to "caller", actually trapret (see allocproc).
2449 }
2450 // Atomically release lock and sleep on chan.
2451 // Reacquires lock when awakened.
2452 void
2453 sleep(void *chan, struct spinlock *lk)
2454 {
2455   if(proc == 0)
2456     panic("sleep");
2457 
2458   if(lk == 0)
2459     panic("sleep without lk");
2460 
2461   // Must acquire ptable.lock in order to
2462   // change p->state and then call sched.
2463   // Once we hold ptable.lock, we can be
2464   // guaranteed that we won't miss any wakeup
2465   // (wakeup runs with ptable.lock locked),
2466   // so it's okay to release lk.
2467   if(lk != &ptable.lock){  //DOC: sleeplock0
2468     acquire(&ptable.lock);  //DOC: sleeplock1
2469     release(lk);
2470   }
2471 
2472   // Go to sleep.
2473   proc->chan = chan;
2474   proc->state = SLEEPING;
2475   sched();
2476 
2477   // Tidy up.
2478   proc->chan = 0;
2479 
2480   // Reacquire original lock.
2481   if(lk != &ptable.lock){  //DOC: sleeplock2
2482     release(&ptable.lock);
2483     acquire(lk);
2484   }
2485 }
2486 
2487 // Wake up all processes sleeping on chan.
2488 // The ptable lock must be held.
2489 static void
2490 wakeup1(void *chan)
2491 {
2492   struct proc *p;
2493 
2494   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2495     if(p->state == SLEEPING && p->chan == chan)
2496       p->state = RUNNABLE;
2497 }
2498 
2499 
2500 // Wake up all processes sleeping on chan.
2501 void
2502 wakeup(void *chan)
2503 {
2504   acquire(&ptable.lock);
2505   wakeup1(chan);
2506   release(&ptable.lock);
2507 }
2508 
2509 // Kill the process with the given pid.
2510 // Process won't exit until it returns
2511 // to user space (see trap in trap.c).
2512 int
2513 kill(int pid)
2514 {
2515   struct proc *p;
2516 
2517   acquire(&ptable.lock);
2518   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2519     if(p->pid == pid){
2520       p->killed = 1;
2521       // Wake process from sleep if necessary.
2522       if(p->state == SLEEPING)
2523         p->state = RUNNABLE;
2524       release(&ptable.lock);
2525       return 0;
2526     }
2527   }
2528   release(&ptable.lock);
2529   return -1;
2530 }
2531 
2532 // Print a process listing to console.  For debugging.
2533 // Runs when user types ^P on console.
2534 // No lock to avoid wedging a stuck machine further.
2535 void
2536 procdump(void)
2537 {
2538   static char *states[] = {
2539   [UNUSED]    "unused",
2540   [EMBRYO]    "embryo",
2541   [SLEEPING]  "sleep ",
2542   [RUNNABLE]  "runble",
2543   [RUNNING]   "run   ",
2544   [ZOMBIE]    "zombie"
2545   };
2546   int i;
2547   struct proc *p;
2548   char *state;
2549   uint pc[10];
2550   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2551     if(p->state == UNUSED)
2552       continue;
2553     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
2554       state = states[p->state];
2555     else
2556       state = "???";
2557     cprintf("%d %s %s", p->pid, state, p->name);
2558     if(p->state == SLEEPING){
2559       getcallerpcs((uint*)p->context->ebp+2, pc);
2560       for(i=0; i<10 && pc[i] != 0; i++)
2561         cprintf(" %p", pc[i]);
2562     }
2563     cprintf("\n");
2564   }
2565 }
2566 
2567 
2568 
2569 
2570 
2571 
2572 
2573 
2574 
2575 
2576 
2577 
2578 
2579 
2580 
2581 
2582 
2583 
2584 
2585 
2586 
2587 
2588 
2589 
2590 
2591 
2592 
2593 
2594 
2595 
2596 
2597 
2598 
2599 
