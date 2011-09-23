#include "common.h"
#include "sound.h"
#include "spectrum.h"

#include <iostream>
#ifdef HAVESDLSOUND
	#include <SDL/SDL_sound.h>
#endif

extern ZXComp* zx;

struct OutSys {
	std::string name;
	bool (*open)();
	void (*play)();
	void (*close)();
};

bool sndEnabled;
bool sndMute;

uint8_t sndBuffer[0x2000];
uint8_t ringBuffer[0x4000];
int ringPos = 0;
int playPos = 0;
int pass = 0;
int smpCount = 0;

int beepVolume = 100;
int tapeVolume = 100;
int ayVolume = 100;
int gsVolume = 100;

std::vector<OutSys> sndOutputList;
OutSys *sndOutput = NULL;
int sndRate = 44100;
int sndChans = 2;
int sndChunks = 882;
int sndBufSize = 1764;
double tatbyte = 162;
double tickCount = 162;
uint8_t lev,levr,levl;

#ifndef WIN32
	int32_t ossHandle;			// oss
	int32_t sndFormat;
#ifdef HAVEALSASOUND
	char *alsaDevice;			// alsa
	snd_output_t *alsaOutput;
	snd_pcm_t *alsaHandle;
	snd_pcm_sframes_t alsaFrames;
#endif
#endif

// output

void sndSync(uint32_t tk) {
	if (tk < tickCount) return;
//	if (tickCount > 0) return;
	tickCount += tatbyte;
	zx->tape->sync();
	lev = zx->beeplev ? beepVolume : 0;
	if (zx->tape->flags & TAPE_ON) {
		lev += (((zx->tape->outsig & 1) ? tapeVolume : 0) + ((zx->tape->signal & 1) ? tapeVolume : 0));
	}
	lev *= 0.16;
	levl = lev;
	levr = lev;
	AYData tmpl = zx->aym->sc1->getvol(tk);
	levl += tmpl.l * ayVolume / 100.0;
	levr += tmpl.r * ayVolume / 100.0;
	tmpl = zx->aym->sc2->getvol(tk);
	levl += tmpl.l * ayVolume / 100.0;
	levr += tmpl.r * ayVolume / 100.0;
	GSData tmpm = zx->gs->getvol();
//if ((levr != 0) || (levl != 0)) printf("%i : %i\t%i : %i\n",levl,levr,tmpm.l,tmpm.r);
	levl += tmpm.l * gsVolume / 100.0;
	levr += tmpm.r * gsVolume / 100.0;
	if (smpCount >= sndChunks) return;
	ringBuffer[ringPos] = levl;
	ringPos++;
	ringBuffer[ringPos] = levr;
	ringPos++;
	ringPos &= 0x3fff;
	smpCount++;
}

void sndCalibrate() {
	sndChunks = (int)(sndRate / 50.0);			// samples played at 1/50 sec			882
	sndBufSize = (int)(sndRate * sndChans / 50.0);		// buffer size for 1/50 sec play		1764
	tatbyte = (448 * 320 / (double)sndChunks);		// count of 7MHz ticks between samples		162.54
}

void addOutput(std::string nam, bool (*opf)(), void (*plf)(), void (*clf)()) {
	OutSys newsys;
	newsys.name = nam;
	newsys.open = opf;
 	newsys.play = plf;
	newsys.close = clf;
	sndOutputList.push_back(newsys);
}

std::string sndGetOutputName() {
	std::string res = "NULL";
	if (sndOutput != NULL) {
		res = sndOutput->name;
	}
	return res;
}

void setOutput(std::string nam) {
	if (sndOutput != NULL) {
		if (sndOutput->name == nam) return;
		sndOutput->close();
	}
	sndOutput = NULL;
	for (uint32_t i=0; i<sndOutputList.size(); i++) {
		if (sndOutputList[i].name == nam) {
			sndOutput = &sndOutputList[i];
			break;
		}
	}
	if (sndOutput == NULL) {
		printf("Can't find sound system. Reset to NULL\n");
		setOutput("NULL");
	}
	if (!sndOutput->open()) {
		printf("Can't open sound system. Reset to NULL\n");
		setOutput("NULL");
	}
}

bool sndOpen() {
	if (sndOutput == NULL) return true;
	return sndOutput->open();
}

void sndPlay() {
	if (sndOutput != NULL) {
		sndOutput->play();
	}
}

void sndPause(bool b) {
	if (sndOutput == NULL) return;
	if (sndOutput->name != "SDL") return;
	SDL_PauseAudio(b ? 1 : 0);
}

void sndClose() {
	if (sndOutput != NULL) sndOutput->close();
}

std::vector<std::string> sndGetList() {
	std::vector<std::string> res;
	for (uint32_t i=0; i<sndOutputList.size(); i++) {
		res.push_back(sndOutputList[i].name);
	}
	return res;
}

std::string sndGetName() {
	std::string res = "NULL";
	if (sndOutput != NULL) {
		res = sndOutput->name;
	}
	return res;
}

int sndGet(int wut) {
	int res = 0;
	switch (wut) {
		case SND_RATE: res = sndRate; break;
		case SND_BEEP: res = beepVolume; break;
		case SND_TAPE: res = tapeVolume; break;
		case SND_AYVL: res = ayVolume; break;
		case SND_GSVL: res = gsVolume; break;
		case SND_ENABLE: res = sndEnabled ? 1 : 0; break;
		case SND_MUTE: res = sndMute ? 1 : 0; break;
	}
	return res;
}

void sndSet(int wut,int val) {
	switch (wut) {
		case SND_RATE: sndRate = val; break;
		case SND_COUNT: smpCount = val; break;
		case SND_BEEP: beepVolume = val; break;
		case SND_TAPE: tapeVolume = val; break;
		case SND_AYVL: ayVolume = val; break;
		case SND_GSVL: gsVolume = val; break;
	}
}

