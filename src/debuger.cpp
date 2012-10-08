#include "xcore/xcore.h"

#include "debuger.h"
#include "emulwin.h"

#include "z80ex_dasm.h"

#include <QIcon>
#include <QDebug>

DebugWin* dbgWin;

unsigned long lastDbgTicks = 0;

void dbgInit(QWidget* par) {
	dbgWin = new DebugWin(par);
}

// TODO: extract from DebugWindow almost all

void dbgShow() {
	dbgWin->start();
}

bool dbgIsActive() {
	return dbgWin->active;
}

// OBJECT

void DebugWin::start() {
	emulPause(true,PR_DEBUG);
	ledit->hide();
	active = true;
	upadr = z80ex_get_reg(zx->cpu,regPC);
	curcol = 3;
	currow = 0;
	fillall();
	show();
	lastDbgTicks = zx->tickCount;
	vidFlag |= VF_FRAMEDBG;
	vidDarkTail(zx->vid);
}

void DebugWin::stop() {
	vidFlag &= ~VF_FRAMEDBG;
	ledit->hide();
	emulExec();
	hide();
	active = false;
	emulPause(false,PR_DEBUG);
}

void DebugWin::reject() {stop();}

#define DASMROW 25
#define DMPSIZE 16

DebugWin::DebugWin(QWidget* par):QDialog(par) {
	QLabel *lab;
	int i,j;
	QVBoxLayout *mlay = new QVBoxLayout;
	QHBoxLayout *alay = new QHBoxLayout;
		QVBoxLayout *llay = new QVBoxLayout;
			QGroupBox *regbox = new QGroupBox("Registers");
				rglay = new QGridLayout;
					rglay->setVerticalSpacing(0);
					lab = new QLabel("AF"); rglay->addWidget(lab,0,0); lab = new QLabel; rglay->addWidget(lab,0,1); lab->setAutoFillBackground(true);
					lab = new QLabel("BC"); rglay->addWidget(lab,1,0); lab = new QLabel; rglay->addWidget(lab,1,1); lab->setAutoFillBackground(true);
					lab = new QLabel("DE"); rglay->addWidget(lab,2,0); lab = new QLabel; rglay->addWidget(lab,2,1); lab->setAutoFillBackground(true);
					lab = new QLabel("HL"); rglay->addWidget(lab,3,0); lab = new QLabel; rglay->addWidget(lab,3,1); lab->setAutoFillBackground(true);
					lab = new QLabel("AF'"); rglay->addWidget(lab,4,0); lab = new QLabel; rglay->addWidget(lab,4,1); lab->setAutoFillBackground(true);
					lab = new QLabel("BC'"); rglay->addWidget(lab,5,0); lab = new QLabel; rglay->addWidget(lab,5,1); lab->setAutoFillBackground(true);
					lab = new QLabel("DE'"); rglay->addWidget(lab,6,0); lab = new QLabel; rglay->addWidget(lab,6,1); lab->setAutoFillBackground(true);
					lab = new QLabel("HL'"); rglay->addWidget(lab,7,0); lab = new QLabel; rglay->addWidget(lab,7,1); lab->setAutoFillBackground(true);
					lab = new QLabel("IX"); rglay->addWidget(lab,8,0); lab = new QLabel; rglay->addWidget(lab,8,1); lab->setAutoFillBackground(true);
					lab = new QLabel("IY"); rglay->addWidget(lab,9,0); lab = new QLabel; rglay->addWidget(lab,9,1); lab->setAutoFillBackground(true);
					lab = new QLabel("IR"); rglay->addWidget(lab,10,0); lab = new QLabel; rglay->addWidget(lab,10,1); lab->setAutoFillBackground(true);
					lab = new QLabel("PC"); rglay->addWidget(lab,11,0); lab = new QLabel; rglay->addWidget(lab,11,1); lab->setAutoFillBackground(true);
					lab = new QLabel("SP"); rglay->addWidget(lab,12,0); lab = new QLabel; rglay->addWidget(lab,12,1); lab->setAutoFillBackground(true);
					lab = new QLabel("IM"); rglay->addWidget(lab,13,0); lab = new QLabel; rglay->addWidget(lab,13,1); lab->setAutoFillBackground(true);
					rglay->setColumnStretch(1,10);
					rglay->setRowStretch(100,10);
					rowincol[0] = 13;
				regbox->setLayout(rglay);
			QGroupBox *raybox = new QGroupBox("Mem");
				raylay = new QGridLayout;
					lab = new QLabel("RAM"); raylay->addWidget(lab,0,0); lab = new QLabel; raylay->addWidget(lab,0,1);
					lab = new QLabel("ROM"); raylay->addWidget(lab,1,0); lab = new QLabel; raylay->addWidget(lab,1,1);
				raybox->setLayout(raylay);
			llay->addWidget(regbox);
			llay->addWidget(raybox);
			llay->addStretch(10);
		QGroupBox *asmbox = new QGroupBox("Disasm");
			asmlay = new QGridLayout;
				asmlay->setHorizontalSpacing(0);
				asmlay->setVerticalSpacing(0);
				for (i=0;i<DASMROW;i++) {
					lab = new QLabel; asmlay->addWidget(lab,i,0);
						lab->setAutoFillBackground(true);
					lab = new QLabel; asmlay->addWidget(lab,i,1);
						lab->setAutoFillBackground(true);
					lab = new QLabel; asmlay->addWidget(lab,i,2);
						lab->setAutoFillBackground(true);
				}
				rowincol[1] = rowincol[2] = rowincol[3] = DASMROW-1;
				asmlay->setColumnMinimumWidth(0,40);
				asmlay->setColumnMinimumWidth(1,80);
				asmlay->setColumnMinimumWidth(2,130);
			asmbox->setLayout(asmlay);
		QVBoxLayout *rlay = new QVBoxLayout;
			QGroupBox *dmpbox = new QGroupBox("Memory");
				dmplay = new QGridLayout;
				dmplay->setVerticalSpacing(0);
					for (i=0; i<DMPSIZE; i++) {
						lab = new QLabel; dmplay->addWidget(lab,i,0);
						lab->setAutoFillBackground(true);
						for (j=1; j<9; j++) {
							lab = new QLabel;
							lab->setAutoFillBackground(true);
							dmplay->addWidget(lab,i,j);
						}
					}
					for (i=4; i<13; i++) rowincol[i]=DMPSIZE-1;
				dmplay->setColumnMinimumWidth(0,40);
				dmpbox->setLayout(dmplay);
			QGroupBox *vgbox = new QGroupBox("BDI");
				vglay = new QGridLayout;
				vglay->setVerticalSpacing(0);
				lab = new QLabel("VG.TRK"); vglay->addWidget(lab,0,0); lab = new QLabel; vglay->addWidget(lab,0,1);
				lab = new QLabel("VG.SEC"); vglay->addWidget(lab,1,0); lab = new QLabel; vglay->addWidget(lab,1,1);
				lab = new QLabel("VG.DAT"); vglay->addWidget(lab,2,0); lab = new QLabel; vglay->addWidget(lab,2,1);
				lab = new QLabel("EmulVG.Com"); vglay->addWidget(lab,3,0); lab = new QLabel; vglay->addWidget(lab,3,1);
				lab = new QLabel("EmulVG.Wait"); vglay->addWidget(lab,4,0); lab = new QLabel; vglay->addWidget(lab,4,1);
				lab = new QLabel("VG.floppy"); vglay->addWidget(lab,5,0); lab = new QLabel; vglay->addWidget(lab,5,1);
				lab = new QLabel("VG.com"); vglay->addWidget(lab,6,0); lab = new QLabel; vglay->addWidget(lab,6,1);
				lab = new QLabel("FLP.TRK"); vglay->addWidget(lab,0,2); lab = new QLabel; vglay->addWidget(lab,0,3);
				lab = new QLabel("FLP.SID"); vglay->addWidget(lab,1,2); lab = new QLabel; vglay->addWidget(lab,1,3);
				lab = new QLabel("FLP.POS"); vglay->addWidget(lab,2,2); lab = new QLabel; vglay->addWidget(lab,2,3);
				lab = new QLabel("FLP.DAT"); vglay->addWidget(lab,3,2); lab = new QLabel; vglay->addWidget(lab,3,3);
				lab = new QLabel("FLP.FLD"); vglay->addWidget(lab,4,2); lab = new QLabel; vglay->addWidget(lab,4,3);
				vgbox->setLayout(vglay);
			rlay->addWidget(dmpbox);
			rlay->addWidget(vgbox);
			rlay->addStretch(10);
		alay->addLayout(llay);
		alay->addWidget(asmbox);
		alay->addLayout(rlay,10);
	mlay->addLayout(alay);
		QHBoxLayout *dlay = new QHBoxLayout;
			tlab = new QLabel;
			dlay->addWidget(tlab);
			dlay->addStretch(10);
	mlay->addLayout(dlay);
	setLayout(mlay);

	t = 0;
	active = false;
	curcol = 1;
	currow = 0;
	dmpadr = 0;
	cpoint.active = false;

	ledit = new QLineEdit(this);
	ledit->setWindowModality(Qt::ApplicationModal);

	QPalette pal;
	pal.setColor(QPalette::ToolTipBase,QColor(128,255,128));
	pal.setColor(QPalette::Highlight,QColor(255,128,128));
	pal.setColor(QPalette::ToolTipText,QColor(255,0,0));
	setPalette(pal);

	setWindowTitle("Xpeccy deBUGa");
	setWindowIcon(QIcon(":/images/bug.png"));
	setFont(QFont("Monospace",9));
	setModal(true);
	setFixedSize(640,480);
}

