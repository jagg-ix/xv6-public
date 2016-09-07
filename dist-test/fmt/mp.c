6150 // Multiprocessor support
6151 // Search memory for MP description structures.
6152 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
6153 
6154 #include "types.h"
6155 #include "defs.h"
6156 #include "param.h"
6157 #include "memlayout.h"
6158 #include "mp.h"
6159 #include "x86.h"
6160 #include "mmu.h"
6161 #include "proc.h"
6162 
6163 struct cpu cpus[NCPU];
6164 static struct cpu *bcpu;
6165 int ismp;
6166 int ncpu;
6167 uchar ioapicid;
6168 
6169 int
6170 mpbcpu(void)
6171 {
6172   return bcpu-cpus;
6173 }
6174 
6175 static uchar
6176 sum(uchar *addr, int len)
6177 {
6178   int i, sum;
6179 
6180   sum = 0;
6181   for(i=0; i<len; i++)
6182     sum += addr[i];
6183   return sum;
6184 }
6185 
6186 // Look for an MP structure in the len bytes at addr.
6187 static struct mp*
6188 mpsearch1(uint a, int len)
6189 {
6190   uchar *e, *p, *addr;
6191 
6192   addr = p2v(a);
6193   e = addr+len;
6194   for(p = addr; p < e; p += sizeof(struct mp))
6195     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
6196       return (struct mp*)p;
6197   return 0;
6198 }
6199 
6200 // Search for the MP Floating Pointer Structure, which according to the
6201 // spec is in one of the following three locations:
6202 // 1) in the first KB of the EBDA;
6203 // 2) in the last KB of system base memory;
6204 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
6205 static struct mp*
6206 mpsearch(void)
6207 {
6208   uchar *bda;
6209   uint p;
6210   struct mp *mp;
6211 
6212   bda = (uchar *) P2V(0x400);
6213   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
6214     if((mp = mpsearch1(p, 1024)))
6215       return mp;
6216   } else {
6217     p = ((bda[0x14]<<8)|bda[0x13])*1024;
6218     if((mp = mpsearch1(p-1024, 1024)))
6219       return mp;
6220   }
6221   return mpsearch1(0xF0000, 0x10000);
6222 }
6223 
6224 // Search for an MP configuration table.  For now,
6225 // don't accept the default configurations (physaddr == 0).
6226 // Check for correct signature, calculate the checksum and,
6227 // if correct, check the version.
6228 // To do: check extended table checksum.
6229 static struct mpconf*
6230 mpconfig(struct mp **pmp)
6231 {
6232   struct mpconf *conf;
6233   struct mp *mp;
6234 
6235   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
6236     return 0;
6237   conf = (struct mpconf*) p2v((uint) mp->physaddr);
6238   if(memcmp(conf, "PCMP", 4) != 0)
6239     return 0;
6240   if(conf->version != 1 && conf->version != 4)
6241     return 0;
6242   if(sum((uchar*)conf, conf->length) != 0)
6243     return 0;
6244   *pmp = mp;
6245   return conf;
6246 }
6247 
6248 
6249 
6250 void
6251 mpinit(void)
6252 {
6253   uchar *p, *e;
6254   struct mp *mp;
6255   struct mpconf *conf;
6256   struct mpproc *proc;
6257   struct mpioapic *ioapic;
6258 
6259   bcpu = &cpus[0];
6260   if((conf = mpconfig(&mp)) == 0)
6261     return;
6262   ismp = 1;
6263   lapic = (uint*)conf->lapicaddr;
6264   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
6265     switch(*p){
6266     case MPPROC:
6267       proc = (struct mpproc*)p;
6268       if(ncpu != proc->apicid){
6269         cprintf("mpinit: ncpu=%d apicid=%d\n", ncpu, proc->apicid);
6270         ismp = 0;
6271       }
6272       if(proc->flags & MPBOOT)
6273         bcpu = &cpus[ncpu];
6274       cpus[ncpu].id = ncpu;
6275       ncpu++;
6276       p += sizeof(struct mpproc);
6277       continue;
6278     case MPIOAPIC:
6279       ioapic = (struct mpioapic*)p;
6280       ioapicid = ioapic->apicno;
6281       p += sizeof(struct mpioapic);
6282       continue;
6283     case MPBUS:
6284     case MPIOINTR:
6285     case MPLINTR:
6286       p += 8;
6287       continue;
6288     default:
6289       cprintf("mpinit: unknown config type %x\n", *p);
6290       ismp = 0;
6291     }
6292   }
6293   if(!ismp){
6294     // Didn't like what we found; fall back to no MP.
6295     ncpu = 1;
6296     lapic = 0;
6297     ioapicid = 0;
6298     return;
6299   }
6300   if(mp->imcrp){
6301     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
6302     // But it would on real hardware.
6303     outb(0x22, 0x70);   // Select IMCR
6304     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
6305   }
6306 }
6307 
6308 
6309 
6310 
6311 
6312 
6313 
6314 
6315 
6316 
6317 
6318 
6319 
6320 
6321 
6322 
6323 
6324 
6325 
6326 
6327 
6328 
6329 
6330 
6331 
6332 
6333 
6334 
6335 
6336 
6337 
6338 
6339 
6340 
6341 
6342 
6343 
6344 
6345 
6346 
6347 
6348 
6349 
