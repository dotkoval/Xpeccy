#include <string.h>

#include "hardware.h"


// IO ports

// video modes:
// [mode 2 : ~80T][mode 3 : ~170T][mode 0 : ~206T] = 456T
// [mode 1] during VBlank

unsigned char gbIORd(Computer* comp, unsigned short port) {
	port &= 0x7f;
	unsigned char res = comp->gb.iomap[port];
	switch(port) {
// JOYSTICK
		case 0x00:
			switch(comp->gb.iomap[0] & 0x30) {
				case 0x10: res = (comp->gb.buttons & 0xf0) >> 4; break;
				case 0x20: res = comp->gb.buttons & 0x0f; break;
			}
			break;
// SERIAL
		case 0x01: res = 0xff; break;
		case 0x02: res = 0xff; break;
// TIMER
		case 0x04: break;
		case 0x05: break;
		case 0x06: break;
		case 0x07: break;
// SOUND
		case 0x1a: break;
		case 0x24: break;
		case 0x25: break;
		case 0x26:
			res = 0xf0;
			if (comp->gbsnd->ch1.on) res |= 1;
			if (comp->gbsnd->ch2.on) res |= 2;
			if (comp->gbsnd->ch3.on && comp->gbsnd->ch3on) res |= 4;
			if (comp->gbsnd->ch4.on) res |= 8;
			break;
// VIDEO
		case 0x40: break;
		case 0x41:
			res = (comp->vid->ray.y == comp->vid->gbc->lyc) ? 4 : 0;
			res |= comp->vid->gbc->mode & 3;
			break;
		case 0x42: break;
		case 0x43: break;
		case 0x44: res = comp->vid->ray.y; break;
		case 0x45: break;
		case 0x47: break;
		case 0x48: break;
		case 0x49: break;
		case 0x4a: break;
		case 0x4b: break;
// INTERRUPT
		case 0x0f:
			res = comp->cpu->intrq;
			break;
// GBC
		case 0x4d:
			res = res & 1;
			if (comp->cpu->speed)
				res |= 0x80;
			break;
		case 0x4f:
			res = comp->gb.vbank & 1;
			break;
		case 0x70:
			res = comp->gb.wbank & 7;
			break;
		default:
			printf("GB: in %.4X\n",port);
			assert(0);
			break;
	}
	return res;
}

extern xColor iniCol[4];
void setGrayScale(xColor pal[256], int base, unsigned char val) {
	pal[base] = iniCol[val & 3];
	pal[base + 1] = iniCol[(val >> 2) & 3];
	pal[base + 2] = iniCol[(val >> 4) & 3];
	pal[base + 3] = iniCol[(val >> 6) & 3];
}

void gbSetTone(gbsChan* gbch, int frq, int form) {
	// 1 tick = cpu.freq (Hz) / 32		!!!
	// 1e9 ns in one sec
	// ns = 1e9 / 128KHz * (2048 - frq)

	// int per = 1e9 / 131072 * (2048 - frq);			// full, ns
	int per = (2048 - frq);		// 2048-frq ticks @ 128KHz
	switch (form & 0xc0) {
		case 0x00:
			gbch->perL = per >> 3;		// 1/8
			gbch->perH = (per * 7) >> 3;	// 7/8
			break;
		case 0x40:
			gbch->perL = per >> 2;		// 1/4
			gbch->perH = (per * 3) >> 2;	// 3/4
			break;
		case 0x80:
			gbch->perL = per >> 1;		// 1/2
			gbch->perH = per >> 1;		// 1/2
			break;
		case 0xc0:
			gbch->perL = (per * 3) >> 2;	// 3/4
			gbch->perH = per >> 2;		// 1/4
			break;
	}
}

void gbSetEnv(gbsChan* gbch, unsigned char reg, unsigned char old) {
	unsigned char vol = (reg & 0xf0) >> 4;
	if (((reg & 7) == 0) && gbch->env.cnt) {
		vol++;
	} else if ((reg & 8) == 0) {
		vol += 2;
	}
	if ((reg ^ old) & 8) {
		vol = 16 - vol;
	}
	gbch->env.vol = vol & 15;
	gbch->env.dir = (reg & 0x08) ? 1 : 0;
	gbch->env.per = (reg & 7) ? (reg & 7) : 8;
	gbch->env.on = (reg & 7) ? 1 : 0;
}