QString gethexword(int num) {return QString::number(num+0x10000,16).right(4).toUpper();}
QString gethexbyte(uchar num) {return QString::number(num+0x100,16).right(2).toUpper();}

bool DebugWin::fillall() {
	fillregz();
	filldump();
	fillrays();
	fillvg();
	filltick();
	return filldasm();
}

void DebugWin::filltick() {
	QString text = QString::number(zx->tickCount - lastDbgTicks).append(" | ");
	text.append(QString::number(zx->vid->matrix[zx->vid->dotCount].tick));
	tlab->setText(text);
}

void DebugWin::fillvg() {
	QLabel* lab;
	Floppy* flp = zx->bdi->fdc->fptr;	// current floppy
	lab = (QLabel*)vglay->itemAtPosition(0,1)->widget(); lab->setText(QString::number(zx->bdi->fdc->trk));
	lab = (QLabel*)vglay->itemAtPosition(1,1)->widget(); lab->setText(QString::number(zx->bdi->fdc->sec));
	lab = (QLabel*)vglay->itemAtPosition(2,1)->widget(); lab->setText(QString::number(zx->bdi->fdc->data));
	lab = (QLabel*)vglay->itemAtPosition(3,1)->widget();
	if (zx->bdi->fdc->wptr == NULL) {
		lab->setText("NULL");
	} else {
		lab->setText(QString::number(zx->bdi->fdc->cop,16));
	}
	lab = (QLabel*)vglay->itemAtPosition(4,1)->widget(); lab->setText(QString::number(zx->bdi->fdc->count));
	lab = (QLabel*)vglay->itemAtPosition(5,1)->widget(); lab->setText(QString::number(zx->bdi->fdc->fptr->id));
	lab = (QLabel*)vglay->itemAtPosition(6,1)->widget(); lab->setText(QString::number(zx->bdi->fdc->com,16));
	lab = (QLabel*)vglay->itemAtPosition(0,3)->widget(); lab->setText(QString::number(flp->trk));
	lab = (QLabel*)vglay->itemAtPosition(1,3)->widget(); lab->setText(zx->bdi->fdc->side ? "1" : "0");
	lab = (QLabel*)vglay->itemAtPosition(2,3)->widget(); lab->setText(QString::number(flp->pos));
	lab = (QLabel*)vglay->itemAtPosition(3,3)->widget(); lab->setText(QString::number(flpRd(flp),16));
	lab = (QLabel*)vglay->itemAtPosition(4,3)->widget(); lab->setText(QString::number(flp->field));
}

