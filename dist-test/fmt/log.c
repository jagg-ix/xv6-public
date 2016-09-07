4000 #include "types.h"
4001 #include "defs.h"
4002 #include "param.h"
4003 #include "spinlock.h"
4004 #include "fs.h"
4005 #include "buf.h"
4006 
4007 // Simple logging that allows concurrent FS system calls.
4008 //
4009 // A log transaction contains the updates of multiple FS system
4010 // calls. The logging system only commits when there are
4011 // no FS system calls active. Thus there is never
4012 // any reasoning required about whether a commit might
4013 // write an uncommitted system call's updates to disk.
4014 //
4015 // A system call should call begin_op()/end_op() to mark
4016 // its start and end. Usually begin_op() just increments
4017 // the count of in-progress FS system calls and returns.
4018 // But if it thinks the log is close to running out, it
4019 // sleeps until the last outstanding end_op() commits.
4020 //
4021 // The log is a physical re-do log containing disk blocks.
4022 // The on-disk log format:
4023 //   header block, containing block #s for block A, B, C, ...
4024 //   block A
4025 //   block B
4026 //   block C
4027 //   ...
4028 // Log appends are synchronous.
4029 
4030 // Contents of the header block, used for both the on-disk header block
4031 // and to keep track in memory of logged block# before commit.
4032 struct logheader {
4033   int n;
4034   int block[LOGSIZE];
4035 };
4036 
4037 struct log {
4038   struct spinlock lock;
4039   int start;
4040   int size;
4041   int outstanding; // how many FS sys calls are executing.
4042   int committing;  // in commit(), please wait.
4043   int dev;
4044   struct logheader lh;
4045 };
4046 
4047 
4048 
4049 
4050 struct log log;
4051 
4052 static void recover_from_log(void);
4053 static void commit();
4054 
4055 void
4056 initlog(int dev)
4057 {
4058   if (sizeof(struct logheader) >= BSIZE)
4059     panic("initlog: too big logheader");
4060 
4061   struct superblock sb;
4062   initlock(&log.lock, "log");
4063   readsb(dev, &sb);
4064   log.start = sb.logstart;
4065   log.size = sb.nlog;
4066   log.dev = dev;
4067   recover_from_log();
4068 }
4069 
4070 // Copy committed blocks from log to their home location
4071 static void
4072 install_trans(void)
4073 {
4074   int tail;
4075 
4076   for (tail = 0; tail < log.lh.n; tail++) {
4077     struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
4078     struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
4079     memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
4080     bwrite(dbuf);  // write dst to disk
4081     brelse(lbuf);
4082     brelse(dbuf);
4083   }
4084 }
4085 
4086 // Read the log header from disk into the in-memory log header
4087 static void
4088 read_head(void)
4089 {
4090   struct buf *buf = bread(log.dev, log.start);
4091   struct logheader *lh = (struct logheader *) (buf->data);
4092   int i;
4093   log.lh.n = lh->n;
4094   for (i = 0; i < log.lh.n; i++) {
4095     log.lh.block[i] = lh->block[i];
4096   }
4097   brelse(buf);
4098 }
4099 
4100 // Write in-memory log header to disk.
4101 // This is the true point at which the
4102 // current transaction commits.
4103 static void
4104 write_head(void)
4105 {
4106   struct buf *buf = bread(log.dev, log.start);
4107   struct logheader *hb = (struct logheader *) (buf->data);
4108   int i;
4109   hb->n = log.lh.n;
4110   for (i = 0; i < log.lh.n; i++) {
4111     hb->block[i] = log.lh.block[i];
4112   }
4113   bwrite(buf);
4114   brelse(buf);
4115 }
4116 
4117 static void
4118 recover_from_log(void)
4119 {
4120   read_head();
4121   install_trans(); // if committed, copy from log to disk
4122   log.lh.n = 0;
4123   write_head(); // clear the log
4124 }
4125 
4126 // called at the start of each FS system call.
4127 void
4128 begin_op(void)
4129 {
4130   acquire(&log.lock);
4131   while(1){
4132     if(log.committing){
4133       sleep(&log, &log.lock);
4134     } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
4135       // this op might exhaust log space; wait for commit.
4136       sleep(&log, &log.lock);
4137     } else {
4138       log.outstanding += 1;
4139       release(&log.lock);
4140       break;
4141     }
4142   }
4143 }
4144 
4145 
4146 
4147 
4148 
4149 
4150 // called at the end of each FS system call.
4151 // commits if this was the last outstanding operation.
4152 void
4153 end_op(void)
4154 {
4155   int do_commit = 0;
4156 
4157   acquire(&log.lock);
4158   log.outstanding -= 1;
4159   if(log.committing)
4160     panic("log.committing");
4161   if(log.outstanding == 0){
4162     do_commit = 1;
4163     log.committing = 1;
4164   } else {
4165     // begin_op() may be waiting for log space.
4166     wakeup(&log);
4167   }
4168   release(&log.lock);
4169 
4170   if(do_commit){
4171     // call commit w/o holding locks, since not allowed
4172     // to sleep with locks.
4173     commit();
4174     acquire(&log.lock);
4175     log.committing = 0;
4176     wakeup(&log);
4177     release(&log.lock);
4178   }
4179 }
4180 
4181 // Copy modified blocks from cache to log.
4182 static void
4183 write_log(void)
4184 {
4185   int tail;
4186 
4187   for (tail = 0; tail < log.lh.n; tail++) {
4188     struct buf *to = bread(log.dev, log.start+tail+1); // log block
4189     struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
4190     memmove(to->data, from->data, BSIZE);
4191     bwrite(to);  // write the log
4192     brelse(from);
4193     brelse(to);
4194   }
4195 }
4196 
4197 
4198 
4199 
4200 static void
4201 commit()
4202 {
4203   if (log.lh.n > 0) {
4204     write_log();     // Write modified blocks from cache to log
4205     write_head();    // Write header to disk -- the real commit
4206     install_trans(); // Now install writes to home locations
4207     log.lh.n = 0;
4208     write_head();    // Erase the transaction from the log
4209   }
4210 }
4211 
4212 // Caller has modified b->data and is done with the buffer.
4213 // Record the block number and pin in the cache with B_DIRTY.
4214 // commit()/write_log() will do the disk write.
4215 //
4216 // log_write() replaces bwrite(); a typical use is:
4217 //   bp = bread(...)
4218 //   modify bp->data[]
4219 //   log_write(bp)
4220 //   brelse(bp)
4221 void
4222 log_write(struct buf *b)
4223 {
4224   int i;
4225 
4226   if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
4227     panic("too big a transaction");
4228   if (log.outstanding < 1)
4229     panic("log_write outside of trans");
4230 
4231   acquire(&log.lock);
4232   for (i = 0; i < log.lh.n; i++) {
4233     if (log.lh.block[i] == b->blockno)   // log absorbtion
4234       break;
4235   }
4236   log.lh.block[i] = b->blockno;
4237   if (i == log.lh.n)
4238     log.lh.n++;
4239   b->flags |= B_DIRTY; // prevent eviction
4240   release(&log.lock);
4241 }
4242 
4243 
4244 
4245 
4246 
4247 
4248 
4249 