void gbIOWr(Computer* comp, unsigned short port, unsigned char val) {
	port &= 0x7f;
	unsigned short sadr;
	unsigned short dadr;
	int frq;
	int per;
	int s,r;
	xColor col;
	// old env values
	unsigned char env1 = comp->gb.iomap[0x12];
	unsigned char env2 = comp->gb.iomap[0x17];
	unsigned char env4 = comp->gb.iomap[0x21];
	// snd chans ptr
	gbsChan* ch1 = &comp->gbsnd->ch1;
	gbsChan* ch2 = &comp->gbsnd->ch2;
	gbsChan* ch3 = &comp->gbsnd->ch3;
	gbsChan* ch4 = &comp->gbsnd->ch4;
	// gbc video ptr
	GBCVid* gbv = comp->vid->gbc;

	comp->gb.iomap[port] = val;
	switch (port) {
// JOYSTICK
		// b4: 0:select crest
		// b5: 0:select buttons
		case 0x00: break;
// VIDEO
		case 0x40:
			gbv->lcdon = (val & 0x80) ? 1 : 0;
			gbv->winmapadr = (val & 0x40) ? 0x9c00 : 0x9800;
			gbv->winen = (val & 0x20) ? 1 : 0;
			gbv->altile = (val & 0x10) ? 0 : 1;			// signed tile num
			gbv->tilesadr = (val & 0x10) ? 0x8000 : 0x8800;
			gbv->bgmapadr = (val & 0x08) ? 0x9c00 : 0x9800;
			gbv->bigspr = (val & 0x04) ? 1 : 0;
			gbv->spren  = (val & 0x02) ? 1 : 0;
			if (gbv->gbmode) {
				gbv->bgen = (val & 1);		// gbc in gb mode has different effect
			} else {
				gbv->bgprior = (val & 1);	// change bg/win priority
			}
			break;
		case 0x41:						// lcd stat interrupts enabling
			gbv->inten = val;
			break;
		case 0x42:
			gbv->sc.y = val;
			break;
		case 0x43:
			gbv->sc.x = val;
			break;
		case 0x44:						// TODO: writing will reset the counter (?)
			break;
		case 0x45:
			gbv->lyc = val;
			break;
		case 0x46:						// TODO: block CPU memory access for 160 microsec (except ff80..fffe)
			sadr = val << 8;
			dadr = 0;
			while (dadr < 0xa0) {
				gbv->oam[dadr] = memRd(comp->mem, sadr);
				sadr++;
				dadr++;
			}
			break;
		case 0x47:							// palete for bg/win
			if (gbv->gbmode) setGrayScale(gbv->pal, 0, val);
			break;
		case 0x48:
			if (gbv->gbmode) setGrayScale(gbv->pal, 0x40, val);	// object pal 0
			break;
		case 0x49:
			if (gbv->gbmode) setGrayScale(gbv->pal, 0x44, val);	// object pal 1
			break;
		case 0x4a:
			gbv->win.y = val;
			break;
		case 0x4b:
			gbv->win.x = val - 7;
			break;
// SOUND
// 1e9/frq(Hz) = full period ns
// 5e8/frq(Hz) = half period ns
// frq = 131072/(2048-N) Hz
// full period = 7630 * (2048-N) ns -> 15626240 max / 7630 min
// half period = 3815 * (2048-N) ns
		// chan 1 : tone,sweep,env
		case 0x10:
			per = (val & 0x70) >> 4;
			if (per == 0)
				per = 8;
			ch1->sweep.dir = (val & 8) ? 0 : 1;
			ch1->sweep.per = per;
			ch1->sweep.step = val & 7;
			break;
		case 0x11:
			ch1->dur = 64 - (val & 0x3f);
			frq = ((comp->gb.iomap[0x14] << 8) | comp->gb.iomap[0x13]) & 0x07ff;
			gbSetTone(ch1, frq, val);
			break;
		case 0x12:
			gbSetEnv(ch1, val, env1);
			break;
		case 0x13:
			frq = ((comp->gb.iomap[0x14] << 8) | val) & 0x07ff;
			gbSetTone(ch1, frq, comp->gb.iomap[0x11]);
			break;
		case 0x14:
			frq = ((val << 8) | (comp->gb.iomap[0x13])) & 0x07ff;
			gbSetTone(ch1, frq, comp->gb.iomap[0x11]);
			ch1->cont = (val & 0x40) ? 0 : 1;
			if (val & 0x80) {
				ch1->step = 0;
				ch1->env.vol = (comp->gb.iomap[0x12] >> 4) & 0x0f;
				ch1->env.cnt = ch1->env.per;
				ch1->sweep.cnt = ch1->sweep.per;
				ch1->cnt = ch1->lev ? ch1->perH : ch1->perL;
				if (!ch1->dur) ch1->dur = 64;
				ch1->on = 1;
			}
			break;
		// chan 2: tone,env
		case 0x16:
			ch2->dur = 64 - (val & 0x3f);
			frq = ((comp->gb.iomap[0x19] << 8) | comp->gb.iomap[0x18]) & 0x07ff;
			gbSetTone(ch2, frq, val);
			break;
		case 0x17:
			// printf("ch2 env %.2X\n",val);
			gbSetEnv(ch2, val, env2);
			break;
		case 0x18:
			frq = ((comp->gb.iomap[0x19] << 8) | val) & 0x07ff;
			gbSetTone(ch2, frq, comp->gb.iomap[0x16]);
			break;
		case 0x19:
			frq = ((val << 8) | comp->gb.iomap[0x18]) & 0x07ff;
			gbSetTone(ch2, frq, comp->gb.iomap[0x16]);
			ch2->cont = (val & 0x40) ? 0 : 1;
			if (val & 0x80) {
				ch2->step = 0;
				ch2->env.vol = (comp->gb.iomap[0x17] >> 4) & 0x0f;
				ch2->env.cnt = ch2->env.per;
				ch2->cnt = ch2->lev ? ch2->perH : ch2->perL;
				if (!ch2->dur) ch2->dur = 64;
				ch2->on = 1;
			}
			break;
		// chan 3 : wave
		case 0x1a:
			comp->gbsnd->ch3on = (val & 0x80) ? 1 : 0;
			break;
		case 0x1b:
			ch3->dur = 256 - val;		// 256-x @ 256Hz
			break;
		case 0x1c:
			comp->gbsnd->ch3vol = (val >> 5) & 3;
			break;
		case 0x1d:
			frq = ((comp->gb.iomap[0x1e] << 8) | val) & 0x07ff;
			per = (2048 - frq) >> 5;	// 2048-frq @ 64KHz | << 1 @ 128KHz. / 32 for 1 sample
			ch3->perH = per;
			ch3->perL = per;
			break;
		case 0x1e:
			frq = ((val << 8) | comp->gb.iomap[0x1d]) & 0x07ff;
			per = (2048 - frq) >> 5;
			ch3->perH = per;
			ch3->perL = per;
			ch3->cont = (val & 0x40) ? 0 : 1;
			if (val & 0x80) {
				ch3->step = 0;
				ch3->cnt = ch3->perH;
				if (!ch3->dur) ch3->dur = 256;
				ch3->on = 1;
			}
			break;
		// chan 4
		case 0x20:
			ch4->dur = (64 - (val & 0x3f));		// 64-(val&63) @ 256Hz
			break;
		case 0x21:
			gbSetEnv(ch4, val, env4);
			break;
		case 0x22:
			r = val & 7;
			s = (val & 0xf0) >> 4;
			// frq = 512KHz / r / 2^(s+1)		512KHz = 2^19 Hz
			// r*(2^(s+1)) ticks @ 512KHz
			// :4 ticks @ 128KHz
			frq = r ? (r * (2 ^ (s + 1))) : (2 ^ s);
			if (val & 8) {			// FIXME: find a real way
				per = frq << 2;
			} else {
				per = frq >> 2;
			}
			ch4->perH = per >> 1;
			ch4->perL = per >> 1;
			break;
		case 0x23:
			ch4->cont = (val & 0x40) ? 0 : 1;
			if (val & 0x80) {
				ch4->step = 0;
				ch4->env.vol = (comp->gb.iomap[0x21] >> 4) & 0x0f;
				ch4->env.cnt = ch4->env.per;
				ch4->cnt = ch4->perH;
				if (!ch4->dur) ch4->dur = 64;
				ch4->on = 1;
			}
			break;
		// control
		case 0x24:		// Vin sound channel control (not used?)
			break;
		case 0x25:		// send chan sound to SO1/SO2. b0..3 : ch1..4 -> SO1; b4..7 : ch1..4 -> SO2
			ch1->so1 = (val & 0x01) ? 1 : 0;
			ch2->so1 = (val & 0x02) ? 1 : 0;
			ch3->so1 = (val & 0x04) ? 1 : 0;
			ch4->so1 = (val & 0x08) ? 1 : 0;
			ch1->so2 = (val & 0x10) ? 1 : 0;
			ch2->so2 = (val & 0x20) ? 1 : 0;
			ch3->so2 = (val & 0x40) ? 1 : 0;
			ch4->so2 = (val & 0x80) ? 1 : 0;
			break;
		case 0x26:		// on/off channels
			comp->gbsnd->on = (val & 0x80) ? 1 : 0;
			break;
// TIMER
		case 0x04:				// divider. inc @ 16384Hz
			comp->gb.iomap[4] = 0x00;
			break;
		case 0x05:				// custom timer (see 07)
			break;
		case 0x06:				// custom timer init value @ overflow
			break;
		case 0x07:
			// custom timer control
			// b2 : timer on
			// b0,1 : 00 = 4KHz, 01 = 256KHz, 10 = 64KHz, 11 = 16KHz
			switch (val & 3) {
				case 0: comp->gb.timer.t.per = comp->nsPerTick << 10; break;
				case 1: comp->gb.timer.t.per = comp->nsPerTick << 4; break;
				case 2: comp->gb.timer.t.per = comp->nsPerTick << 6; break;
				case 3: comp->gb.timer.t.per = comp->nsPerTick << 8; break;
			}
			comp->gb.timer.t.on = (val & 4) ? 1 : 0;
			break;
// SERIAL
		case 0x01:
			break;
		case 0x02:
			break;
// INT
		case 0x0f:				// interrupt requesting
			comp->cpu->intrq |= val;	// add to cpu int req
			break;
// GBC
//	cpu speed
		case 0x4d:
			comp->cpu->speedrq = (val & 1);
			break;
//	memory mapping
		case 0x4f:				// VRAM bank (8000..9fff)
			comp->gb.vbank = val & 1;
			break;
		case 0x70:				// WRAM bank (D000..DFFF)
			val &= 7;
			if (val == 0)
				val = 1;
			comp->gb.wbank = val;
			break;
//	palette
		case 0x68:				// gbc bg palete index (b0..5) & autoincrement (b7)
			break;
		case 0x69:				// gbc lo/hi palete value
			per = comp->gb.iomap[0x68];
			frq = ((per & 0x3e) >> 1);	// color idx
			col = gbv->pal[frq];		// current color;
			if (per & 1) {			// B/g
				col.b = (val & 0x7c) << 1;
				col.g = (col.g & 0x3f) | ((val & 3) << 6);
			} else {			// g/R
				col.r = (val & 0x1f) << 3;
				col.g = (col.g & 0xc0) | ((val & 0xe0) >> 2);
			}
			gbv->pal[frq] = col;
			if (per & 0x80) {
				comp->gb.iomap[0x68] = (per & 0xc0) | ((per + 1) & 0x3f);
			}
			break;
		case 0x6a:
			break;
		case 0x6b:
			per= comp->gb.iomap[0x6a];
			frq = 0x40 | ((per & 0x3e) >> 1);
			col = gbv->pal[frq];
			if (per & 1) {
				col.b = (val & 0x7c) << 1;
				col.g = (col.g & 0x3f) | ((val & 3) << 6);
			} else {			// g/R
				col.r = (val & 0x1f) << 3;
				col.g = (col.g & 0xc0) | ((val & 0xe0) >> 2);
			}
			gbv->pal[frq] = col;
			if (per & 0x80) {
				comp->gb.iomap[0x6a] = (per & 0xc0) | ((per + 1) & 0x3f);
			}
			break;
//	dma
		case 0x51: comp->dma.src.h = val; break;
		case 0x52: comp->dma.src.l = val & 0xf0; break;		// low 4 bits = 0
		case 0x53: comp->dma.dst.h = val & 0x1f; break;		// dst is vram, hi 3 bits ignored
		case 0x54: comp->dma.dst.l = val & 0xf0; break;
		case 0x55:
			sadr = (comp->dma.src.h << 8) | comp->dma.src.l;	// rom/ram adr
			//if ((sadr & 0xe000) == 0x8000) break;	// not vram
			//if (sadr > 0xdfff) break;		// not wram
			dadr = (comp->dma.dst.h << 8) | comp->dma.dst.l;	// vram adr (0000...1ff0)
			per = ((val & 0x7f) + 1) << 4;				// bytes to transfer (10..800)
			while (per > 0) {
				memWr(comp->mem, 0x8000 | (dadr & 0x1fff), memRd(comp->mem, sadr));
				dadr++;
				sadr++;
				per--;
			}
			break;
//	misc
		case 0x4c:
			break;
		case 0x56:			// IR port
			break;
		case 0x6c:			// b0: 1:GB mode, 0:GBC mode
			comp->vid->gbc->gbmode = val & 1;
			break;
// MISC
		case 0x50:
			comp->gb.boot = 0;
			break;

		default:
			if ((port & 0xf0) == 0x30) {	// ff30..ff3f : wave pattern ram for chan 3
				dadr = (port & 0x0f) << 1;
				comp->gbsnd->wave[dadr++] = (val & 0xf0) | ((val & 0xf0) >> 4);		// HH : high 4 bits
				comp->gbsnd->wave[dadr] = (val & 0x0f) | ((val & 0x0f) << 4);		// LL : low 4 bits
			} else {
				printf("GB: out %.4X,%.2X\n",port,val);
				assert(0);
			}
			break;
	}
}

