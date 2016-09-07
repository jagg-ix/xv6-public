6050 // See MultiProcessor Specification Version 1.[14]
6051 
6052 struct mp {             // floating pointer
6053   uchar signature[4];           // "_MP_"
6054   void *physaddr;               // phys addr of MP config table
6055   uchar length;                 // 1
6056   uchar specrev;                // [14]
6057   uchar checksum;               // all bytes must add up to 0
6058   uchar type;                   // MP system config type
6059   uchar imcrp;
6060   uchar reserved[3];
6061 };
6062 
6063 struct mpconf {         // configuration table header
6064   uchar signature[4];           // "PCMP"
6065   ushort length;                // total table length
6066   uchar version;                // [14]
6067   uchar checksum;               // all bytes must add up to 0
6068   uchar product[20];            // product id
6069   uint *oemtable;               // OEM table pointer
6070   ushort oemlength;             // OEM table length
6071   ushort entry;                 // entry count
6072   uint *lapicaddr;              // address of local APIC
6073   ushort xlength;               // extended table length
6074   uchar xchecksum;              // extended table checksum
6075   uchar reserved;
6076 };
6077 
6078 struct mpproc {         // processor table entry
6079   uchar type;                   // entry type (0)
6080   uchar apicid;                 // local APIC id
6081   uchar version;                // local APIC verison
6082   uchar flags;                  // CPU flags
6083     #define MPBOOT 0x02           // This proc is the bootstrap processor.
6084   uchar signature[4];           // CPU signature
6085   uint feature;                 // feature flags from CPUID instruction
6086   uchar reserved[8];
6087 };
6088 
6089 struct mpioapic {       // I/O APIC table entry
6090   uchar type;                   // entry type (2)
6091   uchar apicno;                 // I/O APIC id
6092   uchar version;                // I/O APIC version
6093   uchar flags;                  // I/O APIC flags
6094   uint *addr;                  // I/O APIC address
6095 };
6096 
6097 
6098 
6099 
6100 // Table entry types
6101 #define MPPROC    0x00  // One per processor
6102 #define MPBUS     0x01  // One per bus
6103 #define MPIOAPIC  0x02  // One per I/O APIC
6104 #define MPIOINTR  0x03  // One per bus interrupt source
6105 #define MPLINTR   0x04  // One per system interrupt source
6106 
6107 // Blank page.
6108 
6109 
6110 
6111 
6112 
6113 
6114 
6115 
6116 
6117 
6118 
6119 
6120 
6121 
6122 
6123 
6124 
6125 
6126 
6127 
6128 
6129 
6130 
6131 
6132 
6133 
6134 
6135 
6136 
6137 
6138 
6139 
6140 
6141 
6142 
6143 
6144 
6145 
6146 
6147 
6148 
6149 
