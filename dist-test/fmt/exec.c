5600 #include "types.h"
5601 #include "param.h"
5602 #include "memlayout.h"
5603 #include "mmu.h"
5604 #include "proc.h"
5605 #include "defs.h"
5606 #include "x86.h"
5607 #include "elf.h"
5608 
5609 int
5610 exec(char *path, char **argv)
5611 {
5612   char *s, *last;
5613   int i, off;
5614   uint argc, sz, sp, ustack[3+MAXARG+1];
5615   struct elfhdr elf;
5616   struct inode *ip;
5617   struct proghdr ph;
5618   pde_t *pgdir, *oldpgdir;
5619 
5620   begin_op();
5621   if((ip = namei(path)) == 0){
5622     end_op();
5623     return -1;
5624   }
5625   ilock(ip);
5626   pgdir = 0;
5627 
5628   // Check ELF header
5629   if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
5630     goto bad;
5631   if(elf.magic != ELF_MAGIC)
5632     goto bad;
5633 
5634   if((pgdir = setupkvm()) == 0)
5635     goto bad;
5636 
5637   // Load program into memory.
5638   sz = 0;
5639   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
5640     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
5641       goto bad;
5642     if(ph.type != ELF_PROG_LOAD)
5643       continue;
5644     if(ph.memsz < ph.filesz)
5645       goto bad;
5646     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
5647       goto bad;
5648     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
5649       goto bad;
5650   }
5651   iunlockput(ip);
5652   end_op();
5653   ip = 0;
5654 
5655   // Allocate two pages at the next page boundary.
5656   // Make the first inaccessible.  Use the second as the user stack.
5657   sz = PGROUNDUP(sz);
5658   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
5659     goto bad;
5660   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
5661   sp = sz;
5662 
5663   // Push argument strings, prepare rest of stack in ustack.
5664   for(argc = 0; argv[argc]; argc++) {
5665     if(argc >= MAXARG)
5666       goto bad;
5667     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
5668     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
5669       goto bad;
5670     ustack[3+argc] = sp;
5671   }
5672   ustack[3+argc] = 0;
5673 
5674   ustack[0] = 0xffffffff;  // fake return PC
5675   ustack[1] = argc;
5676   ustack[2] = sp - (argc+1)*4;  // argv pointer
5677 
5678   sp -= (3+argc+1) * 4;
5679   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
5680     goto bad;
5681 
5682   // Save program name for debugging.
5683   for(last=s=path; *s; s++)
5684     if(*s == '/')
5685       last = s+1;
5686   safestrcpy(proc->name, last, sizeof(proc->name));
5687 
5688   // Commit to the user image.
5689   oldpgdir = proc->pgdir;
5690   proc->pgdir = pgdir;
5691   proc->sz = sz;
5692   proc->tf->eip = elf.entry;  // main
5693   proc->tf->esp = sp;
5694   switchuvm(proc);
5695   freevm(oldpgdir);
5696   return 0;
5697 
5698 
5699 
5700  bad:
5701   if(pgdir)
5702     freevm(pgdir);
5703   if(ip){
5704     iunlockput(ip);
5705     end_op();
5706   }
5707   return -1;
5708 }
5709 
5710 
5711 
5712 
5713 
5714 
5715 
5716 
5717 
5718 
5719 
5720 
5721 
5722 
5723 
5724 
5725 
5726 
5727 
5728 
5729 
5730 
5731 
5732 
5733 
5734 
5735 
5736 
5737 
5738 
5739 
5740 
5741 
5742 
5743 
5744 
5745 
5746 
5747 
5748 
5749 
