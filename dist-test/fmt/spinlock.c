1400 // Mutual exclusion spin locks.
1401 
1402 #include "types.h"
1403 #include "defs.h"
1404 #include "param.h"
1405 #include "x86.h"
1406 #include "memlayout.h"
1407 #include "mmu.h"
1408 #include "proc.h"
1409 #include "spinlock.h"
1410 
1411 void
1412 initlock(struct spinlock *lk, char *name)
1413 {
1414   lk->name = name;
1415   lk->locked = 0;
1416   lk->cpu = 0;
1417 }
1418 
1419 // Acquire the lock.
1420 // Loops (spins) until the lock is acquired.
1421 // Holding a lock for a long time may cause
1422 // other CPUs to waste time spinning to acquire it.
1423 void
1424 acquire(struct spinlock *lk)
1425 {
1426   pushcli(); // disable interrupts to avoid deadlock.
1427   if(holding(lk))
1428     panic("acquire");
1429 
1430   // The xchg is atomic.
1431   // It also serializes, so that reads after acquire are not
1432   // reordered before it.
1433   while(xchg(&lk->locked, 1) != 0)
1434     ;
1435 
1436   // Record info about lock acquisition for debugging.
1437   lk->cpu = cpu;
1438   getcallerpcs(&lk, lk->pcs);
1439 }
1440 
1441 
1442 
1443 
1444 
1445 
1446 
1447 
1448 
1449 
1450 // Release the lock.
1451 void
1452 release(struct spinlock *lk)
1453 {
1454   if(!holding(lk))
1455     panic("release");
1456 
1457   lk->pcs[0] = 0;
1458   lk->cpu = 0;
1459 
1460   // The xchg serializes, so that reads before release are
1461   // not reordered after it.  The 1996 PentiumPro manual (Volume 3,
1462   // 7.2) says reads can be carried out speculatively and in
1463   // any order, which implies we need to serialize here.
1464   // But the 2007 Intel 64 Architecture Memory Ordering White
1465   // Paper says that Intel 64 and IA-32 will not move a load
1466   // after a store. So lock->locked = 0 would work here.
1467   // The xchg being asm volatile ensures gcc emits it after
1468   // the above assignments (and after the critical section).
1469   xchg(&lk->locked, 0);
1470 
1471   popcli();
1472 }
1473 
1474 // Record the current call stack in pcs[] by following the %ebp chain.
1475 void
1476 getcallerpcs(void *v, uint pcs[])
1477 {
1478   uint *ebp;
1479   int i;
1480 
1481   ebp = (uint*)v - 2;
1482   for(i = 0; i < 10; i++){
1483     if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
1484       break;
1485     pcs[i] = ebp[1];     // saved %eip
1486     ebp = (uint*)ebp[0]; // saved %ebp
1487   }
1488   for(; i < 10; i++)
1489     pcs[i] = 0;
1490 }
1491 
1492 // Check whether this cpu is holding the lock.
1493 int
1494 holding(struct spinlock *lock)
1495 {
1496   return lock->locked && lock->cpu == cpu;
1497 }
1498 
1499 
1500 // Pushcli/popcli are like cli/sti except that they are matched:
1501 // it takes two popcli to undo two pushcli.  Also, if interrupts
1502 // are off, then pushcli, popcli leaves them off.
1503 
1504 void
1505 pushcli(void)
1506 {
1507   int eflags;
1508 
1509   eflags = readeflags();
1510   cli();
1511   if(cpu->ncli++ == 0)
1512     cpu->intena = eflags & FL_IF;
1513 }
1514 
1515 void
1516 popcli(void)
1517 {
1518   if(readeflags()&FL_IF)
1519     panic("popcli - interruptible");
1520   if(--cpu->ncli < 0)
1521     panic("popcli");
1522   if(cpu->ncli == 0 && cpu->intena)
1523     sti();
1524 }
1525 
1526 
1527 
1528 
1529 
1530 
1531 
1532 
1533 
1534 
1535 
1536 
1537 
1538 
1539 
1540 
1541 
1542 
1543 
1544 
1545 
1546 
1547 
1548 
1549 