void sndSet(int wut, bool val) {
	switch(wut) {
		case SND_ENABLE: sndEnabled = val; break;
		case SND_MUTE: sndMute = val; break;
	}
}

//------------------------
// Sound output
//------------------------

void fillBuffer(int len) {
//printf("%i\t%i\t",len,playPos);
//int z = playPos;
	for (int bufPos=0; bufPos < len; bufPos++) {
		sndBuffer[bufPos] = ringBuffer[playPos];
		playPos++;
		playPos &= 0x3fff;
	}
//printf("%i\t%i\n",playPos,(z < playPos) ? playPos - z : 0x4000 - (z - playPos));
}

bool null_open() {return true;}
void null_play() {}
void null_close() {}

#ifdef HAVESDLSOUND

// FIXME: something going wrong. sdlPlayAudio plays buffer slower than emulation fill it
void sdlPlayAudio(void*,Uint8* stream, int len) {
	if (pass < 2) {
		pass++;
	} else {
#if 1
		int diff;
		if (playPos < ringPos) {
			diff = ringPos - playPos;
		} else {
			diff = 0x4000 - (ringPos - playPos);
		}
		if (diff > len) fillBuffer(len);
#else
		fillBuffer(len);
#endif
	}
//	SDL_LockAudio();
	SDL_MixAudio(stream,sndBuffer,len,SDL_MIX_MAXVOLUME);
//	SDL_UnlockAudio();
}

bool sdlopen() {
	printf("Open SDL audio device...");
	SDL_AudioSpec* asp = new SDL_AudioSpec;
	asp->freq = sndRate;
	asp->format = AUDIO_U8;
	asp->channels = sndChans;
//	asp->samples = 1024;
	asp->callback = &sdlPlayAudio;
	asp->userdata = NULL;
	if (SDL_OpenAudio(asp,NULL) < 0) {
		printf("failed\n");
		return false;
	}
	SDL_PauseAudio(0);
	printf("OK\n");
	return true;
}

void sdlplay() {
}

void sdlclose() {
	printf("Close SDL audio device\n");
	SDL_PauseAudio(1);
	SDL_CloseAudio();
}

#endif

#ifndef WIN32

bool oss_open() {
	printf("Open OSS audio device\n");
	ossHandle = open("/dev/dsp",O_WRONLY,0777);
	if (ossHandle < 0) return false;
	ioctl(ossHandle,SNDCTL_DSP_SETFMT,&sndFormat);
	ioctl(ossHandle,SNDCTL_DSP_CHANNELS,&sndChans);
	ioctl(ossHandle,SNDCTL_DSP_SPEED,&sndRate);
	return true;
}

void oss_play() {
	if (ossHandle < 0) return;
	uint8_t* ptr = ringBuffer;
	int fsz = ringPos;
	int res;
	while (fsz > 0) {
		res = write(ossHandle,ptr,fsz);
		ptr += res;
		fsz -= res;
	}
	ringPos = 0;
}

void oss_close() {
	if (ossHandle < 0) return;
	printf("Close OSS audio device\n");
	close(ossHandle);
}


#ifdef HAVEALSASOUND

bool alsa_open() {
	int err;
	bool res = true;
	printf("libasound: open audio device... ");
	if ((err = snd_pcm_open(&alsaHandle,alsaDevice,SND_PCM_STREAM_PLAYBACK,0))<0) {
		printf("playback open error: %s\n",snd_strerror(err));
		res=false;
	} else {
		if (alsaHandle == NULL) {
			printf("shit happens\n");
			res = false;
		} else {
			printf("OK\n");
			printf("libasound: set audio paramz...");
			if ((err = snd_pcm_set_params(alsaHandle,SND_PCM_FORMAT_U8,SND_PCM_ACCESS_RW_INTERLEAVED,sndChans,sndRate,1,100000)) < 0) {
				printf("playback open error: %s\n",snd_strerror(err));
				res=false;
			} else {
				printf("OK\n");
			}
		}
	}
	return res;
}

void alsa_play() {
	snd_pcm_sframes_t res;
	uint8_t* ptr = ringBuffer;
	int fsz = sndBufSize / sndChans;
	while (fsz > 0) {
		res = snd_pcm_writei(alsaHandle, ptr, fsz);
		if (res<0) res = snd_pcm_recover(alsaHandle, res, 1);
		if (res<0) break;
		fsz -= res;
		ptr += res * sndChans;
	}
	ringPos = 0;
}

void alsa_close() {
	printf("libasound: close device\n");
	snd_pcm_close(alsaHandle);
}

#endif

#else

// TODO: Windows sound output would be here... someday

#endif

// init
	
void sndInit() {
#ifdef HAVEALSASOUND
	alsaDevice = (char*)"default";
	alsaOutput = NULL;
#endif
	sndFormat = AFMT_U8;
	sndRate = 44100;
	sndChans = 2;
	sndEnabled = true;
	sndMute = true;
	sndOutput = NULL;
	beepVolume = tapeVolume = ayVolume = gsVolume = 100;
	initNoise();							// ay/ym
//	zx->aym->sc2->n.cur = zx->aym->sc1->n.cur = 0xffff;

	addOutput("NULL",&null_open,&null_play,&null_close);
#ifndef WIN32
	addOutput("OSS",&oss_open,&oss_play,&oss_close);
#ifdef HAVEALSASOUND
	addOutput("ALSA",&alsa_open,&alsa_play,&alsa_close);
#endif
#endif
#ifdef HAVESDLSOUND
	addOutput("SDL",&sdlopen,&sdlplay,&sdlclose);	// TODO: do something with SDL output
#endif
}
