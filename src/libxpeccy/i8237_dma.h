#pragma once


// controller registers
#define	DMA_CR		0	// command register
#define DMA_SR		DMA_CR
#define	DMA_RR		1	// request register
#define	CMA_CMR		2	// channel mask register
#define DMA_MR		3	// mode reigster
#define	DMA_BTR		4	// byte trigger reset
#define DMA_RES		5	// controller reset
#define DMA_MRES	6	// mask reset
#define	DMA_WAMR	7	// write all masks register
// channels registers
#define DMA_CH_BAR	8	// channel bar/car (base/current address)
#define DMA_CH_CAR	DMA_CH_BAR
#define DMA_CH_BWCR	9	// cnannel bwcr/cwr (base/current counter)
#define DMA_CH_CWR	DMA_CH_BWCR
#define DMA_CH_PAR	10	// channel page register (b16+ of address)
// controller state
#define DMA_IDLE	0	// idle
#define	DMA_PRG		1	// programming
#define	DMA_TRF		2	// transfer

typedef struct {
	unsigned masked:1;	// don't process if 1
	unsigned short bar;	// base address
	unsigned short car;	// current address
	unsigned char par;	// page address register (high 4/8 bits of address)
	unsigned short bwr;	// base counter
	unsigned short cwr;	// current counter
	unsigned char mode;	// b2,3=00:verify mem,01:mem2io,10:verify mem,11:io2mem; b4=1:adr decrement; b6:16bit
	int(*rd)(int, void*);
	void(*wr)(int, int, void*);
} DMAChan;

typedef struct {
	unsigned wrd:1;		// 16-bit dma, ignore btr
	unsigned btr:1;		// byte trigger. 0=low, 1=high
	DMAChan ch[4];
	int state;
	void* ptr;
} i8237DMA;

i8237DMA* dma_create(void*, int);
void dma_destroy(i8237DMA*);
void dma_reset(i8237DMA*);

void dma_wr(i8237DMA*, int reg, int ch, int val);
int dma_rd(i8237DMA*, int reg, int ch);