// 0000..7fff : slot

unsigned char gbSlotRd(unsigned short adr, void* data) {
	Computer* comp = (Computer*)data;
	xCartridge* slot = comp->slot;
	unsigned char res = 0xff;
	if (comp->gb.boot && (adr < comp->romsize) && ((adr & 0xff00) != 0x0100)) {
		res = comp->mem->romData[adr & 0x3fff];
	} else if (slot->data) {
		res = slot->core->rd(slot, adr);
	}
	return res;
}

void gbSlotWr(unsigned short adr, unsigned char val, void* data) {
	Computer* comp = (Computer*)data;
	xCartridge* slot = comp->slot;
	if (slot->data && slot->core->wr)
		slot->core->wr(slot, adr, val);
	memSetBank(comp->mem, MEM_BANK1, MEM_SLOT, comp->slot->memMap[0], gbSlotRd, gbSlotWr, comp);
	//gbMaper(comp);
}

// 8000..9fff : video mem (bank 0,1 gbc)
// a000..bfff : external ram

unsigned char gbvRd(unsigned short adr, void* data) {
	Computer* comp = (Computer*)data;
	xCartridge* slot = comp->slot;
	unsigned char res = 0xff;
	int radr;
	if (adr & 0x2000) {		// cartrige ram
		if (slot->data) {
			res = slot->core->rd(slot, adr);
		}
	} else {			// video ram
		radr = (comp->gb.vbank << 13) | (adr & 0x1fff);
		res = comp->vid->gbc->ram[radr];
	}
	return res;
}

