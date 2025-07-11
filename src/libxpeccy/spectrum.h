#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cpu/cpu.h"
#include "video/video.h"
#include "memory.h"

#include "input.h"
#include "tape.h"
#include "fdc.h"
#include "hdd.h"
#include "sdcard.h"
#include "cartridge.h"
#include "i8255_ppi.h"
#include "i8253_pit.h"
#include "i8259_pic.h"
#include "i8042_kbd.h"
#include "i8237_dma.h"
#include "mos6526_cia.h"
#include "uart8250.h"

#include "sound/ayym.h"
#include "sound/gs.h"
#include "sound/saa1099.h"
#include "sound/soundrive.h"
#include "sound/gbsound.h"
#include "sound/nesapu.h"

#ifdef HAVEZLIB
	#include <zlib.h>
#endif

// hw reset rompage
enum {
	RES_DEFAULT = 0,
	RES_48,
	RES_128,
	RES_DOS,
	RES_SHADOW
};

enum {
	DBG_VIEW_CODE = 0x00,
	DBG_VIEW_BYTE = 0x10,
	DBG_VIEW_WORD = 0x20,
	DBG_VIEW_ADDR = 0x30,
	DBG_VIEW_TEXT = 0x40,
	DBG_VIEW_EXEC = 0x50
};

typedef struct {
	unsigned char l;
	unsigned char h;
	unsigned char x;
} DMAaddr;

typedef struct {
	unsigned char flag;
	unsigned char page;
} memEntry;

typedef struct {
	int t;
	int a;
	unsigned char* ptr;
} bpChecker;

#define p7FFD	reg[0]
#define p1FFD	reg[1]
#define pEFF7	reg[2]
#define pDFFD	reg[3]
#define p77hi	reg[4]
#define prt2	reg[5]

