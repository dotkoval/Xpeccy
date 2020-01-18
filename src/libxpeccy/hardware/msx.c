#include "hardware.h"

// TODO: recheck memory map (A8 port)

typedef struct {
	int type;
	int num;
} mPageNr;

static mPageNr msxMemTab[4][4] = {
	{{MEM_ROM, 0}, {MEM_ROM, 1}, {MEM_RAM, 1}, {MEM_RAM, 0}},
	{{MEM_SLOT,0},{MEM_SLOT,0},{MEM_SLOT,0},{MEM_SLOT,0}},
	{{MEM_SLOT,1},{MEM_SLOT,1},{MEM_SLOT,1},{MEM_SLOT,1}},
	{{MEM_RAM, 3},{MEM_RAM, 2},{MEM_RAM, 1},{MEM_RAM, 0}}
};

unsigned char msxSlotRd(unsigned short adr, void* data) {
	xCartridge* slot = (xCartridge*)data;
	return sltRead(slot, SLT_PRG, adr);
}

void msxSlotWr(unsigned short adr, unsigned char val, void* data) {
	xCartridge* slot = (xCartridge*)data;
	sltWrite(slot, SLT_PRG, adr, val);
}

void msxSetMem(Computer* comp, int bank, unsigned char slot) {
	slot &= 3;
	bank &= 3;
	int type = msxMemTab[slot][bank].type;
	int num = msxMemTab[slot][bank].num;
	switch(type) {
		case MEM_SLOT:
			memSetBank(comp->mem, bank << 6, MEM_SLOT, comp->slot->memMap[bank], MEM_16K, msxSlotRd, msxSlotWr, comp->slot);
			break;
		case MEM_RAM:
			memSetBank(comp->mem, bank << 6, MEM_RAM, comp->reg[0xfc | bank] & 7, MEM_16K, NULL, NULL, NULL);
			break;
		case MEM_ROM:
			memSetBank(comp->mem, bank << 6, MEM_ROM, num, MEM_16K, NULL, NULL, NULL);
			break;
	}
}

void msxMapMem(Computer* comp) {
	msxSetMem(comp, 0, comp->msx.pA8 & 0x03);
	msxSetMem(comp, 1, (comp->msx.pA8 & 0x0c) >> 2);
	msxSetMem(comp, 2, (comp->msx.pA8 & 0x30) >> 4);
	msxSetMem(comp, 3, (comp->msx.pA8 & 0xc0) >> 6);
}

void msxResetSlot(xCartridge* slot) {
	slot->memMap[0] = 0;
	slot->memMap[1] = 0;
	slot->memMap[2] = 0;
	slot->memMap[3] = 0;
}

void msxReset(Computer* comp) {
	kbdSetMode(comp->keyb, KBD_MSX);
	comp->msx.pA8 = 0x00;
	comp->reg[0xfc] = 3;
	comp->reg[0xfd] = 2;
	comp->reg[0xfe] = 1;
	comp->reg[0xff] = 0;
	msxResetSlot(comp->slot);
	vdpReset(comp->vid);
	comp->vid->memMask = MEM_16K - 1;
	msxMapMem(comp);
}

// AY

void msxAYIdxOut(Computer* comp, unsigned short port, unsigned char val) {
	tsOut(comp->ts, 0xfffd, val);
}

void msxAYDataOut(Computer* comp, unsigned short port, unsigned char val) {
	tsOut(comp->ts, 0xbffd, val);
}

unsigned char msxAYDataIn(Computer* comp, unsigned short port) {
	unsigned char res = 0xff;
	if (comp->ts->curChip->curReg == 0x0e) {		// b7:tape in, b6:?, b0..5 = joystick u/d/l/r/fa/fb (0:active)
		res = 0x7f | (comp->tape->volPlay & 0x80);
	} else {
		res = tsIn(comp->ts, 0xfffd);
	}
	return res;
}

// 8255A

unsigned char msxA9In(Computer* comp, unsigned short port) {
	return comp->keyb->msxMap[comp->msx.keyLine];
	//return kbdRead(comp->keyb, comp->msx.keyLine);
}

void msxAAOut(Computer* comp, unsigned short port, unsigned char val) {
	comp->msx.pAA = val;
	comp->msx.keyLine = val & 0x0f;
	if (val & 0x10) {
		tapStop(comp->tape);
	} else {
		tapPlay(comp->tape);
	}
	comp->tape->levRec = (val & 0x20) ? 1 : 0;
	comp->beep->lev = (val & 0x80) ? 1 : 0;
}

unsigned char msxAAIn(Computer* comp, unsigned short port) {
	return comp->msx.pAA;
}