void gbvWr(unsigned short adr, unsigned char val, void* data) {
	Computer* comp = (Computer*)data;
	xCartridge* slot = comp->slot;
	int radr;
	if (adr & 0x2000) {		// cartrige ram
		if (slot->data) {
			slot->core->wr(slot, adr, val);
		}
	} else {			// video ram
		radr = (comp->gb.vbank << 13) | (adr & 0x1fff);
		comp->vid->gbc->ram[radr & 0x3fff] = val;
	}
}

// c000..ffff : internal ram, oem, iomap, int mask

// C000..DFFF : RAM1 : C000..CFFF page 0, D000..DFFF page X
// E000..FDFF : mirror RAM1
// FE00..FE9F : OAM (sprites data)
// FEA0..FEFF : not used
// FF00..FF7F : IOmap
// FF80..FFFE : RAM2
// FFFF..FFFF : INT mask

unsigned char gbrRd(unsigned short adr, void* data) {
	Computer* comp = (Computer*)data;
	unsigned char res = 0xff;
	if (adr < 0xfe00) {
		adr &= 0x1fff;						// 8K, [e000...fdff] -> [c000..ddff]
		if (adr & 0x1000) {					// high 4K -> WRAM page X
			adr =(comp->gb.wbank << 12) | (adr & 0xfff);
			res = comp->mem->ramData[adr];
		} else {						// low 4K -> WRAM page 0
			res = comp->mem->ramData[adr & 0xfff];
		}
	} else if (adr < 0xfea0) {					// video oam not accessible @ mode 2
		if (comp->vid->gbc->mode != 2)
			res = comp->vid->gbc->oam[adr & 0xff];
	} else if (adr < 0xff00) {
		res = 0xff;						// unused
	} else if (adr < 0xff80) {
		res = gbIORd(comp, adr);				// io rd
	} else if (adr < 0xffff) {
		res = comp->gb.iram[adr & 0xff];			// ram2
	} else {
		res = comp->cpu->inten;					// int mask
	}
	return res;
}

