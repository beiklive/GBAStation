// Stub implementations for missing MP3 decoder symbols
// These are needed by platform/common/mp3.c but normally come from
// submodule-based decoders (helix/drmp3). We provide no-op stubs
// since MP3 CD audio is not critical.
#include "mp3.h"
#include <string.h>

int mp3_find_sync_word(const unsigned char *buf, int size) { (void)buf; (void)size; return -1; }
int mp3dec_start(FILE *f, int fpos_start) { (void)f; (void)fpos_start; return -1; }
int mp3dec_decode(FILE *f, int *file_pos, int file_len) { (void)f; (void)file_pos; (void)file_len; return 0; }
