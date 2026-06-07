/*
 * PicoDrive
 * (C) notaz, 2010,2013
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 */
#include <stdio.h>
#include <string.h>

#include <pico/pico_int.h>
#include <pico/sound/mix.h>

#ifdef USE_TREMOR
#include "tremor/ivorbisfile.h"
#define ov_read(vf,b,l,be,w,s,ip) (ov_read)(vf,b,l,ip)
#else
#include <vorbis/vorbisfile.h>
#define ov_time_total(vf,ix) (ov_time_total)(vf,ix)*1000
#endif

static OggVorbis_File ogg_current_file;
static int ogg_current_index;
static int cdda_out_pos;
static int decoder_active;

// need this as ov_clear would close the FILE* otherwise
static int ov_fseek(void *f, ogg_int64_t off, int wence)
	{ return fseek(f, off, wence); }
static size_t ov_fread(void *buf, size_t sz, size_t n, void *f)
	{ return fread(buf, sz, n, f); }
static long ov_ftell(void *f)
	{ return ftell(f); }
static ov_callbacks ogg_cb = { ov_fread, ov_fseek, NULL /*fclose*/, ov_ftell };

int ogg_get_length(void *f_)
{
	FILE *f = f_;
	int fs, ret;

	ret = ov_open_callbacks(f, &ogg_current_file, NULL, 0, ogg_cb);
	if (ret < 0)
		return ret;

	fs = ov_time_total(&ogg_current_file, -1);
	ov_clear(&ogg_current_file);
	return fs;
}

void ogg_start_play(void *f_, int sample_offset)
{
	FILE *f = f_;
	int ret;

	cdda_out_pos = 0;
	decoder_active = 0;

	if (!(PicoIn.opt & POPT_EN_MCD_CDDA) || f == NULL) // cdda disabled or no file?
		return;

	fseek(f, 0, SEEK_SET);
	ret = ov_open_callbacks(f, &ogg_current_file, NULL, 0, ogg_cb);
	if (ret)
		return;
	ogg_current_index = 0;

	// seek..
	ov_pcm_seek(&ogg_current_file, sample_offset);

	decoder_active = 1;
}

void ogg_stop_play(void)
{
	if (decoder_active)
		ov_clear(&ogg_current_file);
	decoder_active = 0;
}

void ogg_update(s32 *buffer, int length, int stereo)
{
	int length_ogg;
	void (*mix_samples)(s32 *dest_buf, short *mp3_buf, int count, int fac16);

	if (!decoder_active)
		return;

	length_ogg = length * Pico.snd.cdda_mult >> 16;

	mix_samples = mix_16h_to_32_resample_stereo;
	if (!stereo)
		mix_samples = mix_16h_to_32_resample_mono;

	if (1152 - cdda_out_pos >= length_ogg) {
		mix_samples(buffer, cdda_out_buffer + cdda_out_pos * 2,
			length, Pico.snd.cdda_mult);

		cdda_out_pos += length_ogg;
	} else {
		int left = (1152 - cdda_out_pos) * Pico.snd.cdda_div >> 16;
		int ret, sm = stereo ? 2 : 1;

		if (left > 0)
			mix_samples(buffer, cdda_out_buffer + cdda_out_pos * 2,
				left, Pico.snd.cdda_mult);

		for (length_ogg = 4*1152; length_ogg > 0; ) {
			ret = ov_read(&ogg_current_file,
				(char *)cdda_out_buffer + (4*1152 - length_ogg),
				length_ogg, !CPU_IS_LE,2,1, &ogg_current_index);
			if (ret > 0)
				length_ogg -= ret;
			else
				break;
		}
		if (ret > 0) {
			mix_samples(buffer + left * sm, cdda_out_buffer,
				length-left, Pico.snd.cdda_mult);
			cdda_out_pos = (length-left) * Pico.snd.cdda_mult >> 16;
		} else
			cdda_out_pos = 0;
	}
}

