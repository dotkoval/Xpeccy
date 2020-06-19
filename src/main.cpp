#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QFontDatabase>
#include <QStyleFactory>

#include <locale.h>

#include "xcore/xcore.h"
#include "xcore/sound.h"
#include "xgui/xgui.h"
#include "libxpeccy/spectrum.h"

#include "emulwin.h"
#include "xgui/debuga/debuger.h"
#include "xgui/options/setupwin.h"
#include "filer.h"

#include <SDL.h>
#undef main

void help() {
	printf("Xpeccy command line arguments:\n");
	printf("-h | --help\t\tshow this help\n");
	printf("-d | --debug\t\tstart debugging immediately\n");
	printf("-s | --size {1..4}\tset window size x1..x4.\n");
	printf("-n | --noflick {0..100}\tset noflick level\n");
	printf("-r | --ratio {0|1}\tset 'keep aspect ratio' property\n");
	printf("-p | --profile NAME\tset current profile\n");
	printf("-b | --bank NUM\t\tset rampage NUM to #c000 memory window\n");
	printf("-a | --adr ADR\t\tset loading address (see -f below)\n");
	printf("-f | --file NAME\tload binary file to address defined by -a (see above)\n");
	printf("-l | --labels NAME\tload labels list generated by LABELSLIST in SJASM+\n");
	printf("--fullscreen {0|1}\tfullscreen mode\n");
	printf("--pc ADR\t\tset PC\n");
	printf("--sp ADR\t\tset SP\n");
	printf("--bp ADR\t\tset fetch brakepoint to address ADR\n");
	printf("--bp NAME\t\tset fetch brakepoint to label NAME (see -l key)\n");
	printf("--disk X\t\tselect drive to loading file (0..3 | a..d | A..D)\n");
	printf("--style\t\t\tMacOSX only: use native qt style, else 'fusion' will be forced\n");
}

