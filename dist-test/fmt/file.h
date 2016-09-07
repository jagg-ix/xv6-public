3600 struct file {
3601   enum { FD_NONE, FD_PIPE, FD_INODE } type;
3602   int ref; // reference count
3603   char readable;
3604   char writable;
3605   struct pipe *pipe;
3606   struct inode *ip;
3607   uint off;
3608 };
3609 
3610 
3611 // in-memory copy of an inode
3612 struct inode {
3613   uint dev;           // Device number
3614   uint inum;          // Inode number
3615   int ref;            // Reference count
3616   int flags;          // I_BUSY, I_VALID
3617 
3618   short type;         // copy of disk inode
3619   short major;
3620   short minor;
3621   short nlink;
3622   uint size;
3623   uint addrs[NDIRECT+1];
3624 };
3625 #define I_BUSY 0x1
3626 #define I_VALID 0x2
3627 
3628 // table mapping major device number to
3629 // device functions
3630 struct devsw {
3631   int (*read)(struct inode*, char*, int);
3632   int (*write)(struct inode*, char*, int);
3633 };
3634 
3635 extern struct devsw devsw[];
3636 
3637 #define CONSOLE 1
3638 
3639 // Blank page.
3640 
3641 
3642 
3643 
3644 
3645 
3646 
3647 
3648 
3649 
