3850 // Buffer cache.
3851 //
3852 // The buffer cache is a linked list of buf structures holding
3853 // cached copies of disk block contents.  Caching disk blocks
3854 // in memory reduces the number of disk reads and also provides
3855 // a synchronization point for disk blocks used by multiple processes.
3856 //
3857 // Interface:
3858 // * To get a buffer for a particular disk block, call bread.
3859 // * After changing buffer data, call bwrite to write it to disk.
3860 // * When done with the buffer, call brelse.
3861 // * Do not use the buffer after calling brelse.
3862 // * Only one process at a time can use a buffer,
3863 //     so do not keep them longer than necessary.
3864 //
3865 // The implementation uses three state flags internally:
3866 // * B_BUSY: the block has been returned from bread
3867 //     and has not been passed back to brelse.
3868 // * B_VALID: the buffer data has been read from the disk.
3869 // * B_DIRTY: the buffer data has been modified
3870 //     and needs to be written to disk.
3871 
3872 #include "types.h"
3873 #include "defs.h"
3874 #include "param.h"
3875 #include "spinlock.h"
3876 #include "fs.h"
3877 #include "buf.h"
3878 
3879 struct {
3880   struct spinlock lock;
3881   struct buf buf[NBUF];
3882 
3883   // Linked list of all buffers, through prev/next.
3884   // head.next is most recently used.
3885   struct buf head;
3886 } bcache;
3887 
3888 void
3889 binit(void)
3890 {
3891   struct buf *b;
3892 
3893   initlock(&bcache.lock, "bcache");
3894 
3895   // Create linked list of buffers
3896   bcache.head.prev = &bcache.head;
3897   bcache.head.next = &bcache.head;
3898   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
3899     b->next = bcache.head.next;
3900     b->prev = &bcache.head;
3901     b->dev = -1;
3902     bcache.head.next->prev = b;
3903     bcache.head.next = b;
3904   }
3905 }
3906 
3907 // Look through buffer cache for block on device dev.
3908 // If not found, allocate a buffer.
3909 // In either case, return B_BUSY buffer.
3910 static struct buf*
3911 bget(uint dev, uint blockno)
3912 {
3913   struct buf *b;
3914 
3915   acquire(&bcache.lock);
3916 
3917  loop:
3918   // Is the block already cached?
3919   for(b = bcache.head.next; b != &bcache.head; b = b->next){
3920     if(b->dev == dev && b->blockno == blockno){
3921       if(!(b->flags & B_BUSY)){
3922         b->flags |= B_BUSY;
3923         release(&bcache.lock);
3924         return b;
3925       }
3926       sleep(b, &bcache.lock);
3927       goto loop;
3928     }
3929   }
3930 
3931   // Not cached; recycle some non-busy and clean buffer.
3932   // "clean" because B_DIRTY and !B_BUSY means log.c
3933   // hasn't yet committed the changes to the buffer.
3934   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
3935     if((b->flags & B_BUSY) == 0 && (b->flags & B_DIRTY) == 0){
3936       b->dev = dev;
3937       b->blockno = blockno;
3938       b->flags = B_BUSY;
3939       release(&bcache.lock);
3940       return b;
3941     }
3942   }
3943   panic("bget: no buffers");
3944 }
3945 
3946 
3947 
3948 
3949 
3950 // Return a B_BUSY buf with the contents of the indicated block.
3951 struct buf*
3952 bread(uint dev, uint blockno)
3953 {
3954   struct buf *b;
3955 
3956   b = bget(dev, blockno);
3957   if(!(b->flags & B_VALID)) {
3958     iderw(b);
3959   }
3960   return b;
3961 }
3962 
3963 // Write b's contents to disk.  Must be B_BUSY.
3964 void
3965 bwrite(struct buf *b)
3966 {
3967   if((b->flags & B_BUSY) == 0)
3968     panic("bwrite");
3969   b->flags |= B_DIRTY;
3970   iderw(b);
3971 }
3972 
3973 // Release a B_BUSY buffer.
3974 // Move to the head of the MRU list.
3975 void
3976 brelse(struct buf *b)
3977 {
3978   if((b->flags & B_BUSY) == 0)
3979     panic("brelse");
3980 
3981   acquire(&bcache.lock);
3982 
3983   b->next->prev = b->prev;
3984   b->prev->next = b->next;
3985   b->next = bcache.head.next;
3986   b->prev = &bcache.head;
3987   bcache.head.next->prev = b;
3988   bcache.head.next = b;
3989 
3990   b->flags &= ~B_BUSY;
3991   wakeup(b);
3992 
3993   release(&bcache.lock);
3994 }
3995 // Blank page.
3996 
3997 
3998 
3999 
