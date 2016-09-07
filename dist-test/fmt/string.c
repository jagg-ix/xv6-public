5900 #include "types.h"
5901 #include "x86.h"
5902 
5903 void*
5904 memset(void *dst, int c, uint n)
5905 {
5906   if ((int)dst%4 == 0 && n%4 == 0){
5907     c &= 0xFF;
5908     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
5909   } else
5910     stosb(dst, c, n);
5911   return dst;
5912 }
5913 
5914 int
5915 memcmp(const void *v1, const void *v2, uint n)
5916 {
5917   const uchar *s1, *s2;
5918 
5919   s1 = v1;
5920   s2 = v2;
5921   while(n-- > 0){
5922     if(*s1 != *s2)
5923       return *s1 - *s2;
5924     s1++, s2++;
5925   }
5926 
5927   return 0;
5928 }
5929 
5930 void*
5931 memmove(void *dst, const void *src, uint n)
5932 {
5933   const char *s;
5934   char *d;
5935 
5936   s = src;
5937   d = dst;
5938   if(s < d && s + n > d){
5939     s += n;
5940     d += n;
5941     while(n-- > 0)
5942       *--d = *--s;
5943   } else
5944     while(n-- > 0)
5945       *d++ = *s++;
5946 
5947   return dst;
5948 }
5949 
5950 // memcpy exists to placate GCC.  Use memmove.
5951 void*
5952 memcpy(void *dst, const void *src, uint n)
5953 {
5954   return memmove(dst, src, n);
5955 }
5956 
5957 int
5958 strncmp(const char *p, const char *q, uint n)
5959 {
5960   while(n > 0 && *p && *p == *q)
5961     n--, p++, q++;
5962   if(n == 0)
5963     return 0;
5964   return (uchar)*p - (uchar)*q;
5965 }
5966 
5967 char*
5968 strncpy(char *s, const char *t, int n)
5969 {
5970   char *os;
5971 
5972   os = s;
5973   while(n-- > 0 && (*s++ = *t++) != 0)
5974     ;
5975   while(n-- > 0)
5976     *s++ = 0;
5977   return os;
5978 }
5979 
5980 // Like strncpy but guaranteed to NUL-terminate.
5981 char*
5982 safestrcpy(char *s, const char *t, int n)
5983 {
5984   char *os;
5985 
5986   os = s;
5987   if(n <= 0)
5988     return os;
5989   while(--n > 0 && (*s++ = *t++) != 0)
5990     ;
5991   *s = 0;
5992   return os;
5993 }
5994 
5995 
5996 
5997 
5998 
5999 
6000 int
6001 strlen(const char *s)
6002 {
6003   int n;
6004 
6005   for(n = 0; s[n]; n++)
6006     ;
6007   return n;
6008 }
6009 
6010 
6011 
6012 
6013 
6014 
6015 
6016 
6017 
6018 
6019 
6020 
6021 
6022 
6023 
6024 
6025 
6026 
6027 
6028 
6029 
6030 
6031 
6032 
6033 
6034 
6035 
6036 
6037 
6038 
6039 
6040 
6041 
6042 
6043 
6044 
6045 
6046 
6047 
6048 
6049 