void gbrWr(unsigned short adr, unsigned char val, void* data) {
	Computer* comp = (Computer*)data;
	if (adr < 0xfe00) {
		adr &= 0x1fff;
		if (adr & 0x1000) {					// high 4K -> WRAM page
			adr = (comp->gb.wbank << 12) | (adr & 0xfff);
			comp->mem->ramData[adr] = val;
		} else {
			comp->mem->ramData[adr & 0xfff] = val;
		}
	} else if (adr < 0xfea0) {					// video oam not accessible @ mode 2
		if (comp->vid->gbc->mode != 2)
			comp->vid->gbc->oam[adr & 0xff] = val;
	} else if (adr < 0xff00) {
		// nothing
	} else if (adr < 0xff80) {
		gbIOWr(comp, adr, val);
	} else if (adr < 0xffff) {
		comp->gb.iram[adr & 0xff] = val;
	} else {
		comp->cpu->inten = val;
	}
}

// maper

void gbMaper(Computer* comp) {
	memSetBank(comp->mem, MEM_BANK0, MEM_SLOT, 0, gbSlotRd, gbSlotWr, comp);
	memSetBank(comp->mem, MEM_BANK1, MEM_SLOT, comp->slot->memMap[0], gbSlotRd, gbSlotWr, comp);
	memSetBank(comp->mem, MEM_BANK2, MEM_EXT, 0, gbvRd, gbvWr, comp);	// VRAM (8K), slot ram (8K)
	memSetBank(comp->mem, MEM_BANK3, MEM_EXT, 1, gbrRd, gbrWr, comp);	// internal RAM/OAM/IOMap
}

