1550 #include "param.h"
1551 #include "types.h"
1552 #include "defs.h"
1553 #include "x86.h"
1554 #include "memlayout.h"
1555 #include "mmu.h"
1556 #include "proc.h"
1557 #include "elf.h"
1558 
1559 extern char data[];  // defined by kernel.ld
1560 pde_t *kpgdir;  // for use in scheduler()
1561 struct segdesc gdt[NSEGS];
1562 
1563 // Set up CPU's kernel segment descriptors.
1564 // Run once on entry on each CPU.
1565 void
1566 seginit(void)
1567 {
1568   struct cpu *c;
1569 
1570   // Map "logical" addresses to virtual addresses using identity map.
1571   // Cannot share a CODE descriptor for both kernel and user
1572   // because it would have to have DPL_USR, but the CPU forbids
1573   // an interrupt from CPL=0 to DPL=3.
1574   c = &cpus[cpunum()];
1575   c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
1576   c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
1577   c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
1578   c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
1579 
1580   // Map cpu, and curproc
1581   c->gdt[SEG_KCPU] = SEG(STA_W, &c->cpu, 8, 0);
1582 
1583   lgdt(c->gdt, sizeof(c->gdt));
1584   loadgs(SEG_KCPU << 3);
1585 
1586   // Initialize cpu-local storage.
1587   cpu = c;
1588   proc = 0;
1589 }
1590 
1591 
1592 
1593 
1594 
1595 
1596 
1597 
1598 
1599 
1600 // Return the address of the PTE in page table pgdir
1601 // that corresponds to virtual address va.  If alloc!=0,
1602 // create any required page table pages.
1603 static pte_t *
1604 walkpgdir(pde_t *pgdir, const void *va, int alloc)
1605 {
1606   pde_t *pde;
1607   pte_t *pgtab;
1608 
1609   pde = &pgdir[PDX(va)];
1610   if(*pde & PTE_P){
1611     pgtab = (pte_t*)p2v(PTE_ADDR(*pde));
1612   } else {
1613     if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
1614       return 0;
1615     // Make sure all those PTE_P bits are zero.
1616     memset(pgtab, 0, PGSIZE);
1617     // The permissions here are overly generous, but they can
1618     // be further restricted by the permissions in the page table
1619     // entries, if necessary.
1620     *pde = v2p(pgtab) | PTE_P | PTE_W | PTE_U;
1621   }
1622   return &pgtab[PTX(va)];
1623 }
1624 
1625 // Create PTEs for virtual addresses starting at va that refer to
1626 // physical addresses starting at pa. va and size might not
1627 // be page-aligned.
1628 static int
1629 mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
1630 {
1631   char *a, *last;
1632   pte_t *pte;
1633 
1634   a = (char*)PGROUNDDOWN((uint)va);
1635   last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
1636   for(;;){
1637     if((pte = walkpgdir(pgdir, a, 1)) == 0)
1638       return -1;
1639     if(*pte & PTE_P)
1640       panic("remap");
1641     *pte = pa | perm | PTE_P;
1642     if(a == last)
1643       break;
1644     a += PGSIZE;
1645     pa += PGSIZE;
1646   }
1647   return 0;
1648 }
1649 
1650 // There is one page table per process, plus one that's used when
1651 // a CPU is not running any process (kpgdir). The kernel uses the
1652 // current process's page table during system calls and interrupts;
1653 // page protection bits prevent user code from using the kernel's
1654 // mappings.
1655 //
1656 // setupkvm() and exec() set up every page table like this:
1657 //
1658 //   0..KERNBASE: user memory (text+data+stack+heap), mapped to
1659 //                phys memory allocated by the kernel
1660 //   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
1661 //   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
1662 //                for the kernel's instructions and r/o data
1663 //   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
1664 //                                  rw data + free physical memory
1665 //   0xfe000000..0: mapped direct (devices such as ioapic)
1666 //
1667 // The kernel allocates physical memory for its heap and for user memory
1668 // between V2P(end) and the end of physical memory (PHYSTOP)
1669 // (directly addressable from end..P2V(PHYSTOP)).
1670 
1671 // This table defines the kernel's mappings, which are present in
1672 // every process's page table.
1673 static struct kmap {
1674   void *virt;
1675   uint phys_start;
1676   uint phys_end;
1677   int perm;
1678 } kmap[] = {
1679  { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
1680  { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
1681  { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
1682  { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
1683 };
1684 
1685 // Set up kernel part of a page table.
1686 pde_t*
1687 setupkvm(void)
1688 {
1689   pde_t *pgdir;
1690   struct kmap *k;
1691 
1692   if((pgdir = (pde_t*)kalloc()) == 0)
1693     return 0;
1694   memset(pgdir, 0, PGSIZE);
1695   if (p2v(PHYSTOP) > (void*)DEVSPACE)
1696     panic("PHYSTOP too high");
1697   for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
1698     if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
1699                 (uint)k->phys_start, k->perm) < 0)
1700       return 0;
1701   return pgdir;
1702 }
1703 
1704 // Allocate one page table for the machine for the kernel address
1705 // space for scheduler processes.
1706 void
1707 kvmalloc(void)
1708 {
1709   kpgdir = setupkvm();
1710   switchkvm();
1711 }
1712 
1713 // Switch h/w page table register to the kernel-only page table,
1714 // for when no process is running.
1715 void
1716 switchkvm(void)
1717 {
1718   lcr3(v2p(kpgdir));   // switch to the kernel page table
1719 }
1720 
1721 // Switch TSS and h/w page table to correspond to process p.
1722 void
1723 switchuvm(struct proc *p)
1724 {
1725   pushcli();
1726   cpu->gdt[SEG_TSS] = SEG16(STS_T32A, &cpu->ts, sizeof(cpu->ts)-1, 0);
1727   cpu->gdt[SEG_TSS].s = 0;
1728   cpu->ts.ss0 = SEG_KDATA << 3;
1729   cpu->ts.esp0 = (uint)proc->kstack + KSTACKSIZE;
1730   ltr(SEG_TSS << 3);
1731   if(p->pgdir == 0)
1732     panic("switchuvm: no pgdir");
1733   lcr3(v2p(p->pgdir));  // switch to new address space
1734   popcli();
1735 }
1736 
1737 
1738 
1739 
1740 
1741 
1742 
1743 
1744 
1745 
1746 
1747 
1748 
1749 
1750 // Load the initcode into address 0 of pgdir.
1751 // sz must be less than a page.
1752 void
1753 inituvm(pde_t *pgdir, char *init, uint sz)
1754 {
1755   char *mem;
1756 
1757   if(sz >= PGSIZE)
1758     panic("inituvm: more than a page");
1759   mem = kalloc();
1760   memset(mem, 0, PGSIZE);
1761   mappages(pgdir, 0, PGSIZE, v2p(mem), PTE_W|PTE_U);
1762   memmove(mem, init, sz);
1763 }
1764 
1765 // Load a program segment into pgdir.  addr must be page-aligned
1766 // and the pages from addr to addr+sz must already be mapped.
1767 int
1768 loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
1769 {
1770   uint i, pa, n;
1771   pte_t *pte;
1772 
1773   if((uint) addr % PGSIZE != 0)
1774     panic("loaduvm: addr must be page aligned");
1775   for(i = 0; i < sz; i += PGSIZE){
1776     if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
1777       panic("loaduvm: address should exist");
1778     pa = PTE_ADDR(*pte);
1779     if(sz - i < PGSIZE)
1780       n = sz - i;
1781     else
1782       n = PGSIZE;
1783     if(readi(ip, p2v(pa), offset+i, n) != n)
1784       return -1;
1785   }
1786   return 0;
1787 }
1788 
1789 
1790 
1791 
1792 
1793 
1794 
1795 
1796 
1797 
1798 
1799 
1800 // Allocate page tables and physical memory to grow process from oldsz to
1801 // newsz, which need not be page aligned.  Returns new size or 0 on error.
1802 int
1803 allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
1804 {
1805   char *mem;
1806   uint a;
1807 
1808   if(newsz >= KERNBASE)
1809     return 0;
1810   if(newsz < oldsz)
1811     return oldsz;
1812 
1813   a = PGROUNDUP(oldsz);
1814   for(; a < newsz; a += PGSIZE){
1815     mem = kalloc();
1816     if(mem == 0){
1817       cprintf("allocuvm out of memory\n");
1818       deallocuvm(pgdir, newsz, oldsz);
1819       return 0;
1820     }
1821     memset(mem, 0, PGSIZE);
1822     mappages(pgdir, (char*)a, PGSIZE, v2p(mem), PTE_W|PTE_U);
1823   }
1824   return newsz;
1825 }
1826 
1827 // Deallocate user pages to bring the process size from oldsz to
1828 // newsz.  oldsz and newsz need not be page-aligned, nor does newsz
1829 // need to be less than oldsz.  oldsz can be larger than the actual
1830 // process size.  Returns the new process size.
1831 int
1832 deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
1833 {
1834   pte_t *pte;
1835   uint a, pa;
1836 
1837   if(newsz >= oldsz)
1838     return oldsz;
1839 
1840   a = PGROUNDUP(newsz);
1841   for(; a  < oldsz; a += PGSIZE){
1842     pte = walkpgdir(pgdir, (char*)a, 0);
1843     if(!pte)
1844       a += (NPTENTRIES - 1) * PGSIZE;
1845     else if((*pte & PTE_P) != 0){
1846       pa = PTE_ADDR(*pte);
1847       if(pa == 0)
1848         panic("kfree");
1849       char *v = p2v(pa);
1850       kfree(v);
1851       *pte = 0;
1852     }
1853   }
1854   return newsz;
1855 }
1856 
1857 // Free a page table and all the physical memory pages
1858 // in the user part.
1859 void
1860 freevm(pde_t *pgdir)
1861 {
1862   uint i;
1863 
1864   if(pgdir == 0)
1865     panic("freevm: no pgdir");
1866   deallocuvm(pgdir, KERNBASE, 0);
1867   for(i = 0; i < NPDENTRIES; i++){
1868     if(pgdir[i] & PTE_P){
1869       char * v = p2v(PTE_ADDR(pgdir[i]));
1870       kfree(v);
1871     }
1872   }
1873   kfree((char*)pgdir);
1874 }
1875 
1876 // Clear PTE_U on a page. Used to create an inaccessible
1877 // page beneath the user stack.
1878 void
1879 clearpteu(pde_t *pgdir, char *uva)
1880 {
1881   pte_t *pte;
1882 
1883   pte = walkpgdir(pgdir, uva, 0);
1884   if(pte == 0)
1885     panic("clearpteu");
1886   *pte &= ~PTE_U;
1887 }
1888 
1889 
1890 
1891 
1892 
1893 
1894 
1895 
1896 
1897 
1898 
1899 
1900 // Given a parent process's page table, create a copy
1901 // of it for a child.
1902 pde_t*
1903 copyuvm(pde_t *pgdir, uint sz)
1904 {
1905   pde_t *d;
1906   pte_t *pte;
1907   uint pa, i, flags;
1908   char *mem;
1909 
1910   if((d = setupkvm()) == 0)
1911     return 0;
1912   for(i = 0; i < sz; i += PGSIZE){
1913     if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
1914       panic("copyuvm: pte should exist");
1915     if(!(*pte & PTE_P))
1916       panic("copyuvm: page not present");
1917     pa = PTE_ADDR(*pte);
1918     flags = PTE_FLAGS(*pte);
1919     if((mem = kalloc()) == 0)
1920       goto bad;
1921     memmove(mem, (char*)p2v(pa), PGSIZE);
1922     if(mappages(d, (void*)i, PGSIZE, v2p(mem), flags) < 0)
1923       goto bad;
1924   }
1925   return d;
1926 
1927 bad:
1928   freevm(d);
1929   return 0;
1930 }
1931 
1932 // Map user virtual address to kernel address.
1933 char*
1934 uva2ka(pde_t *pgdir, char *uva)
1935 {
1936   pte_t *pte;
1937 
1938   pte = walkpgdir(pgdir, uva, 0);
1939   if((*pte & PTE_P) == 0)
1940     return 0;
1941   if((*pte & PTE_U) == 0)
1942     return 0;
1943   return (char*)p2v(PTE_ADDR(*pte));
1944 }
1945 
1946 
1947 
1948 
1949 
1950 // Copy len bytes from p to user address va in page table pgdir.
1951 // Most useful when pgdir is not the current page table.
1952 // uva2ka ensures this only works for PTE_U pages.
1953 int
1954 copyout(pde_t *pgdir, uint va, void *p, uint len)
1955 {
1956   char *buf, *pa0;
1957   uint n, va0;
1958 
1959   buf = (char*)p;
1960   while(len > 0){
1961     va0 = (uint)PGROUNDDOWN(va);
1962     pa0 = uva2ka(pgdir, (char*)va0);
1963     if(pa0 == 0)
1964       return -1;
1965     n = PGSIZE - (va - va0);
1966     if(n > len)
1967       n = len;
1968     memmove(pa0 + (va - va0), buf, n);
1969     len -= n;
1970     buf += n;
1971     va = va0 + PGSIZE;
1972   }
1973   return 0;
1974 }
1975 
1976 // Blank page.
1977 // Blank page.
1978 // Blank page.
1979 
1980 
1981 
1982 
1983 
1984 
1985 
1986 
1987 
1988 
1989 
1990 
1991 
1992 
1993 
1994 
1995 
1996 
1997 
1998 
1999 