void DebugWin::fillrays() {
	QLabel *lab;
	lab = (QLabel*)raylay->itemAtPosition(0,1)->widget();
	lab->setText(QString::number(zx->mem->cram));
	lab = (QLabel*)raylay->itemAtPosition(1,1)->widget();
	lab->setText(QString::number(zx->mem->crom));
}

void DebugWin::filldump() {
	int i,j;
	adr = dmpadr;
	QLabel *lab;
	for (i=0; i<DMPSIZE; i++) {
		lab = (QLabel*)dmplay->itemAtPosition(i,0)->widget(); lab->setText(gethexword(adr));
		lab->setBackgroundRole((curcol==4 && currow==i)?QPalette::Highlight:QPalette::Window);
		for (j=1; j<9; j++) {
			lab = (QLabel*)dmplay->itemAtPosition(i,j)->widget();
			lab->setText(gethexbyte(memRd(zx->mem,adr)));
			adr++;
			lab->setBackgroundRole((curcol==4+j && currow==i)?QPalette::Highlight:QPalette::Window);
		}
	}
}

void DebugWin::fillregz() {
	QLabel *lab;
	lab = (QLabel*)rglay->itemAtPosition(0,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regAF)));	// af
		lab->setBackgroundRole((curcol==0 && currow==0)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(1,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regBC)));	// bc
		lab->setBackgroundRole((curcol==0 && currow==1)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(2,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regDE)));	// de
		lab->setBackgroundRole((curcol==0 && currow==2)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(3,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regHL)));	// hl
		lab->setBackgroundRole((curcol==0 && currow==3)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(4,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regAF_)));	// af'
		lab->setBackgroundRole((curcol==0 && currow==4)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(5,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regBC_)));	// bc'
		lab->setBackgroundRole((curcol==0 && currow==5)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(6,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regDE_)));	// de'
		lab->setBackgroundRole((curcol==0 && currow==6)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(7,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regHL_)));	// hl'
		lab->setBackgroundRole((curcol==0 && currow==7)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(8,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regIX)));	// ix
		lab->setBackgroundRole((curcol==0 && currow==8)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(9,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regIY)));	// iy
		lab->setBackgroundRole((curcol==0 && currow==9)?QPalette::Highlight:QPalette::Window);
	Z80EX_WORD ir = (z80ex_get_reg(zx->cpu,regI) << 8) | (z80ex_get_reg(zx->cpu,regR) & 0x7f) | (z80ex_get_reg(zx->cpu,regR7) & 0x80);
	lab = (QLabel*)rglay->itemAtPosition(10,1)->widget(); lab->setText(gethexword(ir));	// ir
		lab->setBackgroundRole((curcol==0 && currow==10)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(11,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regPC)));		// pc
		lab->setBackgroundRole((curcol==0 && currow==11)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(12,1)->widget(); lab->setText(gethexword(z80ex_get_reg(zx->cpu,regSP)));	// sp
		lab->setBackgroundRole((curcol==0 && currow==12)?QPalette::Highlight:QPalette::Window);
	lab = (QLabel*)rglay->itemAtPosition(13,1)->widget(); lab->setText(QString::number(z80ex_get_reg(zx->cpu,regIM)).append(" ").append(QString::number(z80ex_get_reg(zx->cpu,regIFF1))).append(QString::number(z80ex_get_reg(zx->cpu,regIFF2))));
		lab->setBackgroundRole((curcol==0 && currow==13)?QPalette::Highlight:QPalette::Window);
}

