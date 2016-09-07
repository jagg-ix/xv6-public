4950 //
4951 // File descriptors
4952 //
4953 
4954 #include "types.h"
4955 #include "defs.h"
4956 #include "param.h"
4957 #include "fs.h"
4958 #include "file.h"
4959 #include "spinlock.h"
4960 
4961 struct devsw devsw[NDEV];
4962 struct {
4963   struct spinlock lock;
4964   struct file file[NFILE];
4965 } ftable;
4966 
4967 void
4968 fileinit(void)
4969 {
4970   initlock(&ftable.lock, "ftable");
4971 }
4972 
4973 // Allocate a file structure.
4974 struct file*
4975 filealloc(void)
4976 {
4977   struct file *f;
4978 
4979   acquire(&ftable.lock);
4980   for(f = ftable.file; f < ftable.file + NFILE; f++){
4981     if(f->ref == 0){
4982       f->ref = 1;
4983       release(&ftable.lock);
4984       return f;
4985     }
4986   }
4987   release(&ftable.lock);
4988   return 0;
4989 }
4990 
4991 
4992 
4993 
4994 
4995 
4996 
4997 
4998 
4999 
5000 // Increment ref count for file f.
5001 struct file*
5002 filedup(struct file *f)
5003 {
5004   acquire(&ftable.lock);
5005   if(f->ref < 1)
5006     panic("filedup");
5007   f->ref++;
5008   release(&ftable.lock);
5009   return f;
5010 }
5011 
5012 // Close file f.  (Decrement ref count, close when reaches 0.)
5013 void
5014 fileclose(struct file *f)
5015 {
5016   struct file ff;
5017 
5018   acquire(&ftable.lock);
5019   if(f->ref < 1)
5020     panic("fileclose");
5021   if(--f->ref > 0){
5022     release(&ftable.lock);
5023     return;
5024   }
5025   ff = *f;
5026   f->ref = 0;
5027   f->type = FD_NONE;
5028   release(&ftable.lock);
5029 
5030   if(ff.type == FD_PIPE)
5031     pipeclose(ff.pipe, ff.writable);
5032   else if(ff.type == FD_INODE){
5033     begin_op();
5034     iput(ff.ip);
5035     end_op();
5036   }
5037 }
5038 
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
5050 // Get metadata about file f.
5051 int
5052 filestat(struct file *f, struct stat *st)
5053 {
5054   if(f->type == FD_INODE){
5055     ilock(f->ip);
5056     stati(f->ip, st);
5057     iunlock(f->ip);
5058     return 0;
5059   }
5060   return -1;
5061 }
5062 
5063 // Read from file f.
5064 int
5065 fileread(struct file *f, char *addr, int n)
5066 {
5067   int r;
5068 
5069   if(f->readable == 0)
5070     return -1;
5071   if(f->type == FD_PIPE)
5072     return piperead(f->pipe, addr, n);
5073   if(f->type == FD_INODE){
5074     ilock(f->ip);
5075     if((r = readi(f->ip, addr, f->off, n)) > 0)
5076       f->off += r;
5077     iunlock(f->ip);
5078     return r;
5079   }
5080   panic("fileread");
5081 }
5082 
5083 // Write to file f.
5084 int
5085 filewrite(struct file *f, char *addr, int n)
5086 {
5087   int r;
5088 
5089   if(f->writable == 0)
5090     return -1;
5091   if(f->type == FD_PIPE)
5092     return pipewrite(f->pipe, addr, n);
5093   if(f->type == FD_INODE){
5094     // write a few blocks at a time to avoid exceeding
5095     // the maximum log transaction size, including
5096     // i-node, indirect block, allocation blocks,
5097     // and 2 blocks of slop for non-aligned writes.
5098     // this really belongs lower down, since writei()
5099     // might be writing a device like the console.
5100     int max = ((LOGSIZE-1-1-2) / 2) * 512;
5101     int i = 0;
5102     while(i < n){
5103       int n1 = n - i;
5104       if(n1 > max)
5105         n1 = max;
5106 
5107       begin_op();
5108       ilock(f->ip);
5109       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
5110         f->off += r;
5111       iunlock(f->ip);
5112       end_op();
5113 
5114       if(r < 0)
5115         break;
5116       if(r != n1)
5117         panic("short filewrite");
5118       i += r;
5119     }
5120     return i == n ? n : -1;
5121   }
5122   panic("filewrite");
5123 }
5124 
5125 
5126 
5127 
5128 
5129 
5130 
5131 
5132 
5133 
5134 
5135 
5136 
5137 
5138 
5139 
5140 
5141 
5142 
5143 
5144 
5145 
5146 
5147 
5148 
5149 
