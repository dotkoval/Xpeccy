#pragma once

// compilation flags
#define USE_HOST_KEYBOARD	1

// interrupt types
enum {
	IRQ_VID_INT = 1,// video
	IRQ_VID_INT_E,
	IRQ_VID_VBLANK,
	IRQ_VID_LINE,	// tsconf
	IRQ_RZX_INT,
	IRQ_DMA,
	IRQ_CPU_SYNC,	// sync cpu ticks
	IRQ_FDC,	// ibm
	IRQ_FDC_RD,
	IRQ_FDC_WR,
	IRQ_FDD_RDY,
	IRQ_HDD_PRI,
	IRQ_SLAVE_PIC,
	IRQ_MASTER_PIC,
	IRQ_KBD,	// ps/2 controller kbd int || gbc buttons
	IRQ_MOUSE,	// ps/2 controller mouse int
	IRQ_RESET,
	IRQ_PIT_CH0,
	IRQ_PIT_CH1,
	IRQ_PIT_CH2,
	IRQ_APU,	// nes
	IRQ_BRK,
	IRQ_CIA1,	// commodore
	IRQ_CIA2,
	IRQ_TAP_0,
	IRQ_TAP_1
};

typedef void(*cbirq)(int, void*);
// external data ports rd/wr
typedef int(*cbxrd)(int, void*);
typedef void(*cbxwr)(int, int, void*);

// 16bit reg

#ifdef WORDS_BIG_ENDIAN
	#define PAIR(p,h,l) union{unsigned short p; struct {unsigned char h; unsigned char l;};}
#else
	#define PAIR(p,h,l) union{unsigned short p; struct {unsigned char l; unsigned char h;};}
#endif

typedef PAIR(w,h,l) xpair;

// memory size
#define MEM_256	(1<<8)
#define MEM_512	(1<<9)
#define MEM_1K	(1<<10)
#define MEM_2K	(1<<11)
#define MEM_4K	(1<<12)
#define MEM_8K	(1<<13)
#define MEM_16K	(1<<14)
#define MEM_32K	(1<<15)
#define MEM_64K	(1<<16)
#define MEM_128K	(1<<17)
#define MEM_256K	(1<<18)
#define MEM_512K	(1<<19)
#define MEM_1M	(1<<20)
#define MEM_2M	(1<<21)
#define MEM_4M	(1<<22)
