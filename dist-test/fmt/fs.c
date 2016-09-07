4250 // File system implementation.  Five layers:
4251 //   + Blocks: allocator for raw disk blocks.
4252 //   + Log: crash recovery for multi-step updates.
4253 //   + Files: inode allocator, reading, writing, metadata.
4254 //   + Directories: inode with special contents (list of other inodes!)
4255 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
4256 //
4257 // This file contains the low-level file system manipulation
4258 // routines.  The (higher-level) system call implementations
4259 // are in sysfile.c.
4260 
4261 #include "types.h"
4262 #include "defs.h"
4263 #include "param.h"
4264 #include "stat.h"
4265 #include "mmu.h"
4266 #include "proc.h"
4267 #include "spinlock.h"
4268 #include "fs.h"
4269 #include "buf.h"
4270 #include "file.h"
4271 
4272 #define min(a, b) ((a) < (b) ? (a) : (b))
4273 static void itrunc(struct inode*);
4274 struct superblock sb;   // there should be one per dev, but we run with one dev
4275 
4276 // Read the super block.
4277 void
4278 readsb(int dev, struct superblock *sb)
4279 {
4280   struct buf *bp;
4281 
4282   bp = bread(dev, 1);
4283   memmove(sb, bp->data, sizeof(*sb));
4284   brelse(bp);
4285 }
4286 
4287 // Zero a block.
4288 static void
4289 bzero(int dev, int bno)
4290 {
4291   struct buf *bp;
4292 
4293   bp = bread(dev, bno);
4294   memset(bp->data, 0, BSIZE);
4295   log_write(bp);
4296   brelse(bp);
4297 }
4298 
4299 
4300 // Blocks.
4301 
4302 // Allocate a zeroed disk block.
4303 static uint
4304 balloc(uint dev)
4305 {
4306   int b, bi, m;
4307   struct buf *bp;
4308 
4309   bp = 0;
4310   for(b = 0; b < sb.size; b += BPB){
4311     bp = bread(dev, BBLOCK(b, sb));
4312     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
4313       m = 1 << (bi % 8);
4314       if((bp->data[bi/8] & m) == 0){  // Is block free?
4315         bp->data[bi/8] |= m;  // Mark block in use.
4316         log_write(bp);
4317         brelse(bp);
4318         bzero(dev, b + bi);
4319         return b + bi;
4320       }
4321     }
4322     brelse(bp);
4323   }
4324   panic("balloc: out of blocks");
4325 }
4326 
4327 // Free a disk block.
4328 static void
4329 bfree(int dev, uint b)
4330 {
4331   struct buf *bp;
4332   int bi, m;
4333 
4334   readsb(dev, &sb);
4335   bp = bread(dev, BBLOCK(b, sb));
4336   bi = b % BPB;
4337   m = 1 << (bi % 8);
4338   if((bp->data[bi/8] & m) == 0)
4339     panic("freeing free block");
4340   bp->data[bi/8] &= ~m;
4341   log_write(bp);
4342   brelse(bp);
4343 }
4344 
4345 
4346 
4347 
4348 
4349 
4350 // Inodes.
4351 //
4352 // An inode describes a single unnamed file.
4353 // The inode disk structure holds metadata: the file's type,
4354 // its size, the number of links referring to it, and the
4355 // list of blocks holding the file's content.
4356 //
4357 // The inodes are laid out sequentially on disk at
4358 // sb.startinode. Each inode has a number, indicating its
4359 // position on the disk.
4360 //
4361 // The kernel keeps a cache of in-use inodes in memory
4362 // to provide a place for synchronizing access
4363 // to inodes used by multiple processes. The cached
4364 // inodes include book-keeping information that is
4365 // not stored on disk: ip->ref and ip->flags.
4366 //
4367 // An inode and its in-memory represtative go through a
4368 // sequence of states before they can be used by the
4369 // rest of the file system code.
4370 //
4371 // * Allocation: an inode is allocated if its type (on disk)
4372 //   is non-zero. ialloc() allocates, iput() frees if
4373 //   the link count has fallen to zero.
4374 //
4375 // * Referencing in cache: an entry in the inode cache
4376 //   is free if ip->ref is zero. Otherwise ip->ref tracks
4377 //   the number of in-memory pointers to the entry (open
4378 //   files and current directories). iget() to find or
4379 //   create a cache entry and increment its ref, iput()
4380 //   to decrement ref.
4381 //
4382 // * Valid: the information (type, size, &c) in an inode
4383 //   cache entry is only correct when the I_VALID bit
4384 //   is set in ip->flags. ilock() reads the inode from
4385 //   the disk and sets I_VALID, while iput() clears
4386 //   I_VALID if ip->ref has fallen to zero.
4387 //
4388 // * Locked: file system code may only examine and modify
4389 //   the information in an inode and its content if it
4390 //   has first locked the inode. The I_BUSY flag indicates
4391 //   that the inode is locked. ilock() sets I_BUSY,
4392 //   while iunlock clears it.
4393 //
4394 // Thus a typical sequence is:
4395 //   ip = iget(dev, inum)
4396 //   ilock(ip)
4397 //   ... examine and modify ip->xxx ...
4398 //   iunlock(ip)
4399 //   iput(ip)
4400 //
4401 // ilock() is separate from iget() so that system calls can
4402 // get a long-term reference to an inode (as for an open file)
4403 // and only lock it for short periods (e.g., in read()).
4404 // The separation also helps avoid deadlock and races during
4405 // pathname lookup. iget() increments ip->ref so that the inode
4406 // stays cached and pointers to it remain valid.
4407 //
4408 // Many internal file system functions expect the caller to
4409 // have locked the inodes involved; this lets callers create
4410 // multi-step atomic operations.
4411 
4412 struct {
4413   struct spinlock lock;
4414   struct inode inode[NINODE];
4415 } icache;
4416 
4417 void
4418 iinit(int dev)
4419 {
4420   initlock(&icache.lock, "icache");
4421   readsb(dev, &sb);
4422   cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d inodestart %d bmap start %d\n", sb.size,
4423           sb.nblocks, sb.ninodes, sb.nlog, sb.logstart, sb.inodestart, sb.bmapstart);
4424 }
4425 
4426 static struct inode* iget(uint dev, uint inum);
4427 
4428 // Allocate a new inode with the given type on device dev.
4429 // A free inode has a type of zero.
4430 struct inode*
4431 ialloc(uint dev, short type)
4432 {
4433   int inum;
4434   struct buf *bp;
4435   struct dinode *dip;
4436 
4437   for(inum = 1; inum < sb.ninodes; inum++){
4438     bp = bread(dev, IBLOCK(inum, sb));
4439     dip = (struct dinode*)bp->data + inum%IPB;
4440     if(dip->type == 0){  // a free inode
4441       memset(dip, 0, sizeof(*dip));
4442       dip->type = type;
4443       log_write(bp);   // mark it allocated on the disk
4444       brelse(bp);
4445       return iget(dev, inum);
4446     }
4447     brelse(bp);
4448   }
4449   panic("ialloc: no inodes");
4450 }
4451 
4452 // Copy a modified in-memory inode to disk.
4453 void
4454 iupdate(struct inode *ip)
4455 {
4456   struct buf *bp;
4457   struct dinode *dip;
4458 
4459   bp = bread(ip->dev, IBLOCK(ip->inum, sb));
4460   dip = (struct dinode*)bp->data + ip->inum%IPB;
4461   dip->type = ip->type;
4462   dip->major = ip->major;
4463   dip->minor = ip->minor;
4464   dip->nlink = ip->nlink;
4465   dip->size = ip->size;
4466   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
4467   log_write(bp);
4468   brelse(bp);
4469 }
4470 
4471 // Find the inode with number inum on device dev
4472 // and return the in-memory copy. Does not lock
4473 // the inode and does not read it from disk.
4474 static struct inode*
4475 iget(uint dev, uint inum)
4476 {
4477   struct inode *ip, *empty;
4478 
4479   acquire(&icache.lock);
4480 
4481   // Is the inode already cached?
4482   empty = 0;
4483   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
4484     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
4485       ip->ref++;
4486       release(&icache.lock);
4487       return ip;
4488     }
4489     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
4490       empty = ip;
4491   }
4492 
4493   // Recycle an inode cache entry.
4494   if(empty == 0)
4495     panic("iget: no inodes");
4496 
4497 
4498 
4499 
4500   ip = empty;
4501   ip->dev = dev;
4502   ip->inum = inum;
4503   ip->ref = 1;
4504   ip->flags = 0;
4505   release(&icache.lock);
4506 
4507   return ip;
4508 }
4509 
4510 // Increment reference count for ip.
4511 // Returns ip to enable ip = idup(ip1) idiom.
4512 struct inode*
4513 idup(struct inode *ip)
4514 {
4515   acquire(&icache.lock);
4516   ip->ref++;
4517   release(&icache.lock);
4518   return ip;
4519 }
4520 
4521 // Lock the given inode.
4522 // Reads the inode from disk if necessary.
4523 void
4524 ilock(struct inode *ip)
4525 {
4526   struct buf *bp;
4527   struct dinode *dip;
4528 
4529   if(ip == 0 || ip->ref < 1)
4530     panic("ilock");
4531 
4532   acquire(&icache.lock);
4533   while(ip->flags & I_BUSY)
4534     sleep(ip, &icache.lock);
4535   ip->flags |= I_BUSY;
4536   release(&icache.lock);
4537 
4538   if(!(ip->flags & I_VALID)){
4539     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
4540     dip = (struct dinode*)bp->data + ip->inum%IPB;
4541     ip->type = dip->type;
4542     ip->major = dip->major;
4543     ip->minor = dip->minor;
4544     ip->nlink = dip->nlink;
4545     ip->size = dip->size;
4546     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
4547     brelse(bp);
4548     ip->flags |= I_VALID;
4549     if(ip->type == 0)
4550       panic("ilock: no type");
4551   }
4552 }
4553 
4554 // Unlock the given inode.
4555 void
4556 iunlock(struct inode *ip)
4557 {
4558   if(ip == 0 || !(ip->flags & I_BUSY) || ip->ref < 1)
4559     panic("iunlock");
4560 
4561   acquire(&icache.lock);
4562   ip->flags &= ~I_BUSY;
4563   wakeup(ip);
4564   release(&icache.lock);
4565 }
4566 
4567 // Drop a reference to an in-memory inode.
4568 // If that was the last reference, the inode cache entry can
4569 // be recycled.
4570 // If that was the last reference and the inode has no links
4571 // to it, free the inode (and its content) on disk.
4572 // All calls to iput() must be inside a transaction in
4573 // case it has to free the inode.
4574 void
4575 iput(struct inode *ip)
4576 {
4577   acquire(&icache.lock);
4578   if(ip->ref == 1 && (ip->flags & I_VALID) && ip->nlink == 0){
4579     // inode has no links and no other references: truncate and free.
4580     if(ip->flags & I_BUSY)
4581       panic("iput busy");
4582     ip->flags |= I_BUSY;
4583     release(&icache.lock);
4584     itrunc(ip);
4585     ip->type = 0;
4586     iupdate(ip);
4587     acquire(&icache.lock);
4588     ip->flags = 0;
4589     wakeup(ip);
4590   }
4591   ip->ref--;
4592   release(&icache.lock);
4593 }
4594 
4595 
4596 
4597 
4598 
4599 
4600 // Common idiom: unlock, then put.
4601 void
4602 iunlockput(struct inode *ip)
4603 {
4604   iunlock(ip);
4605   iput(ip);
4606 }
4607 
4608 // Inode content
4609 //
4610 // The content (data) associated with each inode is stored
4611 // in blocks on the disk. The first NDIRECT block numbers
4612 // are listed in ip->addrs[].  The next NINDIRECT blocks are
4613 // listed in block ip->addrs[NDIRECT].
4614 
4615 // Return the disk block address of the nth block in inode ip.
4616 // If there is no such block, bmap allocates one.
4617 static uint
4618 bmap(struct inode *ip, uint bn)
4619 {
4620   uint addr, *a;
4621   struct buf *bp;
4622 
4623   if(bn < NDIRECT){
4624     if((addr = ip->addrs[bn]) == 0)
4625       ip->addrs[bn] = addr = balloc(ip->dev);
4626     return addr;
4627   }
4628   bn -= NDIRECT;
4629 
4630   if(bn < NINDIRECT){
4631     // Load indirect block, allocating if necessary.
4632     if((addr = ip->addrs[NDIRECT]) == 0)
4633       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
4634     bp = bread(ip->dev, addr);
4635     a = (uint*)bp->data;
4636     if((addr = a[bn]) == 0){
4637       a[bn] = addr = balloc(ip->dev);
4638       log_write(bp);
4639     }
4640     brelse(bp);
4641     return addr;
4642   }
4643 
4644   panic("bmap: out of range");
4645 }
4646 
4647 
4648 
4649 
4650 // Truncate inode (discard contents).
4651 // Only called when the inode has no links
4652 // to it (no directory entries referring to it)
4653 // and has no in-memory reference to it (is
4654 // not an open file or current directory).
4655 static void
4656 itrunc(struct inode *ip)
4657 {
4658   int i, j;
4659   struct buf *bp;
4660   uint *a;
4661 
4662   for(i = 0; i < NDIRECT; i++){
4663     if(ip->addrs[i]){
4664       bfree(ip->dev, ip->addrs[i]);
4665       ip->addrs[i] = 0;
4666     }
4667   }
4668 
4669   if(ip->addrs[NDIRECT]){
4670     bp = bread(ip->dev, ip->addrs[NDIRECT]);
4671     a = (uint*)bp->data;
4672     for(j = 0; j < NINDIRECT; j++){
4673       if(a[j])
4674         bfree(ip->dev, a[j]);
4675     }
4676     brelse(bp);
4677     bfree(ip->dev, ip->addrs[NDIRECT]);
4678     ip->addrs[NDIRECT] = 0;
4679   }
4680 
4681   ip->size = 0;
4682   iupdate(ip);
4683 }
4684 
4685 // Copy stat information from inode.
4686 void
4687 stati(struct inode *ip, struct stat *st)
4688 {
4689   st->dev = ip->dev;
4690   st->ino = ip->inum;
4691   st->type = ip->type;
4692   st->nlink = ip->nlink;
4693   st->size = ip->size;
4694 }
4695 
4696 
4697 
4698 
4699 
4700 // Read data from inode.
4701 int
4702 readi(struct inode *ip, char *dst, uint off, uint n)
4703 {
4704   uint tot, m;
4705   struct buf *bp;
4706 
4707   if(ip->type == T_DEV){
4708     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
4709       return -1;
4710     return devsw[ip->major].read(ip, dst, n);
4711   }
4712 
4713   if(off > ip->size || off + n < off)
4714     return -1;
4715   if(off + n > ip->size)
4716     n = ip->size - off;
4717 
4718   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
4719     bp = bread(ip->dev, bmap(ip, off/BSIZE));
4720     m = min(n - tot, BSIZE - off%BSIZE);
4721     memmove(dst, bp->data + off%BSIZE, m);
4722     brelse(bp);
4723   }
4724   return n;
4725 }
4726 
4727 // Write data to inode.
4728 int
4729 writei(struct inode *ip, char *src, uint off, uint n)
4730 {
4731   uint tot, m;
4732   struct buf *bp;
4733 
4734   if(ip->type == T_DEV){
4735     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
4736       return -1;
4737     return devsw[ip->major].write(ip, src, n);
4738   }
4739 
4740   if(off > ip->size || off + n < off)
4741     return -1;
4742   if(off + n > MAXFILE*BSIZE)
4743     return -1;
4744 
4745   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
4746     bp = bread(ip->dev, bmap(ip, off/BSIZE));
4747     m = min(n - tot, BSIZE - off%BSIZE);
4748     memmove(bp->data + off%BSIZE, src, m);
4749     log_write(bp);
4750     brelse(bp);
4751   }
4752 
4753   if(n > 0 && off > ip->size){
4754     ip->size = off;
4755     iupdate(ip);
4756   }
4757   return n;
4758 }
4759 
4760 // Directories
4761 
4762 int
4763 namecmp(const char *s, const char *t)
4764 {
4765   return strncmp(s, t, DIRSIZ);
4766 }
4767 
4768 // Look for a directory entry in a directory.
4769 // If found, set *poff to byte offset of entry.
4770 struct inode*
4771 dirlookup(struct inode *dp, char *name, uint *poff)
4772 {
4773   uint off, inum;
4774   struct dirent de;
4775 
4776   if(dp->type != T_DIR)
4777     panic("dirlookup not DIR");
4778 
4779   for(off = 0; off < dp->size; off += sizeof(de)){
4780     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
4781       panic("dirlink read");
4782     if(de.inum == 0)
4783       continue;
4784     if(namecmp(name, de.name) == 0){
4785       // entry matches path element
4786       if(poff)
4787         *poff = off;
4788       inum = de.inum;
4789       return iget(dp->dev, inum);
4790     }
4791   }
4792 
4793   return 0;
4794 }
4795 
4796 
4797 
4798 
4799 
4800 // Write a new directory entry (name, inum) into the directory dp.
4801 int
4802 dirlink(struct inode *dp, char *name, uint inum)
4803 {
4804   int off;
4805   struct dirent de;
4806   struct inode *ip;
4807 
4808   // Check that name is not present.
4809   if((ip = dirlookup(dp, name, 0)) != 0){
4810     iput(ip);
4811     return -1;
4812   }
4813 
4814   // Look for an empty dirent.
4815   for(off = 0; off < dp->size; off += sizeof(de)){
4816     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
4817       panic("dirlink read");
4818     if(de.inum == 0)
4819       break;
4820   }
4821 
4822   strncpy(de.name, name, DIRSIZ);
4823   de.inum = inum;
4824   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
4825     panic("dirlink");
4826 
4827   return 0;
4828 }
4829 
4830 // Paths
4831 
4832 // Copy the next path element from path into name.
4833 // Return a pointer to the element following the copied one.
4834 // The returned path has no leading slashes,
4835 // so the caller can check *path=='\0' to see if the name is the last one.
4836 // If no name to remove, return 0.
4837 //
4838 // Examples:
4839 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
4840 //   skipelem("///a//bb", name) = "bb", setting name = "a"
4841 //   skipelem("a", name) = "", setting name = "a"
4842 //   skipelem("", name) = skipelem("////", name) = 0
4843 //
4844 static char*
4845 skipelem(char *path, char *name)
4846 {
4847   char *s;
4848   int len;
4849 
4850   while(*path == '/')
4851     path++;
4852   if(*path == 0)
4853     return 0;
4854   s = path;
4855   while(*path != '/' && *path != 0)
4856     path++;
4857   len = path - s;
4858   if(len >= DIRSIZ)
4859     memmove(name, s, DIRSIZ);
4860   else {
4861     memmove(name, s, len);
4862     name[len] = 0;
4863   }
4864   while(*path == '/')
4865     path++;
4866   return path;
4867 }
4868 
4869 // Look up and return the inode for a path name.
4870 // If parent != 0, return the inode for the parent and copy the final
4871 // path element into name, which must have room for DIRSIZ bytes.
4872 // Must be called inside a transaction since it calls iput().
4873 static struct inode*
4874 namex(char *path, int nameiparent, char *name)
4875 {
4876   struct inode *ip, *next;
4877 
4878   if(*path == '/')
4879     ip = iget(ROOTDEV, ROOTINO);
4880   else
4881     ip = idup(proc->cwd);
4882 
4883   while((path = skipelem(path, name)) != 0){
4884     ilock(ip);
4885     if(ip->type != T_DIR){
4886       iunlockput(ip);
4887       return 0;
4888     }
4889     if(nameiparent && *path == '\0'){
4890       // Stop one level early.
4891       iunlock(ip);
4892       return ip;
4893     }
4894     if((next = dirlookup(ip, name, 0)) == 0){
4895       iunlockput(ip);
4896       return 0;
4897     }
4898     iunlockput(ip);
4899     ip = next;
4900   }
4901   if(nameiparent){
4902     iput(ip);
4903     return 0;
4904   }
4905   return ip;
4906 }
4907 
4908 struct inode*
4909 namei(char *path)
4910 {
4911   char name[DIRSIZ];
4912   return namex(path, 0, name);
4913 }
4914 
4915 struct inode*
4916 nameiparent(char *path, char *name)
4917 {
4918   return namex(path, 1, name);
4919 }
4920 
4921 
4922 
4923 
4924 
4925 
4926 
4927 
4928 
4929 
4930 
4931 
4932 
4933 
4934 
4935 
4936 
4937 
4938 
4939 
4940 
4941 
4942 
4943 
4944 
4945 
4946 
4947 
4948 
4949 
