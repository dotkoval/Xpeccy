#include "../spectrum.h"

void speReset(ZXComp* comp) {
	comp->p7FFD = 0x10;
}

void speMapMem(ZXComp* comp) {
	memSetBank(comp->mem,MEM_BANK0,MEM_ROM,(comp->dos) ? 1 : 0);
	memSetBank(comp->mem,MEM_BANK3,MEM_RAM,0);
}

// out

void spOutFE(ZXComp* comp, unsigned short port, unsigned char val) {
	comp->vid->nextbrd = val & 0x07;
	if (!comp->vid->border4t)
		comp->vid->brdcol = val & 0x07;
	comp->beeplev = (val & 0x10) ? 1 : 0;
	comp->tape->levRec = (val & 0x08) ? 1 : 0;
}

// in

unsigned char spIn1F(ZXComp* comp, unsigned short port) {
	return joyInput(comp->joy);
}

unsigned char spInFF(ZXComp* comp, unsigned short port) {
	return comp->vid->atrbyte;
}

xPort spePortMap[] = {
	{0x0001,0x00fe,2,2,2,xInFE,	spOutFE},
	{0xc002,0xfffd,2,2,2,xInFFFD,	xOutFFFD},
	{0xc002,0xbffd,2,2,2,NULL,	xOutBFFD},
	{0x0320,0xfadf,2,2,2,xInFADF,	NULL},
	{0x0720,0xfbdf,2,2,2,xInFBDF,	NULL},
	{0x0720,0xffdf,2,2,2,xInFFDF,	NULL},
	{0x0021,0x001f,0,2,2,spIn1F,	NULL},
	{0x0000,0x0000,0,2,2,spInFF,	NULL},		// all unknown ports is FF (nodos)
	{0x0000,0x0000,2,2,2,spInFF,	NULL}
};

void speOut(ZXComp* comp, unsigned short port, unsigned char val, int dos) {
	difOut(comp->dif, port, val, dos);
	hwOut(spePortMap, comp, port, val, dos);
}

unsigned char speIn(ZXComp* comp, unsigned short port, int dos) {
	unsigned char res;
	if (difIn(comp->dif, port, &res, dos)) return res;
	return hwIn(spePortMap, comp, port, dos);
}
