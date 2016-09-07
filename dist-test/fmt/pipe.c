5750 #include "types.h"
5751 #include "defs.h"
5752 #include "param.h"
5753 #include "mmu.h"
5754 #include "proc.h"
5755 #include "fs.h"
5756 #include "file.h"
5757 #include "spinlock.h"
5758 
5759 #define PIPESIZE 512
5760 
5761 struct pipe {
5762   struct spinlock lock;
5763   char data[PIPESIZE];
5764   uint nread;     // number of bytes read
5765   uint nwrite;    // number of bytes written
5766   int readopen;   // read fd is still open
5767   int writeopen;  // write fd is still open
5768 };
5769 
5770 int
5771 pipealloc(struct file **f0, struct file **f1)
5772 {
5773   struct pipe *p;
5774 
5775   p = 0;
5776   *f0 = *f1 = 0;
5777   if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
5778     goto bad;
5779   if((p = (struct pipe*)kalloc()) == 0)
5780     goto bad;
5781   p->readopen = 1;
5782   p->writeopen = 1;
5783   p->nwrite = 0;
5784   p->nread = 0;
5785   initlock(&p->lock, "pipe");
5786   (*f0)->type = FD_PIPE;
5787   (*f0)->readable = 1;
5788   (*f0)->writable = 0;
5789   (*f0)->pipe = p;
5790   (*f1)->type = FD_PIPE;
5791   (*f1)->readable = 0;
5792   (*f1)->writable = 1;
5793   (*f1)->pipe = p;
5794   return 0;
5795 
5796 
5797 
5798 
5799 
5800  bad:
5801   if(p)
5802     kfree((char*)p);
5803   if(*f0)
5804     fileclose(*f0);
5805   if(*f1)
5806     fileclose(*f1);
5807   return -1;
5808 }
5809 
5810 void
5811 pipeclose(struct pipe *p, int writable)
5812 {
5813   acquire(&p->lock);
5814   if(writable){
5815     p->writeopen = 0;
5816     wakeup(&p->nread);
5817   } else {
5818     p->readopen = 0;
5819     wakeup(&p->nwrite);
5820   }
5821   if(p->readopen == 0 && p->writeopen == 0){
5822     release(&p->lock);
5823     kfree((char*)p);
5824   } else
5825     release(&p->lock);
5826 }
5827 
5828 int
5829 pipewrite(struct pipe *p, char *addr, int n)
5830 {
5831   int i;
5832 
5833   acquire(&p->lock);
5834   for(i = 0; i < n; i++){
5835     while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
5836       if(p->readopen == 0 || proc->killed){
5837         release(&p->lock);
5838         return -1;
5839       }
5840       wakeup(&p->nread);
5841       sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
5842     }
5843     p->data[p->nwrite++ % PIPESIZE] = addr[i];
5844   }
5845   wakeup(&p->nread);  //DOC: pipewrite-wakeup1
5846   release(&p->lock);
5847   return n;
5848 }
5849 
5850 int
5851 piperead(struct pipe *p, char *addr, int n)
5852 {
5853   int i;
5854 
5855   acquire(&p->lock);
5856   while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
5857     if(proc->killed){
5858       release(&p->lock);
5859       return -1;
5860     }
5861     sleep(&p->nread, &p->lock); //DOC: piperead-sleep
5862   }
5863   for(i = 0; i < n; i++){  //DOC: piperead-copy
5864     if(p->nread == p->nwrite)
5865       break;
5866     addr[i] = p->data[p->nread++ % PIPESIZE];
5867   }
5868   wakeup(&p->nwrite);  //DOC: piperead-wakeup
5869   release(&p->lock);
5870   return i;
5871 }
5872 
5873 
5874 
5875 
5876 
5877 
5878 
5879 
5880 
5881 
5882 
5883 
5884 
5885 
5886 
5887 
5888 
5889 
5890 
5891 
5892 
5893 
5894 
5895 
5896 
5897 
5898 
5899 
