#include "../spectrum.h"

void p1mMapMem(ZXComp* comp) {
	if (comp->pEFF7 & 8) {
		memSetBank(comp->mem,MEM_BANK0,MEM_RAM,0);
	} else {
		memSetBank(comp->mem,MEM_BANK0,MEM_ROM,(comp->dos ? 2 : 0) | ((comp->rom) ? 1 : 0));
	}
	memSetBank(comp->mem,MEM_BANK3,MEM_RAM,(comp->p7FFD & 7) | ((comp->pEFF7 & 4) ? 0 : ((comp->p7FFD & 0x20) | ((comp->p7FFD & 0xc0) >> 3))));
}

// in

unsigned char p1mIn1F(ZXComp* comp, unsigned short port) {
	return joyInput(comp->joy);
}

unsigned char p1mInBFF7(ZXComp* comp, unsigned short port) {
	return (comp->pEFF7 & 0x80) ? cmsRd(comp) : 0xff;
}

// out

/*
void p1mOutFE(ZXComp* comp, unsigned short port, unsigned char val) {
	comp->vid->nextbrd = val & 0x07;
	if (!comp->vid->border4t) comp->vid->brdcol = val & 0x07;
	comp->beeplev = (val & 0x10) ? 1 : 0;
	comp->tape->levRec = (val & 0x08) ? 1 : 0;
}
*/

void p1mOut7FFD(ZXComp* comp, unsigned short port, unsigned char val) {
	if ((comp->pEFF7 & 4) && (comp->p7FFD & 0x20)) return;
	comp->rom = (val & 0x10) ? 1 : 0;
	comp->p7FFD = val;
	comp->vid->curscr = (val & 0x08) ? 7 : 5;
	p1mMapMem(comp);
}

void p1mOutBFF7(ZXComp* comp, unsigned short port, unsigned char val) {
	if (comp->pEFF7 & 0x80) cmsWr(comp,val);
}

void p1mOutDFF7(ZXComp* comp, unsigned short port, unsigned char val) {
	if (comp->pEFF7 & 0x80) comp->cmos.adr = val;
}

void p1mOutEFF7(ZXComp* comp, unsigned short port, unsigned char val) {
	comp->pEFF7 = val;
	vidSetMode(comp->vid,(val & 0x01) ? VID_ALCO : VID_NORMAL);
	zxSetFrq(comp, (val & 0x10) ? 7.0 : 3.5);
	p1mMapMem(comp);
}

xPort p1mPortMap[] = {
	{0x0003,0x00fe,2,2,2,xInFE,	xOutFE},
	{0x8002,0x7ffd,2,2,2,NULL,	p1mOut7FFD},
	{0xc002,0xbffd,2,2,2,NULL,	xOutBFFD},
	{0xc002,0xfffd,2,2,2,xInFFFD,	xOutFFFD},
	{0xf008,0xeff7,2,2,2,NULL,	p1mOutEFF7},
	{0xf008,0xbff7,2,2,2,p1mInBFF7,	p1mOutBFF7},
	{0xf008,0xdff7,2,2,2,NULL,	p1mOutDFF7},
	{0x05a3,0xfadf,0,2,2,xInFADF,	NULL},
	{0x05a3,0xfbdf,0,2,2,xInFBDF,	NULL},
	{0x05a3,0xffdf,0,2,2,xInFFDF,	NULL},
	{0x00ff,0x001f,0,2,2,p1mIn1F,	NULL},		// TODO : ORLY (x & FF = 1F)
	{0x0000,0x0000,2,2,2,NULL,	NULL}
};

void p1mOut(ZXComp* comp, unsigned short port, unsigned char val, int dos) {
	difOut(comp->dif, port, val, dos);
	hwOut(p1mPortMap, comp, port, val, dos);
}

unsigned char p1mIn(ZXComp* comp, unsigned short port, int dos) {
	unsigned char res = 0xff;
	if (difIn(comp->dif, port, &res, dos)) return res;
	res = hwIn(p1mPortMap, comp, port, dos);
	return res;
}