void msxABOut(Computer* comp, unsigned short port, unsigned char val) {
	if (val & 0x80) {
		comp->msx.ppi.regC = val;
	} else {
		unsigned char mask = 0x01 << ((val >> 1) & 7);
		if (val & 1) {
			msxAAOut(comp, port, comp->msx.pAA | mask);
		} else {
			msxAAOut(comp, port, comp->msx.pAA & ~mask);
		}
	}
}

// memory

void msxA8Out(Computer* comp, unsigned short port, unsigned char val) {
	comp->msx.pA8 = val;
	msxMapMem(comp);
}

unsigned char msxA8In(Computer* comp, unsigned short port) {
	return comp->msx.pA8;
}

void msxMemOut(Computer* comp, unsigned short port, unsigned char val) {
	comp->reg[port] = val;
}

// v9938

void msx9938wr(Computer* comp, unsigned short adr, unsigned char val) {
	vdpWrite(comp->vid, adr & 3, val);
}

unsigned char msx9938rd(Computer* comp, unsigned short adr) {
	return vdpRead(comp->vid, adr & 3);
}

// Port map

static xPort msxPortMap[] = {

	{0xff,0x90,2,2,2,dummyIn,	dummyOut},	// 90	RW	ULA5RA087 Centronic BUSY state (bit 1=1) / ULA5RA087 Centronic STROBE output (bit 0=0)
	{0xff,0x91,2,2,2,NULL,		dummyOut},	// 91	W	ULA5RA087 Centronic Printer Data

	{0xfc,0x88,2,2,2,msx9938rd,	msx9938wr},	// 88..8D	VDP9938 extension
	{0xfe,0x98,2,2,2,msx9938rd,	msx9938wr},	// 98/99	VDP9918

	{0xff,0xa0,2,2,2,NULL,		msxAYIdxOut},	// A0	W	I AY-3-8910 PSG Sound Generator Index
	{0xff,0xa1,2,2,2,NULL,		msxAYDataOut},	// A1	W	I AY-3-8910 PSG Sound Generator Data write
	{0xff,0xa2,2,2,2,msxAYDataIn,	NULL},		// A2	R	I AY-3-8910 PSG Sound Generator Data read

	{0xff,0xa8,2,2,2,msxA8In,	msxA8Out},	// A8	RW	I 8255A/ULA9RA041 PPI Port A Memory PSLOT Register (RAM/ROM)
	{0xff,0xa9,2,2,2,msxA9In,	NULL},		// A9	R	I 8255A/ULA9RA041 PPI Port B Keyboard column inputs
	{0xff,0xaa,2,2,2,msxAAIn,	msxAAOut},	// AA	RW	I 8255A/ULA9RA041 PPI Port C Kbd Row sel,LED,CASo,CASm
	{0xff,0xab,2,2,2,NULL,		msxABOut},	// AB	W	I 8255A/ULA9RA041 Mode select and I/O setup of A,B,C

	{0xfc,0xfc,2,2,2,NULL,	msxMemOut},		// FC..FF W	RAM pages for memBanks

//	{0x00,0x00,2,2,2,brkIn,brkOut}
	{0x00,0x00,2,2,2,dummyIn,dummyOut},
};

unsigned char msxIn(Computer* comp, unsigned short port, int dos) {
	return hwIn(msxPortMap, comp, port, dos);
}

void msxOut(Computer* comp, unsigned short port, unsigned char val, int dos) {
	hwOut(msxPortMap,comp, port, val, dos);
}

void msx_sync(Computer* comp, int ns) {
	int irq = (comp->vid->inth || comp->vid->intf) ? 1 : 0;
	if (irq && !(comp->cpu->intrq & Z80_INT)) {			// 0->1 : TESTED 20ms
		comp->cpu->intrq |= Z80_INT;
		comp->intVector = 0xff;
	} else if (!irq && (comp->cpu->intrq & Z80_INT)) {		// 1->0 : clear
		comp->cpu->intrq &= ~Z80_INT;
	}
	tsSync(comp->ts, ns);
	tapSync(comp->tape, ns);
}

void msx_keyp(Computer* comp, keyEntry ent) {
	kbdPress(comp->keyb, ent);
}

void msx_keyr(Computer* comp, keyEntry ent) {
	kbdRelease(comp->keyb, ent);
}

sndPair msx_vol(Computer* comp, sndVolume* sv) {
	int amp = 0;
	if (comp->tape->on)
		amp = (comp->tape->volPlay << 8) * sv->tape / 1600;
	sndPair vol;
	vol.left = amp;
	vol.right = amp;
	sndPair tv = aymGetVolume(comp->ts->chipA);
	vol.left += tv.left * sv->ay / 100;
	vol.right += tv.right * sv->ay / 100;
	return vol;
}
