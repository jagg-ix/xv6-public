6800 // PC keyboard interface constants
6801 
6802 #define KBSTATP         0x64    // kbd controller status port(I)
6803 #define KBS_DIB         0x01    // kbd data in buffer
6804 #define KBDATAP         0x60    // kbd data port(I)
6805 
6806 #define NO              0
6807 
6808 #define SHIFT           (1<<0)
6809 #define CTL             (1<<1)
6810 #define ALT             (1<<2)
6811 
6812 #define CAPSLOCK        (1<<3)
6813 #define NUMLOCK         (1<<4)
6814 #define SCROLLLOCK      (1<<5)
6815 
6816 #define E0ESC           (1<<6)
6817 
6818 // Special keycodes
6819 #define KEY_HOME        0xE0
6820 #define KEY_END         0xE1
6821 #define KEY_UP          0xE2
6822 #define KEY_DN          0xE3
6823 #define KEY_LF          0xE4
6824 #define KEY_RT          0xE5
6825 #define KEY_PGUP        0xE6
6826 #define KEY_PGDN        0xE7
6827 #define KEY_INS         0xE8
6828 #define KEY_DEL         0xE9
6829 
6830 // C('A') == Control-A
6831 #define C(x) (x - '@')
6832 
6833 static uchar shiftcode[256] =
6834 {
6835   [0x1D] CTL,
6836   [0x2A] SHIFT,
6837   [0x36] SHIFT,
6838   [0x38] ALT,
6839   [0x9D] CTL,
6840   [0xB8] ALT
6841 };
6842 
6843 static uchar togglecode[256] =
6844 {
6845   [0x3A] CAPSLOCK,
6846   [0x45] NUMLOCK,
6847   [0x46] SCROLLLOCK
6848 };
6849 
6850 static uchar normalmap[256] =
6851 {
6852   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
6853   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
6854   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
6855   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
6856   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
6857   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
6858   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
6859   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
6860   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
6861   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
6862   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
6863   [0x9C] '\n',      // KP_Enter
6864   [0xB5] '/',       // KP_Div
6865   [0xC8] KEY_UP,    [0xD0] KEY_DN,
6866   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
6867   [0xCB] KEY_LF,    [0xCD] KEY_RT,
6868   [0x97] KEY_HOME,  [0xCF] KEY_END,
6869   [0xD2] KEY_INS,   [0xD3] KEY_DEL
6870 };
6871 
6872 static uchar shiftmap[256] =
6873 {
6874   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
6875   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
6876   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
6877   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
6878   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
6879   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
6880   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
6881   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
6882   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
6883   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
6884   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
6885   [0x9C] '\n',      // KP_Enter
6886   [0xB5] '/',       // KP_Div
6887   [0xC8] KEY_UP,    [0xD0] KEY_DN,
6888   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
6889   [0xCB] KEY_LF,    [0xCD] KEY_RT,
6890   [0x97] KEY_HOME,  [0xCF] KEY_END,
6891   [0xD2] KEY_INS,   [0xD3] KEY_DEL
6892 };
6893 
6894 
6895 
6896 
6897 
6898 
6899 
6900 static uchar ctlmap[256] =
6901 {
6902   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
6903   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
6904   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
6905   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
6906   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
6907   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
6908   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
6909   [0x9C] '\r',      // KP_Enter
6910   [0xB5] C('/'),    // KP_Div
6911   [0xC8] KEY_UP,    [0xD0] KEY_DN,
6912   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
6913   [0xCB] KEY_LF,    [0xCD] KEY_RT,
6914   [0x97] KEY_HOME,  [0xCF] KEY_END,
6915   [0xD2] KEY_INS,   [0xD3] KEY_DEL
6916 };
6917 
6918 
6919 
6920 
6921 
6922 
6923 
6924 
6925 
6926 
6927 
6928 
6929 
6930 
6931 
6932 
6933 
6934 
6935 
6936 
6937 
6938 
6939 
6940 
6941 
6942 
6943 
6944 
6945 
6946 
6947 
6948 
6949 
