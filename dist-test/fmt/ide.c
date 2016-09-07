3650 // Simple PIO-based (non-DMA) IDE driver code.
3651 
3652 #include "types.h"
3653 #include "defs.h"
3654 #include "param.h"
3655 #include "memlayout.h"
3656 #include "mmu.h"
3657 #include "proc.h"
3658 #include "x86.h"
3659 #include "traps.h"
3660 #include "spinlock.h"
3661 #include "fs.h"
3662 #include "buf.h"
3663 
3664 #define SECTOR_SIZE   512
3665 #define IDE_BSY       0x80
3666 #define IDE_DRDY      0x40
3667 #define IDE_DF        0x20
3668 #define IDE_ERR       0x01
3669 
3670 #define IDE_CMD_READ  0x20
3671 #define IDE_CMD_WRITE 0x30
3672 
3673 // idequeue points to the buf now being read/written to the disk.
3674 // idequeue->qnext points to the next buf to be processed.
3675 // You must hold idelock while manipulating queue.
3676 
3677 static struct spinlock idelock;
3678 static struct buf *idequeue;
3679 
3680 static int havedisk1;
3681 static void idestart(struct buf*);
3682 
3683 // Wait for IDE disk to become ready.
3684 static int
3685 idewait(int checkerr)
3686 {
3687   int r;
3688 
3689   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
3690     ;
3691   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
3692     return -1;
3693   return 0;
3694 }
3695 
3696 
3697 
3698 
3699 
3700 void
3701 ideinit(void)
3702 {
3703   int i;
3704 
3705   initlock(&idelock, "ide");
3706   picenable(IRQ_IDE);
3707   ioapicenable(IRQ_IDE, ncpu - 1);
3708   idewait(0);
3709 
3710   // Check if disk 1 is present
3711   outb(0x1f6, 0xe0 | (1<<4));
3712   for(i=0; i<1000; i++){
3713     if(inb(0x1f7) != 0){
3714       havedisk1 = 1;
3715       break;
3716     }
3717   }
3718 
3719   // Switch back to disk 0.
3720   outb(0x1f6, 0xe0 | (0<<4));
3721 }
3722 
3723 // Start the request for b.  Caller must hold idelock.
3724 static void
3725 idestart(struct buf *b)
3726 {
3727   if(b == 0)
3728     panic("idestart");
3729   if(b->blockno >= FSSIZE)
3730     panic("incorrect blockno");
3731   int sector_per_block =  BSIZE/SECTOR_SIZE;
3732   int sector = b->blockno * sector_per_block;
3733 
3734   if (sector_per_block > 7) panic("idestart");
3735 
3736   idewait(0);
3737   outb(0x3f6, 0);  // generate interrupt
3738   outb(0x1f2, sector_per_block);  // number of sectors
3739   outb(0x1f3, sector & 0xff);
3740   outb(0x1f4, (sector >> 8) & 0xff);
3741   outb(0x1f5, (sector >> 16) & 0xff);
3742   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
3743   if(b->flags & B_DIRTY){
3744     outb(0x1f7, IDE_CMD_WRITE);
3745     outsl(0x1f0, b->data, BSIZE/4);
3746   } else {
3747     outb(0x1f7, IDE_CMD_READ);
3748   }
3749 }
3750 // Interrupt handler.
3751 void
3752 ideintr(void)
3753 {
3754   struct buf *b;
3755 
3756   // First queued buffer is the active request.
3757   acquire(&idelock);
3758   if((b = idequeue) == 0){
3759     release(&idelock);
3760     // cprintf("spurious IDE interrupt\n");
3761     return;
3762   }
3763   idequeue = b->qnext;
3764 
3765   // Read data if needed.
3766   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
3767     insl(0x1f0, b->data, BSIZE/4);
3768 
3769   // Wake process waiting for this buf.
3770   b->flags |= B_VALID;
3771   b->flags &= ~B_DIRTY;
3772   wakeup(b);
3773 
3774   // Start disk on next buf in queue.
3775   if(idequeue != 0)
3776     idestart(idequeue);
3777 
3778   release(&idelock);
3779 }
3780 
3781 // Sync buf with disk.
3782 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
3783 // Else if B_VALID is not set, read buf from disk, set B_VALID.
3784 void
3785 iderw(struct buf *b)
3786 {
3787   struct buf **pp;
3788 
3789   if(!(b->flags & B_BUSY))
3790     panic("iderw: buf not busy");
3791   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
3792     panic("iderw: nothing to do");
3793   if(b->dev != 0 && !havedisk1)
3794     panic("iderw: ide disk 1 not present");
3795 
3796   acquire(&idelock);  //DOC:acquire-lock
3797 
3798 
3799 
3800   // Append b to idequeue.
3801   b->qnext = 0;
3802   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
3803     ;
3804   *pp = b;
3805 
3806   // Start disk if necessary.
3807   if(idequeue == b)
3808     idestart(b);
3809 
3810   // Wait for request to finish.
3811   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
3812     sleep(b, &idelock);
3813   }
3814 
3815   release(&idelock);
3816 }
3817 
3818 
3819 
3820 
3821 
3822 
3823 
3824 
3825 
3826 
3827 
3828 
3829 
3830 
3831 
3832 
3833 
3834 
3835 
3836 
3837 
3838 
3839 
3840 
3841 
3842 
3843 
3844 
3845 
3846 
3847 
3848 
3849 
