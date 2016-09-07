2650 // Physical memory allocator, intended to allocate
2651 // memory for user processes, kernel stacks, page table pages,
2652 // and pipe buffers. Allocates 4096-byte pages.
2653 
2654 #include "types.h"
2655 #include "defs.h"
2656 #include "param.h"
2657 #include "memlayout.h"
2658 #include "mmu.h"
2659 #include "spinlock.h"
2660 
2661 void freerange(void *vstart, void *vend);
2662 extern char end[]; // first address after kernel loaded from ELF file
2663 
2664 struct run {
2665   struct run *next;
2666 };
2667 
2668 struct {
2669   struct spinlock lock;
2670   int use_lock;
2671   struct run *freelist;
2672 } kmem;
2673 
2674 // Initialization happens in two phases.
2675 // 1. main() calls kinit1() while still using entrypgdir to place just
2676 // the pages mapped by entrypgdir on free list.
2677 // 2. main() calls kinit2() with the rest of the physical pages
2678 // after installing a full page table that maps them on all cores.
2679 void
2680 kinit1(void *vstart, void *vend)
2681 {
2682   initlock(&kmem.lock, "kmem");
2683   kmem.use_lock = 0;
2684   freerange(vstart, vend);
2685 }
2686 
2687 void
2688 kinit2(void *vstart, void *vend)
2689 {
2690   freerange(vstart, vend);
2691   kmem.use_lock = 1;
2692 }
2693 
2694 
2695 
2696 
2697 
2698 
2699 
2700 void
2701 freerange(void *vstart, void *vend)
2702 {
2703   char *p;
2704   p = (char*)PGROUNDUP((uint)vstart);
2705   for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
2706     kfree(p);
2707 }
2708 
2709 // Free the page of physical memory pointed at by v,
2710 // which normally should have been returned by a
2711 // call to kalloc().  (The exception is when
2712 // initializing the allocator; see kinit above.)
2713 void
2714 kfree(char *v)
2715 {
2716   struct run *r;
2717 
2718   if((uint)v % PGSIZE || v < end || v2p(v) >= PHYSTOP)
2719     panic("kfree");
2720 
2721   // Fill with junk to catch dangling refs.
2722   memset(v, 1, PGSIZE);
2723 
2724   if(kmem.use_lock)
2725     acquire(&kmem.lock);
2726   r = (struct run*)v;
2727   r->next = kmem.freelist;
2728   kmem.freelist = r;
2729   if(kmem.use_lock)
2730     release(&kmem.lock);
2731 }
2732 
2733 // Allocate one 4096-byte page of physical memory.
2734 // Returns a pointer that the kernel can use.
2735 // Returns 0 if the memory cannot be allocated.
2736 char*
2737 kalloc(void)
2738 {
2739   struct run *r;
2740 
2741   if(kmem.use_lock)
2742     acquire(&kmem.lock);
2743   r = kmem.freelist;
2744   if(r)
2745     kmem.freelist = r->next;
2746   if(kmem.use_lock)
2747     release(&kmem.lock);
2748   return (char*)r;
2749 }