int main(int ac,char** av) {

// NOTE:SDL_INIT_VIDEO must be here for SDL_Joystick event processing. Joystick doesn't works without video init
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
	atexit(SDL_Quit);
	SDL_version sdlver;
	SDL_VERSION(&sdlver);
	printf("Using SDL ver %u.%u.%u\n", sdlver.major, sdlver.minor, sdlver.patch);

#ifdef HAVEZLIB
	printf("Using ZLIB ver %s\n",ZLIB_VERSION);
#endif
	printf("Using Qt ver %s\n",qVersion());

// this works since Qt5.6 (must be set before QCoreApplication is created)
	#if QT_VERSION >= 0x050600
		QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	#endif
	QApplication app(ac,av,true);

#ifdef _WIN32
	QStringList paths = QCoreApplication::libraryPaths();
	paths.append(".");
	QCoreApplication::setLibraryPaths(paths);
#endif
	sndInit();
	conf_init(av[0]);
	shortcut_init();

	QFontDatabase::addApplicationFont("://DejaVuSansMono.ttf");

	MainWin mwin;
	xThread ethread;
	DebugWin dbgw(&mwin);
	SetupWin optw(&mwin);
	TapeWin tapw(&mwin);
	RZXWin rzxw(&mwin);
	xWatcher wutw(&mwin);
	keyWindow keyw(&mwin);

	loadConfig();
	mwin.fillUserMenu();

	if ((conf.xpos >= 0) && (conf.ypos >= 0))
		mwin.move(conf.xpos, conf.ypos);

	app.connect(&ethread, SIGNAL(s_frame()), &mwin, SLOT(d_frame()));
	app.connect(&ethread, SIGNAL(dbgRequest()), &mwin, SLOT(doDebug()));
	app.connect(&ethread, SIGNAL(tapeSignal(int,int)), &mwin,SLOT(tapStateChanged(int,int)));
	app.connect(&mwin, SIGNAL(s_emulwin_close()), &ethread, SLOT(stop()));

	app.connect(&dbgw, SIGNAL(closed()), &mwin, SLOT(dbgReturn()));
	app.connect(&dbgw, SIGNAL(wannaKeys()), &keyw, SLOT(show()));
	app.connect(&dbgw, SIGNAL(wannaWutch()), &wutw, SLOT(show()));
	app.connect(&dbgw, SIGNAL(wannaOptions(xProfile*)), &optw, SLOT(start(xProfile*)));

	app.connect(&mwin, SIGNAL(s_debug(Computer*)), &dbgw, SLOT(start(Computer*)));
	app.connect(&mwin, SIGNAL(s_prf_change(xProfile*)), &dbgw, SLOT(onPrfChange(xProfile*)));

	app.connect(&mwin, SIGNAL(s_options(xProfile*)), &optw, SLOT(start(xProfile*)));
	app.connect(&optw, SIGNAL(closed()), &mwin, SLOT(optApply()));
	app.connect(&optw, SIGNAL(closed()), &dbgw, SLOT(chaPal()));
	app.connect(&optw, SIGNAL(s_prf_change(std::string)), &mwin, SLOT(setProfile(std::string)));

	app.connect(&mwin, SIGNAL(s_tape_upd(Tape*)), &tapw, SLOT(upd(Tape*)));
	app.connect(&mwin, SIGNAL(s_tape_progress(Tape*)), &tapw, SLOT(updProgress(Tape*)));
	app.connect(&mwin, SIGNAL(s_tape_show()), &tapw, SLOT(show()));

	app.connect(&rzxw, SIGNAL(stateChanged(int)), &mwin, SLOT(rzxStateChanged(int)));
	app.connect(&mwin, SIGNAL(s_rzx_start()), &rzxw, SLOT(startPlay()));
	app.connect(&mwin, SIGNAL(s_rzx_stop()), &rzxw, SLOT(stop()));
	app.connect(&mwin, SIGNAL(s_rzx_upd(Computer*)), &rzxw, SLOT(upd(Computer*)));
	app.connect(&mwin, SIGNAL(s_rzx_show()), &rzxw, SLOT(show()));

	app.connect(&mwin, SIGNAL(s_watch_upd(Computer*)), &wutw, SLOT(fillFields(Computer*)));
	app.connect(&mwin, SIGNAL(s_watch_show()), &wutw, SLOT(show()));

	app.connect(&mwin, SIGNAL(s_keywin_shide()), &keyw, SLOT(switcher()));
	app.connect(&mwin, SIGNAL(s_keywin_upd(Keyboard*)), &keyw, SLOT(upd(Keyboard*)));
	app.connect(&mwin, SIGNAL(s_keywin_close()), &keyw, SLOT(close()));
	app.connect(&mwin, SIGNAL(s_keywin_rall(Keyboard*)), &keyw, SLOT(rall(Keyboard*)));
	app.connect(&keyw, SIGNAL(s_key_press(QKeyEvent*)), &mwin, SLOT(kPress(QKeyEvent*)));
	app.connect(&keyw, SIGNAL(s_key_release(QKeyEvent*)), &mwin, SLOT(kRelease(QKeyEvent*)));

	int i = 1;
	char* parg;
	int adr = 0x4000;
	mwin.setProfile("");
	int dbg = 0;
	int hlp = 0;
	int drv = 0;
	int lab = 1;
	xAdr xadr;
	int tmpi;
	int style = 0;
	while (i < ac) {
		parg = av[i++];
		if ((strcmp(parg,"-d") == 0) || (strcmp(parg,"--debug") == 0)) {
			dbg = 1;
		} else if (!strcmp(parg,"-h") || !strcmp(parg,"--help")) {
			help();
			hlp = 1;
		} else if (!strcmp(parg, "--style")) {
			style = 1;
		} else if (i < ac) {
			if (!strcmp(parg,"-p") || !strcmp(parg,"--profile")) {
				mwin.setProfile(std::string(av[i]));
				i++;
			} else if (!strcmp(parg,"--pc")) {
				mwin.comp->cpu->pc = strtol(av[i],NULL,0) & 0xffff;
				i++;
			} else if (!strcmp(parg,"--sp")) {
				mwin.comp->cpu->sp = strtol(av[i],NULL,0) & 0xffff;
				i++;
			} else if (!strcmp(parg,"-b") || !strcmp(parg,"--bank")) {
				memSetBank(mwin.comp->mem, 0xc0, MEM_RAM, strtol(av[i],NULL,0), MEM_16K, NULL, NULL, NULL);
				i++;
			} else if (!strcmp(parg,"-a") || !strcmp(parg,"--adr")) {
				adr = strtol(av[i],NULL,0) & 0xffff;
				i++;
			} else if (!strcmp(parg,"-f") || !strcmp(parg,"--file")) {
				loadDUMP(mwin.comp, av[i], adr);
				i++;
			} else if (!strcmp(parg,"--bp")) {
				xadr = getLabel(av[i]);
				if (xadr.abs < 0) {
					brkSet(BRK_CPUADR, MEM_BRK_FETCH, strtol(av[i],NULL,0) & 0xffff, -1);
				} else {
					brkSet(BRK_CPUADR, MEM_BRK_FETCH, xadr.adr & 0xffff, -1);
				}
				i++;
			} else if (!strcmp(parg,"-l") || !strcmp(parg,"--labels")) {
				lab = loadLabels(av[i]);
				i++;
			} else if (!strcmp(parg,"-s") || !strcmp(parg, "--size")) {
				tmpi = atoi(av[i]);
				if ((tmpi > 0) && (tmpi < 5))
					conf.vid.scale = tmpi;
				i++;
			} else if (!strcmp(parg, "-n") || !strcmp(parg, "--noflick")) {
				tmpi = atoi(av[i]);
				if ((tmpi >= 0) && (tmpi <= 100))
					noflic = tmpi / 2;
				i++;
			} else if (!strcmp(parg, "-r") || !strcmp(parg, "--ratio")) {
				conf.vid.keepRatio = atoi(av[i]) ? 1 : 0;
				i++;
			} else if (!strcmp(parg,"--fullscreen")) {
				conf.vid.fullScreen = atoi(av[i]) ? 1 : 0;
				i++;
			} else if (!strcmp(parg, "--disk")) {
				parg = av[i];
				if (strlen(parg) == 1) {
					switch(parg[0]) {
						case '0': case 'a': case 'A': drv = 0; break;
						case '1': case 'b': case 'B': drv = 1; break;
						case '2': case 'c': case 'C': drv = 2; break;
						case '3': case 'd': case 'D': drv = 3; break;
					}
				}
				i++;
			} else if (strlen(parg) > 0) {
				load_file(mwin.comp, parg, FG_ALL, drv);
			}
		} else if (strlen(parg) > 0) {
			load_file(mwin.comp, parg, FG_ALL, drv);
		}
	}

#ifdef __APPLE__
	if (!style) {
		app.setStyle(QStyleFactory::create("Fusion"));
	}
#endif

	if (!hlp) {
//		prfFillBreakpoints(conf.prof.cur);
		mwin.updateWindow();
		mwin.checkState();
		if (dbg) mwin.doDebug();
		mwin.show();
		mwin.raise();
		mwin.activateWindow();
		conf.running = 1;
		ethread.start();
		if (!lab) shitHappens("Can't open labels file");
		app.exec();
		ethread.stop();
		ethread.wait();
	}
	conf.running = 0;
	sndClose();
	return 0;
}