Z80EX_BYTE rdbyte(Z80EX_WORD adr,void*) {
	return memRd(zx->mem,adr);
}

DasmRow DebugWin::getdisasm() {
	int t1,t2,clen;
	DasmRow res;
	res.adr = adr;
	res.bytes = "";
	res.dasm = "";
	char* buf = new char[256];
	clen = z80ex_dasm(buf,256,0,&t1,&t2,&rdbyte,adr,NULL);
	if (clen > 4) clen=4;		// FIXME: z80ex_dasm bug? for DDCB instructions length
	res.dasm = QString(buf);
	while (clen > 0) {
		res.bytes.append(gethexbyte(memRd(zx->mem,adr)));
		clen--;
		adr++;
	}
	return res;
}

bool DebugWin::filldasm() {
	fdasm.clear();
	adr = upadr;
	uchar i;
	unsigned char idx;
	bool ispc = false;
	DasmRow res;
	QLabel *lab1,*lab2,*lab3;
	Z80EX_WORD pc = z80ex_get_reg(zx->cpu,regPC);
	for (i=0; i<DASMROW; i++) {
		if (adr == pc) ispc=true;
		idx = memGetCellFlags(zx->mem,adr);
		lab1 = (QLabel*)asmlay->itemAtPosition(i,0)->widget();
		lab2 = (QLabel*)asmlay->itemAtPosition(i,1)->widget();
		lab3 = (QLabel*)asmlay->itemAtPosition(i,2)->widget();
		lab1->setBackgroundRole((pc == adr) ? QPalette::ToolTipBase : QPalette::Window);
		lab2->setBackgroundRole((pc == adr) ? QPalette::ToolTipBase : QPalette::Window);
		lab3->setBackgroundRole((pc == adr) ? QPalette::ToolTipBase : QPalette::Window);
		lab1->setForegroundRole((idx == 0) ? QPalette::WindowText : QPalette::ToolTipText);
		lab2->setForegroundRole((idx == 0) ? QPalette::WindowText : QPalette::ToolTipText);
		lab3->setForegroundRole((idx == 0) ? QPalette::WindowText : QPalette::ToolTipText);
		if (curcol==1 && currow==i) lab1->setBackgroundRole(QPalette::Highlight);
		if (curcol==2 && currow==i) lab2->setBackgroundRole(QPalette::Highlight);
		if (curcol==3 && currow==i) lab3->setBackgroundRole(QPalette::Highlight);
		res = getdisasm();
		fdasm.append(res);
		lab1->setText(gethexword(res.adr));
		lab2->setText(res.bytes);
		lab3->setText(res.dasm);
	}
	return ispc;
}

