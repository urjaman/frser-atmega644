
enum partype {
	PT_LIST_END = 0,
	PT_PMS = 1, //"%s" with PGM_P
	PT_STR = 2, //"%s"
	PT_HB = 3, //"%02X"
	PT_HW = 4, //"%04X"
	PT_DW = 5, //"%u" (0 - 65535)
	PT_DH = 6, // luint2outdual with 16b param
	PT_CB = 7, // "%c"
	PT_CW = 8, // "%c%c"
};

#define PTCW(a,b) (((b)<<8)|(a))
#define XPT2(a,b) (((b)<<4)|(a))
#define XPT3(a,b,c) (((c)<<8)|((b)<<4)|(a))
#define XPT4(a,b,c,d) (((d)<<12)|((c)<<8)|((b)<<4)|(a))

void xprint_put(uint16_t format, ...);
uint8_t xprint_get(char *buf, uint8_t len);
