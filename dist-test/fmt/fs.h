3500 // On-disk file system format.
3501 // Both the kernel and user programs use this header file.
3502 
3503 
3504 #define ROOTINO 1  // root i-number
3505 #define BSIZE 512  // block size
3506 
3507 // Disk layout:
3508 // [ boot block | super block | log | inode blocks | free bit map | data blocks ]
3509 //
3510 // mkfs computes the super block and builds an initial file system. The super describes
3511 // the disk layout:
3512 struct superblock {
3513   uint size;         // Size of file system image (blocks)
3514   uint nblocks;      // Number of data blocks
3515   uint ninodes;      // Number of inodes.
3516   uint nlog;         // Number of log blocks
3517   uint logstart;     // Block number of first log block
3518   uint inodestart;   // Block number of first inode block
3519   uint bmapstart;    // Block number of first free map block
3520 };
3521 
3522 #define NDIRECT 12
3523 #define NINDIRECT (BSIZE / sizeof(uint))
3524 #define MAXFILE (NDIRECT + NINDIRECT)
3525 
3526 // On-disk inode structure
3527 struct dinode {
3528   short type;           // File type
3529   short major;          // Major device number (T_DEV only)
3530   short minor;          // Minor device number (T_DEV only)
3531   short nlink;          // Number of links to inode in file system
3532   uint size;            // Size of file (bytes)
3533   uint addrs[NDIRECT+1];   // Data block addresses
3534 };
3535 
3536 
3537 
3538 
3539 
3540 
3541 
3542 
3543 
3544 
3545 
3546 
3547 
3548 
3549 
3550 // Inodes per block.
3551 #define IPB           (BSIZE / sizeof(struct dinode))
3552 
3553 // Block containing inode i
3554 #define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)
3555 
3556 // Bitmap bits per block
3557 #define BPB           (BSIZE*8)
3558 
3559 // Block of free map containing bit for block b
3560 #define BBLOCK(b, sb) (b/BPB + sb.bmapstart)
3561 
3562 // Directory is a file containing a sequence of dirent structures.
3563 #define DIRSIZ 14
3564 
3565 struct dirent {
3566   ushort inum;
3567   char name[DIRSIZ];
3568 };
3569 
3570 
3571 
3572 
3573 
3574 
3575 
3576 
3577 
3578 
3579 
3580 
3581 
3582 
3583 
3584 
3585 
3586 
3587 
3588 
3589 
3590 
3591 
3592 
3593 
3594 
3595 
3596 
3597 
3598 
3599 