ushort DebugWin::getprevadr(ushort ad) {
	ushort i;
	for (i=16;i>0;i--) {
		adr = ad-i;
		getdisasm();
		if (adr==ad) return (ad-i);
	}
	return (ad-1);
}

void DebugWin::showedit(QLabel* lab,QString imsk) {
	ledit->resize(lab->size() + QSize(10,10));
	ledit->setParent(lab->parentWidget());
	ledit->move(lab->pos() - QPoint(6,6));
	ledit->setInputMask(imsk);
	ledit->setText(lab->text());
	ledit->selectAll();
	ledit->show();
	ledit->setFocus();
}

void DebugWin::keyPressEvent(QKeyEvent* ev) {
	qint32 cod = ev->key();
	QLabel *lab = NULL;
	uchar i;
	int idx;
	if (!ledit->isVisible()) {
		switch (ev->modifiers()) {
		case Qt::AltModifier:
			switch (cod) {
				case Qt::Key_A: dmpadr = z80ex_get_reg(zx->cpu,regAF); filldump(); break;
				case Qt::Key_B: dmpadr = z80ex_get_reg(zx->cpu,regBC); filldump(); break;
				case Qt::Key_D: dmpadr = z80ex_get_reg(zx->cpu,regDE); filldump(); break;
				case Qt::Key_H: dmpadr = z80ex_get_reg(zx->cpu,regHL); filldump(); break;
				case Qt::Key_X: dmpadr = z80ex_get_reg(zx->cpu,regIX); filldump(); break;
				case Qt::Key_Y: dmpadr = z80ex_get_reg(zx->cpu,regIY); filldump(); break;
				case Qt::Key_S: dmpadr = z80ex_get_reg(zx->cpu,regSP); filldump(); break;
				case Qt::Key_P: dmpadr = z80ex_get_reg(zx->cpu,regPC); filldump(); break;
			}
			break;
		case Qt::NoModifier:
			switch (cod) {
				case Qt::Key_Escape: if (!ev->isAutoRepeat()) stop(); break;
				case Qt::Key_Return:
					if (ev->isAutoRepeat()) break;
					switch (curcol) {
						case 0: showedit((QLabel*)rglay->itemAtPosition(currow,1)->widget(),"HHHH"); break;
						case 1: showedit((QLabel*)asmlay->itemAtPosition(currow,0)->widget(),"HHHH"); break;
						case 2: showedit((QLabel*)asmlay->itemAtPosition(currow,1)->widget(),"HHHHHHHHHH"); break;
						case 3: showedit((QLabel*)asmlay->itemAtPosition(currow,2)->widget(),""); break;
						case 4: showedit((QLabel*)dmplay->itemAtPosition(currow,0)->widget(),"HHHH"); break;
						case 5:
						case 6:
						case 7:
						case 8:
						case 9:
						case 10:
						case 11:
						case 12: showedit((QLabel*)dmplay->itemAtPosition(currow,curcol-4)->widget(),"HH"); break;
					}
					break;
				case Qt::Key_Space:
					if (!ev->isAutoRepeat() && curcol>0 && curcol<4) {
						memSwitchCellFlags(zx->mem,fdasm[currow].adr,MEM_BRK_FETCH);
						filldasm();
					}
					break;
				case Qt::Key_Z:
					if (curcol>0 && curcol<4) {
						z80ex_set_reg(zx->cpu,regPC,fdasm[currow].adr);
						filldasm();
					}
					break;
				case Qt::Key_F7:
					lastDbgTicks = zx->tickCount;
					emulExec();
					if (!fillall()) {
						upadr = z80ex_get_reg(zx->cpu,regPC);
						filldasm();
					}
					break;
				case Qt::Key_F8:
					lastDbgTicks = zx->tickCount;
					cpoint.adr = z80ex_get_reg(zx->cpu,regPC);
					cpoint.sp = z80ex_get_reg(zx->cpu,regSP);
					cpoint.active = true;
					stop();
					break;
				case Qt::Key_F12:
					zxReset(zx,RES_DEFAULT);
					fillall();
					break;
				case Qt::Key_Left:
					if (curcol>0) {
						curcol--;
						if (currow >= rowincol[curcol]) currow = rowincol[curcol];
					}
					fillall();
					break;
				case Qt::Key_Right:
					if (curcol<12) {
						curcol++;
						if (currow >= rowincol[curcol]) currow = rowincol[curcol];
					}
					fillall();
					break;
				case Qt::Key_Down:
					if (currow < rowincol[curcol]) {
						currow++;
					} else {
						if (curcol>0 && curcol<4) {
							lab = (QLabel*)asmlay->itemAtPosition(1,0)->widget();
							upadr = lab->text().toInt(NULL,16);
						}
						if (curcol>3 && curcol<12) dmpadr += 8;
					}
					fillall();
					break;
				case Qt::Key_Up:
					if (currow > 0) {
						currow--;
					} else {
						if (curcol>0 && curcol<4) upadr = getprevadr(upadr);
						if (curcol>3 && curcol<12) dmpadr -= 8;
					}
					fillall();
					break;
				case Qt::Key_PageDown:
					if (curcol>0 && curcol<4) {
						lab = (QLabel*)asmlay->itemAtPosition(DASMROW-1,0)->widget();
						upadr = lab->text().toInt(NULL,16);
						filldasm();
					}
					if (curcol>3 && curcol<12) {
						dmpadr += (DMPSIZE << 3);
						filldump();
					}
					break;
				case Qt::Key_PageUp:
					if (curcol>0 && curcol<4) {
						for (i=0; i<DASMROW-2; i++) upadr=getprevadr(upadr);
						filldasm();
					}
					if (curcol>3 && curcol<12) {
						dmpadr -= (DMPSIZE << 3);
						filldump();
					}
					break;
				case Qt::Key_Home:
					upadr = z80ex_get_reg(zx->cpu,regPC);
					filldasm();
					break;
			}
			break;
		}
	} else {
		switch (cod) {
			case Qt::Key_Escape:
				ledit->hide();
				setFocus();
				break;
			case Qt::Key_Return:
				tmpb = false;
				switch (curcol) {
					case 0: idx = ledit->text().toUShort(&tmpb,16);
						if (!tmpb) break;
						switch (currow) {
							case 0: z80ex_set_reg(zx->cpu,regAF,idx); break;
							case 1: z80ex_set_reg(zx->cpu,regBC,idx); break;
							case 2: z80ex_set_reg(zx->cpu,regDE,idx); break;
							case 3: z80ex_set_reg(zx->cpu,regHL,idx); break;
							case 4: z80ex_set_reg(zx->cpu,regAF_,idx); break;
							case 5: z80ex_set_reg(zx->cpu,regBC_,idx); break;
							case 6: z80ex_set_reg(zx->cpu,regDE_,idx); break;
							case 7: z80ex_set_reg(zx->cpu,regHL_,idx); break;
							case 8: z80ex_set_reg(zx->cpu,regIX,idx); break;
							case 9: z80ex_set_reg(zx->cpu,regIY,idx); break;
							case 10: z80ex_set_reg(zx->cpu,regI,(idx & 0xff00) >> 8);
								z80ex_set_reg(zx->cpu,regR,idx & 0xff);
								z80ex_set_reg(zx->cpu,regR7,idx & 0x80);
								break;
							case 11: z80ex_set_reg(zx->cpu,regPC,idx); break;
							case 12: z80ex_set_reg(zx->cpu,regSP,idx); break;
							case 13: if ((idx & 0xf00)>0x200) {
									tmpb = false;
								} else {
									z80ex_set_reg(zx->cpu,regIM,(idx & 0xf00)>>8);
									z80ex_set_reg(zx->cpu,regIFF1,idx & 0xf0);
									z80ex_set_reg(zx->cpu,regIFF2,idx & 0x0f);
								}
								break;
						}
						fillregz();
						break;
					case 1:
						idx = ledit->text().toUShort(&tmpb,16);
						if (!tmpb) break;
						upadr = idx; for (i=0; i<currow; i++) upadr = getprevadr(upadr);
						filldasm();
						break;
					case 4:
						idx = ledit->text().toUShort(&tmpb,16);
						if (!tmpb) break;
						dmpadr = idx - ((currow)<<3);
						filldump();
						break;
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
					case 10:
					case 11:
					case 12: idx = ledit->text().toShort(&tmpb,16);
						tmpb &= (idx<0x100);
						if (!tmpb) break;
						memWr(zx->mem, dmpadr + (currow << 3) + curcol - 5, idx & 0xff);
						curcol++;
						if (curcol > 12) {
							curcol=5; currow++;
							if (currow > rowincol[5]) {currow--; dmpadr += 8;}
						}
						filldump();
						filldasm();
						showedit((QLabel*)dmplay->itemAtPosition(currow,curcol-4)->widget(),"HH");
						tmpb=false;
						break;
				}
				if (tmpb) {ledit->hide(); setFocus();}

				break;
		}
	}
}