unsigned char gbMemRd(Computer* comp, unsigned short adr, int m1) {
	return memRd(comp->mem, adr);
}

void gbMemWr(Computer* comp, unsigned short adr, unsigned char val) {
	memWr(comp->mem, adr, val);
}

// collect interrupt requests & handle interrupt

void gbcSync(Computer* comp, long ns) {
	unsigned char req = 0;
	if (comp->vid->vbstrb) {
		comp->vid->vbstrb = 0;
		req |= 1;
	} else if (comp->vid->gbc->intrq) {
		comp->vid->gbc->intrq = 0;
		req |= 2;
	} else if (comp->gb.timer.t.intrq) {
		comp->gb.timer.t.intrq = 0;
		req |= 4;
	} else if (0) {				// TODO: serial INT (?)
		req |= 8;
	} else if (comp->gb.inpint) {
		comp->gb.inpint = 0;
		req |= 16;
	}
	comp->cpu->intrq |= req;		// cpu int req
//	if (comp->cpu->iff1 && (comp->cpu->intrq & 0x1f)) //comp->cpu->inten))
//		comp->cpu->inth = 1;
}

// keypress

typedef struct {
	signed int id;
	int mask;
} gbKey;

gbKey gbKeyMap[8] = {
	{XKEY_RIGHT,1},
	{XKEY_LEFT,2},
	{XKEY_UP,4},
	{XKEY_DOWN,8},
	{XKEY_Z,16},
	{XKEY_X,32},
	{XKEY_SPACE,64},
	{XKEY_ENTER,128}
};