typedef struct {
	unsigned brk:1;			// breakpoint
	unsigned debug:1;		// dont' do breakpoints
	unsigned maping:1;		// map memory during execution
	unsigned frmStrobe:1;		// new frame started
	unsigned intStrobe:1;		// int front
	unsigned nmiRequest:1;		// Magic button pressed
//	unsigned halt:1;		// strobe of CPU HALT instruction
	unsigned firstRun:1;
	unsigned ddpal:1;		// ATM2+: use ddp palette
	unsigned z_i:1;			// ATM2+: unblock VSYNC INT
	unsigned speed:1;		// turbo cpu frq

	unsigned rom:1;			// b4,7ffd
	unsigned dos:1;			// BDI dos
	unsigned cpm:1;
	unsigned ext:1;
	unsigned bdiz:1;		// BDI ports accessible

	unsigned evenM1:1;		// scorpion wait mode
	unsigned contMem:1;		// contended mem
	unsigned contIO:1;		// contended IO

	unsigned vidint:1;
	unsigned brkirq:1;		// break on irq

	double fps;
	double cpuFrq;
	double frqMul;
	unsigned char intVector;

	int brkt;	// breakpoint type (cpu, ram, rom...)
	int brka;	// breakpoint addr

	char* msg;			// message ptr for displaying outside
	int resbank;			// rompart active after reset

	struct HardWare *hw;
	CPU* cpu;
	Memory* mem;
	Video* vid;

	Keyboard* keyb;
	Joystick* joy;
	Mouse* mouse;

	Tape* tape;
	DiskIF* dif;
	IDE* ide;

	bitChan* beep;
	SDCard* sdc;
	TSound* ts;
	GSound* gs;
	SDrive* sdrv;
	saaChip* saa;
	gbSound* gbsnd;
	nesAPU* nesapu;

	xCartridge* slot;		// cartrige slot (MSX, GB, NES)

#ifdef HAVEZLIB

	struct {
		unsigned start:1;
		unsigned stop:1;
		unsigned play:1;
		unsigned overio:1;
		int fTotal;
		int fCurrent;
		int fCount;
		FILE* file;		// converted tmp-file
		struct {
			int fetches;
			int size;
			unsigned char data[0x10000];
			int pos;
		} frm;
	} rzx;

#endif

	int tickCount;		// accumulate T
	int frmtCount;		// accumulate T, but reset each INT
	int hCount;		// T before HALT = frmtCount @ HALT
	int fCount;		// T in last frame
	int nsPerTick;

	unsigned char reg[512];		// internal registers
	unsigned short wdata;
	memEntry memMap[16];			// memory map for ATM2, PentEvo
	unsigned char brkRamMap[MEM_4M];	// ram brk/type : b0..3:brk flags, b4..7:type
	unsigned char brkRomMap[MEM_512K];	// rom brk/type : b0..3:brk flags, b4..7:type
	unsigned char brkAdrMap[MEM_64K];	// adr brk
	unsigned char brkIOMap[MEM_64K];	// io brk

	PPI* ppi;
	struct {
		unsigned char evoBF;		// PentEvo rw ports
		unsigned char evo2F;
		unsigned char evo4F;
		unsigned char evo6F;
		unsigned char evo8F;
		unsigned char blVer[16];	// bootloader info
		unsigned char bcVer[16];	// baseconf info
	} evo;
	struct {
		DMAaddr src;
		DMAaddr dst;
		unsigned char len;
		unsigned char num;
	} dma;
	struct {
		int flag;
		unsigned short tsMapAdr;	// adr for altera mapping
		unsigned char Page0;
		unsigned char p01af;
		unsigned char p02af;
		unsigned char p03af;
		unsigned char p04af;
		unsigned char p05af;
		unsigned char p21af;
		unsigned char pwr_up;		// 1 on 1st run, 0 after reading 00AF
		unsigned char vdos;
	} tsconf;
	struct {
		unsigned char p7E;		// color num (if trig7E=0)
	} profi;
	struct {
		unsigned char keyLine;		// selected keyboard line
		unsigned char mFFFF;		// mem FFFF : mapper secondary slot
		unsigned char pF5;
		unsigned char pslot[4];
		unsigned char sslot[4];
	} msx;
	struct {
//		unsigned vblank:1;		// vid->vblank for catching 0->1
		unsigned irq;
		int type;			// DENDY | NTSC | PAL
		int priPadState;		// b0..7 = A,B,sel,start,up,down,left,right,0,0,0,0,....
		int secPadState;
		int priJoy;			// shift registers (reading 4016/17 returns bit 0 & shift right)
		int secJoy;			//		write b0=1 to 4016 restore status
	} nes;
	struct {
		unsigned boot:1;	// boot rom on
		unsigned speedrq:1;	// cpu speed change request @ next STOP
		int buttons;
		struct {
			struct {
				long per;
				long cnt;
			} div;		// divider (16KHz, inc FF04)
			struct {
				unsigned on:1;
				unsigned intrq:1;
				long per;
				long cnt;
			} t;		// manual timer
		} timer;

		int vbank;		// video bank (0,1)
		int wbank;		// ram bank (d000..dfff)
		unsigned char iram[256];	// internal ram (FF80..FFFE)
		unsigned char iomap[128];
	} gb;
	struct {
		unsigned char reg00;
		unsigned char reg01;
		unsigned char memMode;
		unsigned char keyrow;
		unsigned char vicBank;	// b0,1 = b14,15 of VIC address
		unsigned char rs232a;	// rs232 output line
		unsigned char rs232b;
		CIA* cia1;
		CIA* cia2;
	} c64;
	CMOS cmos;
// ibm
	unsigned a20gate:1;
	unsigned char post;
	unsigned char idx;
	PIT* pit;		// timer
	PIC* mpic;		// master pic
	PIC* spic;		// slave pic
	PS2Ctrl* ps2c;
	i8237DMA* dma1;		// 8-bit dma
	i8237DMA* dma2;		// 16-bit dma
	UART* com1;		// com1 (mouse) controller
} Computer;

#include "hardware/hardware.h"

Computer* compCreate();
void compDestroy(Computer*);
void compReset(Computer*,int);
int compExec(Computer*);

//void compKeyPress(Computer*, int, keyEntry*);
//void compKeyRelease(Computer*, int, keyEntry*);
void comp_kbd_release(Computer*);

void compSetBaseFrq(Computer*,double);
void compSetTurbo(Computer*,double);
int compSetHardware(Computer*,const char*);
void comp_set_layout(Computer*, vLayout*);

// read-write cmos
unsigned char cmsRd(Computer*);
void cmsWr(Computer*, int);

void rzxStop(Computer*);

void comp_brk(Computer*);
unsigned char* getBrkPtr(Computer*, int);
unsigned char getBrk(Computer*, int);
void setBrk(Computer*, int, unsigned char);

#ifdef __cplusplus
}
#endif
