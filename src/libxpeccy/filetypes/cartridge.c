#include "filetypes.h"

int loadSlot(xCartridge* slot, const char* name) {
	FILE* file = fopen(name, "rb");
	if (!file) return ERR_CANT_OPEN;
	fseek(file,0,SEEK_END);
	size_t siz = ftell(file);
	rewind(file);
	int err = ERR_OK;
	if (siz > (4 * 1024 * 1024)) {
		err = ERR_RAW_LONG;
	} else {
		int tsiz = 0x4000;
		while (tsiz < siz) {tsiz <<= 1;}		// get nearest 2^n >= siz
		slot->data = realloc(slot->data, tsiz);
		slot->memMask = tsiz - 1;
		strcpy(slot->name, name);
		fread(slot->data, tsiz, 1, file);
		for (tsiz = 0; tsiz < 8; tsiz++) {
			slot->memMap[tsiz] = 0;
		}
		err = ERR_OK;
	}
	fclose(file);
	return err;
}