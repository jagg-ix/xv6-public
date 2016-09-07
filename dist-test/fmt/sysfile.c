5150 //
5151 // File-system system calls.
5152 // Mostly argument checking, since we don't trust
5153 // user code, and calls into file.c and fs.c.
5154 //
5155 
5156 #include "types.h"
5157 #include "defs.h"
5158 #include "param.h"
5159 #include "stat.h"
5160 #include "mmu.h"
5161 #include "proc.h"
5162 #include "fs.h"
5163 #include "file.h"
5164 #include "fcntl.h"
5165 
5166 // Fetch the nth word-sized system call argument as a file descriptor
5167 // and return both the descriptor and the corresponding struct file.
5168 static int
5169 argfd(int n, int *pfd, struct file **pf)
5170 {
5171   int fd;
5172   struct file *f;
5173 
5174   if(argint(n, &fd) < 0)
5175     return -1;
5176   if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
5177     return -1;
5178   if(pfd)
5179     *pfd = fd;
5180   if(pf)
5181     *pf = f;
5182   return 0;
5183 }
5184 
5185 // Allocate a file descriptor for the given file.
5186 // Takes over file reference from caller on success.
5187 static int
5188 fdalloc(struct file *f)
5189 {
5190   int fd;
5191 
5192   for(fd = 0; fd < NOFILE; fd++){
5193     if(proc->ofile[fd] == 0){
5194       proc->ofile[fd] = f;
5195       return fd;
5196     }
5197   }
5198   return -1;
5199 }
5200 int
5201 sys_dup(void)
5202 {
5203   struct file *f;
5204   int fd;
5205 
5206   if(argfd(0, 0, &f) < 0)
5207     return -1;
5208   if((fd=fdalloc(f)) < 0)
5209     return -1;
5210   filedup(f);
5211   return fd;
5212 }
5213 
5214 int
5215 sys_read(void)
5216 {
5217   struct file *f;
5218   int n;
5219   char *p;
5220 
5221   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5222     return -1;
5223   return fileread(f, p, n);
5224 }
5225 
5226 int
5227 sys_write(void)
5228 {
5229   struct file *f;
5230   int n;
5231   char *p;
5232 
5233   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5234     return -1;
5235   return filewrite(f, p, n);
5236 }
5237 
5238 int
5239 sys_close(void)
5240 {
5241   int fd;
5242   struct file *f;
5243 
5244   if(argfd(0, &fd, &f) < 0)
5245     return -1;
5246   proc->ofile[fd] = 0;
5247   fileclose(f);
5248   return 0;
5249 }
5250 int
5251 sys_fstat(void)
5252 {
5253   struct file *f;
5254   struct stat *st;
5255 
5256   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
5257     return -1;
5258   return filestat(f, st);
5259 }
5260 
5261 // Create the path new as a link to the same inode as old.
5262 int
5263 sys_link(void)
5264 {
5265   char name[DIRSIZ], *new, *old;
5266   struct inode *dp, *ip;
5267 
5268   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
5269     return -1;
5270 
5271   begin_op();
5272   if((ip = namei(old)) == 0){
5273     end_op();
5274     return -1;
5275   }
5276 
5277   ilock(ip);
5278   if(ip->type == T_DIR){
5279     iunlockput(ip);
5280     end_op();
5281     return -1;
5282   }
5283 
5284   ip->nlink++;
5285   iupdate(ip);
5286   iunlock(ip);
5287 
5288   if((dp = nameiparent(new, name)) == 0)
5289     goto bad;
5290   ilock(dp);
5291   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
5292     iunlockput(dp);
5293     goto bad;
5294   }
5295   iunlockput(dp);
5296   iput(ip);
5297 
5298   end_op();
5299 
5300   return 0;
5301 
5302 bad:
5303   ilock(ip);
5304   ip->nlink--;
5305   iupdate(ip);
5306   iunlockput(ip);
5307   end_op();
5308   return -1;
5309 }
5310 
5311 // Is the directory dp empty except for "." and ".." ?
5312 static int
5313 isdirempty(struct inode *dp)
5314 {
5315   int off;
5316   struct dirent de;
5317 
5318   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
5319     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5320       panic("isdirempty: readi");
5321     if(de.inum != 0)
5322       return 0;
5323   }
5324   return 1;
5325 }
5326 
5327 int
5328 sys_unlink(void)
5329 {
5330   struct inode *ip, *dp;
5331   struct dirent de;
5332   char name[DIRSIZ], *path;
5333   uint off;
5334 
5335   if(argstr(0, &path) < 0)
5336     return -1;
5337 
5338   begin_op();
5339   if((dp = nameiparent(path, name)) == 0){
5340     end_op();
5341     return -1;
5342   }
5343 
5344   ilock(dp);
5345 
5346   // Cannot unlink "." or "..".
5347   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
5348     goto bad;
5349 
5350   if((ip = dirlookup(dp, name, &off)) == 0)
5351     goto bad;
5352   ilock(ip);
5353 
5354   if(ip->nlink < 1)
5355     panic("unlink: nlink < 1");
5356   if(ip->type == T_DIR && !isdirempty(ip)){
5357     iunlockput(ip);
5358     goto bad;
5359   }
5360 
5361   memset(&de, 0, sizeof(de));
5362   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5363     panic("unlink: writei");
5364   if(ip->type == T_DIR){
5365     dp->nlink--;
5366     iupdate(dp);
5367   }
5368   iunlockput(dp);
5369 
5370   ip->nlink--;
5371   iupdate(ip);
5372   iunlockput(ip);
5373 
5374   end_op();
5375 
5376   return 0;
5377 
5378 bad:
5379   iunlockput(dp);
5380   end_op();
5381   return -1;
5382 }
5383 
5384 static struct inode*
5385 create(char *path, short type, short major, short minor)
5386 {
5387   uint off;
5388   struct inode *ip, *dp;
5389   char name[DIRSIZ];
5390 
5391   if((dp = nameiparent(path, name)) == 0)
5392     return 0;
5393   ilock(dp);
5394 
5395   if((ip = dirlookup(dp, name, &off)) != 0){
5396     iunlockput(dp);
5397     ilock(ip);
5398     if(type == T_FILE && ip->type == T_FILE)
5399       return ip;
5400     iunlockput(ip);
5401     return 0;
5402   }
5403 
5404   if((ip = ialloc(dp->dev, type)) == 0)
5405     panic("create: ialloc");
5406 
5407   ilock(ip);
5408   ip->major = major;
5409   ip->minor = minor;
5410   ip->nlink = 1;
5411   iupdate(ip);
5412 
5413   if(type == T_DIR){  // Create . and .. entries.
5414     dp->nlink++;  // for ".."
5415     iupdate(dp);
5416     // No ip->nlink++ for ".": avoid cyclic ref count.
5417     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
5418       panic("create dots");
5419   }
5420 
5421   if(dirlink(dp, name, ip->inum) < 0)
5422     panic("create: dirlink");
5423 
5424   iunlockput(dp);
5425 
5426   return ip;
5427 }
5428 
5429 int
5430 sys_open(void)
5431 {
5432   char *path;
5433   int fd, omode;
5434   struct file *f;
5435   struct inode *ip;
5436 
5437   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
5438     return -1;
5439 
5440   begin_op();
5441 
5442   if(omode & O_CREATE){
5443     ip = create(path, T_FILE, 0, 0);
5444     if(ip == 0){
5445       end_op();
5446       return -1;
5447     }
5448   } else {
5449     if((ip = namei(path)) == 0){
5450       end_op();
5451       return -1;
5452     }
5453     ilock(ip);
5454     if(ip->type == T_DIR && omode != O_RDONLY){
5455       iunlockput(ip);
5456       end_op();
5457       return -1;
5458     }
5459   }
5460 
5461   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
5462     if(f)
5463       fileclose(f);
5464     iunlockput(ip);
5465     end_op();
5466     return -1;
5467   }
5468   iunlock(ip);
5469   end_op();
5470 
5471   f->type = FD_INODE;
5472   f->ip = ip;
5473   f->off = 0;
5474   f->readable = !(omode & O_WRONLY);
5475   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
5476   return fd;
5477 }
5478 
5479 int
5480 sys_mkdir(void)
5481 {
5482   char *path;
5483   struct inode *ip;
5484 
5485   begin_op();
5486   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
5487     end_op();
5488     return -1;
5489   }
5490   iunlockput(ip);
5491   end_op();
5492   return 0;
5493 }
5494 
5495 
5496 
5497 
5498 
5499 
5500 int
5501 sys_mknod(void)
5502 {
5503   struct inode *ip;
5504   char *path;
5505   int len;
5506   int major, minor;
5507 
5508   begin_op();
5509   if((len=argstr(0, &path)) < 0 ||
5510      argint(1, &major) < 0 ||
5511      argint(2, &minor) < 0 ||
5512      (ip = create(path, T_DEV, major, minor)) == 0){
5513     end_op();
5514     return -1;
5515   }
5516   iunlockput(ip);
5517   end_op();
5518   return 0;
5519 }
5520 
5521 int
5522 sys_chdir(void)
5523 {
5524   char *path;
5525   struct inode *ip;
5526 
5527   begin_op();
5528   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
5529     end_op();
5530     return -1;
5531   }
5532   ilock(ip);
5533   if(ip->type != T_DIR){
5534     iunlockput(ip);
5535     end_op();
5536     return -1;
5537   }
5538   iunlock(ip);
5539   iput(proc->cwd);
5540   end_op();
5541   proc->cwd = ip;
5542   return 0;
5543 }
5544 
5545 
5546 
5547 
5548 
5549 
5550 int
5551 sys_exec(void)
5552 {
5553   char *path, *argv[MAXARG];
5554   int i;
5555   uint uargv, uarg;
5556 
5557   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
5558     return -1;
5559   }
5560   memset(argv, 0, sizeof(argv));
5561   for(i=0;; i++){
5562     if(i >= NELEM(argv))
5563       return -1;
5564     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
5565       return -1;
5566     if(uarg == 0){
5567       argv[i] = 0;
5568       break;
5569     }
5570     if(fetchstr(uarg, &argv[i]) < 0)
5571       return -1;
5572   }
5573   return exec(path, argv);
5574 }
5575 
5576 int
5577 sys_pipe(void)
5578 {
5579   int *fd;
5580   struct file *rf, *wf;
5581   int fd0, fd1;
5582 
5583   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
5584     return -1;
5585   if(pipealloc(&rf, &wf) < 0)
5586     return -1;
5587   fd0 = -1;
5588   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
5589     if(fd0 >= 0)
5590       proc->ofile[fd0] = 0;
5591     fileclose(rf);
5592     fileclose(wf);
5593     return -1;
5594   }
5595   fd[0] = fd0;
5596   fd[1] = fd1;
5597   return 0;
5598 }
5599 