unsigned char gbGetInputMask(signed int keyid) {
	int idx = 0;
	unsigned char mask = 0;
	while (idx < 8) {
		if (gbKeyMap[idx].id == keyid) {
			mask = gbKeyMap[idx].mask;
		}
		idx++;
	}
	return mask;
}

char gbMsgBG0[] = " BG layer off ";
char gbMsgBG1[] = " BG layer on ";
char gbMsgWIN0[] = " WIN layer off ";
char gbMsgWIN1[] = " WIN layer on ";
char gbMsgSPR0[] = " SPR layer off ";
char gbMsgSPR1[] = " SPR layer on ";

void gbc_keyp(Computer* comp, keyEntry ent) {
	unsigned char mask = gbGetInputMask(ent.key);
	if (mask) {
		comp->gb.buttons &= ~mask;
		comp->gb.inpint = 1;			// input interrupt request
	} else if (ent.key == XKEY_1) {
		comp->vid->gbc->bgblock ^= 1;
		comp->msg = comp->vid->gbc->bgblock ? gbMsgBG0 :gbMsgBG1;
	} else if (ent.key == XKEY_2) {
		comp->vid->gbc->winblock ^= 1;
		comp->msg = comp->vid->gbc->winblock ? gbMsgWIN0 :gbMsgWIN1;
	} else if (ent.key == XKEY_3) {
		comp->vid->gbc->sprblock ^= 1;
		comp->msg = comp->vid->gbc->sprblock ? gbMsgSPR0 :gbMsgSPR1;
	}
}

void gbc_keyr(Computer* comp, keyEntry ent) {
	int mask = gbGetInputMask(ent.key);
	if (mask == 0) return;
	comp->gb.buttons |= mask;
}

// reset

void gbReset(Computer* comp) {
	comp->gb.boot = 1;
	comp->vid->gbc->gbmode = 0;
	vidSetMode(comp->vid, VID_GBC);
	gbcvReset(comp->vid->gbc);

	compSetTurbo(comp, 1);
	comp->cpu->inten = 0;
	comp->vid->gbc->inten = 0;
	comp->gb.buttons = 0xff;

	comp->gbsnd->ch1.on = 0;
	comp->gbsnd->ch2.on = 0;
	comp->gbsnd->ch3.on = 0;
	comp->gbsnd->ch4.on = 0;

	comp->gb.timer.t.per = 0;
	comp->gb.timer.t.on = 0;

	comp->gb.vbank = 0;	// vram page
	comp->gb.wbank = 1;	// wram page (D000..DFFF)

	xCartridge* slot = comp->slot;
	slot->memMap[0] = 1;	// rom bank
	slot->memMap[1] = 0;	// ram bank
	slot->ramen = 0;
	slot->ramMask = 0x7fff;
	slot->ramod = 0;
	if (slot->data) {
		unsigned char type = slot->data[0x147];		// slot type
		printf("Cartrige type %.2X\n",type);
		switch (type) {
			case 0x00:
				sltSetMaper(slot, MAP_GB_NOMAP);	// rom only (up to 32K)
				break;
			case 0x01:
			case 0x02:
			case 0x03:
				sltSetMaper(slot, MAP_GB_MBC1);		// mbc1
				break;
			case 0x05:
			case 0x06:
				sltSetMaper(slot, MAP_GB_MBC2);		// mbc2
				break;
			case 0x0f:
			case 0x10:
			case 0x11:
			case 0x12:
			case 0x13:
				sltSetMaper(slot, MAP_GB_MBC3);		// mbc3
				break;
			case 0x19:
			case 0x1a:
			case 0x1b:
			case 0x1c:
			case 0x1d:
			case 0x1e:
				slot->memMap[0] = 0;
				sltSetMaper(slot, MAP_GB_MBC5);		// mbc5
				break;
			default:
				sltSetMaper(slot, MAP_UNKNOWN);
				break;
		}

	}
}
