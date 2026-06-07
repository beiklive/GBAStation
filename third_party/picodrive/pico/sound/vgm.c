/*
 * VGM parser for PicoDrive
 * SPDX-License-Identifier: MIT
 *
 * specs:
 * https://www.smspower.org/uploads/Music/vgmspec170.txt
 * https://vgmrips.net/wiki/VGM_Specification
 */
#include <stdlib.h>
#include <string.h>
#include "../pico_int.h"
#include "sn76496.h"
#include "ym2413.h"
#include "ym2612.h"
#include "vgm.h"
#include <zlib.h>

#define READ_BLOCK (512 * 1024)
#define OVERALLOC 16  // to avoid range checks in the parser

#define BLOCK_TYPE_YM2612  0
#define BLOCK_TYPE_RF5C164 2
#define CHIP_TYPE_YM2612 2

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

struct vgm_header
{
  char vgm[4];
  u32 eof, version, sn76489_clock;
  u32 ym2413_clock, gd3_offset, total_samples, loop_offset;  // 10
  u32 loop_samples, rate, sn_fb_w, ym2612_clock;             // 20
  u32 ym2151_clock, data_offset, spcm_clock, spcm_interface; // 30
};

struct vgm
{
  size_t data_size;
  size_t data_pos;
  u32 sample_pos;
  u32 block_count;
  u32 stream_count;
  int dacout_nonstream;
  struct vgm_block {
    u32 start, end, pos;
  } blocks[0x40];
  struct vgm_stream {
    u8 chip_type, ioport, reg, block_i;
    u32 pos_inc, pos_fraction; // Q24
    u32 pos;
    u32 playing:1;
    u32 new_sample:1;
    //u32 looping:1;
    u32 flags_warned:1;
    u32 bad_start_warned:1;
  } *streams;
  union {
    struct vgm_header hdr;
    u8 data[0];
  };
};

static struct vgm *g_vgm;

static size_t gzread_check(gzFile file, void *ptr, size_t size)
{
#if ZLIB_VERNUM >= 0x1290
  size_t read_size = gzfread(ptr, 1, size, file);
#else
  int read_size = gzread(file, ptr, size);
#endif
  if (read_size == 0 || (long)read_size == -1)
  {
    int erri = Z_OK;
    const char *err = gzerror(file, &erri);
    if (erri != Z_OK && erri != Z_STREAM_END) {
      elprintf(EL_VGM | EL_STATUS | EL_ANOMALY, "gzfread error %d: %s\n", erri, err);
      return 0;
    }
  }
  return read_size;
}

static struct vgm_stream *stream_get(struct vgm *vgm, size_t cmd_pos, u8 id)
{
  void *tmp;
  if (id < vgm->stream_count)
    return &vgm->streams[id];
  if (id == 0xff) // reserved?
    goto problem;
  tmp = realloc(vgm->streams, (id + 1) * sizeof(vgm->streams[0]));
  if (!tmp)
    goto problem;
  vgm->streams = tmp;
  memset(&vgm->streams[vgm->stream_count], 0,
      (id + 1 - vgm->stream_count) * sizeof(vgm->streams[0]));
  vgm->stream_count = id + 1;
  return &vgm->streams[id];
problem:
  elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: broken stream %02x", cmd_pos, id);
  return NULL;
}

static int vgm_reset_p(struct vgm *vgm, const char *fname)
{
  vgm->data_pos = 0x40;
  if (vgm->hdr.data_offset != 0) {
    size_t data_offset = (size_t)0x34 + CPU_LE4(vgm->hdr.data_offset);
    if (data_offset > vgm->data_size) {
      elprintf(EL_VGM | EL_STATUS | EL_ANOMALY, "%s: broken data_offset %x",
          fname, CPU_LE4(vgm->hdr.data_offset));
      return 0;
    }
    vgm->data_pos = data_offset;
  }

  Pico.m.pal = CPU_LE4(vgm->hdr.rate) == 50;
  // pcd_soft_reset() ?
  if (Pico_mcd)
    Pico_mcd->pcm.bank = 0;
  // conflics md vs sms PMS_HW_FMUSED
  Pico.m.hardware = 0;
  return 1;
}

int vgm_load(const char *fname)
{
  struct vgm *vgm = NULL;
  gzFile file = NULL;
  size_t alloc_size = 0;
  int erri = Z_OK;
  size_t read_size;

  file = gzopen(fname, "rb");
  if (!file) {
    perror("gzopen");
    goto fail;
  }
  alloc_size = sizeof(*vgm) + OVERALLOC;
  vgm = calloc(1, alloc_size);
  if (!vgm)
    goto fail;
  while (!gzeof(file))
  {
    void *tmp = realloc(vgm, (alloc_size += READ_BLOCK));
    if (!tmp)
      goto fail;
    vgm = tmp;
    read_size = gzread_check(file, vgm->data + vgm->data_size, READ_BLOCK);
    if (read_size == 0) {
      if (vgm->data_size == 0)
        goto fail;
      break;
    }
    vgm->data_size += read_size;
  }
  erri = gzclose(file);
  file = NULL;
  if (erri != Z_OK)
    elprintf(EL_VGM | EL_STATUS | EL_ANOMALY, "gzclose error %d", erri);
  memset(vgm->data + vgm->data_size, 0, OVERALLOC);
  if (vgm->data_size <= 0x40) {
    elprintf(EL_VGM | EL_STATUS | EL_ANOMALY, "%s: file too small: %zd",
        fname, vgm->data_size);
    goto fail;
  }
  if (memcmp(vgm->hdr.vgm, "Vgm ", 4) != 0) {
    elprintf(EL_VGM | EL_STATUS | EL_ANOMALY, "%s: wrong ident", fname);
    goto fail;
  }
  if (!vgm_reset_p(vgm, fname))
    goto fail;
  elprintf(EL_VGM | EL_STATUS, "vgm v%x.%02x pal=%d",
      CPU_LE4(vgm->hdr.version) >> 8, CPU_LE4(vgm->hdr.version) & 0xff, Pico.m.pal);
  // all supported chips may be used anytime, so partially initialize
  PicoIn.AHW |= PAHW_VGM | PAHW_MCD;
  PicoCreateMCD(NULL, 0);
  // various things assume rom presence, so make a dummy one
  if (!Pico.rom) {
    Pico.rom = PicoCartAlloc(0x10000, 0);
    if (Pico.rom) {
      Pico.romsize = 0x10000;
      memset(Pico.rom, 0, Pico.romsize);
    }
  }
  PicoCartInsert(Pico.rom, Pico.romsize, NULL);
  Pico.m.hardware = 0;
  g_vgm = vgm;
  return 0;

fail:
  elprintf(EL_VGM | EL_STATUS | EL_ANOMALY, "vgm: %s: failed", fname);
  if (file)
    gzclose(file);
  vgm_finish();
  return -1;
}

static int try_start_stream(struct vgm *vgm, struct vgm_stream *stream,
    size_t cmd_pos, u32 block_i, u8 flags)
{
  if (block_i >= ARRAY_SIZE(vgm->blocks) ||
      vgm->blocks[block_i].start >= vgm->blocks[block_i].end) {
    if (!stream->bad_start_warned) {
      elprintf(EL_VGM | EL_ANOMALY,
          "vgm: %06zx: stream start: bad block id %02x", cmd_pos, block_i);
      stream->bad_start_warned = 1;
    }
    return 0;
  }
  if (!stream->pos_inc) {
    if (!stream->bad_start_warned) {
      elprintf(EL_VGM | EL_ANOMALY,
          "vgm: %06zx: stream start: no sample rate", cmd_pos);
      stream->bad_start_warned = 1;
    }
    return 0;
  }
  if (stream->chip_type == CHIP_TYPE_YM2612 || stream->reg == 0x2a)
    /* ok */;
  else {
    if (!stream->bad_start_warned) {
      elprintf(EL_VGM | EL_ANOMALY,
          "vgm: %06zx: stream start: unhandled chip config: %02x %02x",
          cmd_pos, stream->chip_type, stream->reg);
      stream->bad_start_warned = 1;
    }
  }
  if (flags && !stream->flags_warned) {
    elprintf(EL_VGM | EL_ANOMALY,
        "vgm: %06zx: stream start: ignoring flags %02x", cmd_pos, flags);
    stream->flags_warned = 1;
  }
  stream->block_i = block_i;
  stream->pos_fraction = 0;
  stream->pos = vgm->blocks[block_i].start;
  stream->new_sample = 1;
  return 1;
}

int rf5c164_write(struct vgm *vgm, size_t cmd_pos,
    u32 doffset, const u8 *src, u32 soffset, u32 length)
{
  // affected by current bank, but can go to further banks by using offset
  size_t slength = vgm->blocks[0].end - vgm->blocks[0].start;
  if (soffset >= slength || soffset + length > slength ||
      (Pico_mcd->pcm.bank << 12) + doffset >= sizeof(Pico_mcd->pcm_ram) ||
      (Pico_mcd->pcm.bank << 12) + doffset + length > sizeof(Pico_mcd->pcm_ram))
  {
    elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: broken ram write %x->%x l=%x/%zx",
        cmd_pos, soffset, doffset, length, slength);
    return 0;
  }
  memcpy(Pico_mcd->pcm_ram_b[Pico_mcd->pcm.bank] + doffset, src + soffset, length);
  return 1;
}

static u32 get16(const u8 **p)
{
  u32 d_ = *(*p)++; d_ |= *(*p)++ << 8;
  return d_;
}

static u32 get24(const u8 **p)
{
  u32 d_ = *(*p)++; d_ |= *(*p)++ << 8; d_ |= *(*p)++ << 16;
  return d_;
}

static u32 get32(const u8 **p)
{
  u32 d_ = *(*p)++; d_ |= *(*p)++ << 8; d_ |= *(*p)++ << 16; d_ |= *(*p)++ << 24;
  return d_;
}

void vgm_frame(void)
{
  u32 osc_cyc_per_smp = Pico.m.pal ? 256ull*OSC_PAL/44100+1 : 256ull*OSC_NTSC/44100+1;
  u32 scd_cyc_per_smp = 256ull*12500000/44100+1; // Q8
  u32 samples_per_frame = Pico.m.pal ? 44100/50 : 44100/60;
  u32 sample_pos_stream = 0;
  u32 s68k_base = SekCycleCntS68k;
  int dacout_stream = 0;
  int streams_playing = 0;
  struct vgm_stream *stream;
  struct vgm *vgm = g_vgm;
  u32 i;

  if (!vgm)
    return;
  Pico.t.z80c_aim = 0;
  PsndStartFrame();

  for (i = 0; i < vgm->stream_count; i++) {
    stream = &vgm->streams[i];
    if (stream->playing) {
      dacout_stream += ((int)vgm->data[stream->pos] - 0x80) << DAC_SHIFT;
      streams_playing = 1;
    }
  }

  while (vgm->data_pos < vgm->data_size && sample_pos_stream < samples_per_frame)
  {
    if (vgm->sample_pos < samples_per_frame)
    {
      size_t cmd_pos = vgm->data_pos; // file offset for logs
      const u8 *fdata = vgm->data + vgm->data_pos;
      u8 id, type, addr, data, flags, param[3];
      u32 offset32, data32, data16;
      u32 doffset, length;
      int samples, wait_samples;
      u8 cmd = *fdata++;
      switch (cmd)
      {
        case 0x4f: // Game Gear PSG stereo
          SN76496Config(*fdata++);
          break;
        case 0x50: // sn76489/sn76496 write
          PsndDoPSG(z80_cyclesDone());
          SN76496Write(*fdata++);
          break;
        case 0x51: // ym2413 write
          Pico.m.hardware |= PMS_HW_FMUSED;
          YM2413_regWrite(*fdata++);
          YM2413_dataWrite(*fdata++);
          break;
        case 0x52: // ym2612 port 0 write
          ym2612.OPN.ST.address = *fdata++;
          ym2612.addr_A1 = 0;
          ym2612.REGS[ym2612.OPN.ST.address] = data = *fdata++;
          if (ym2612.OPN.ST.address == 0x2a) {
            wait_samples = 0;
            goto write_ym2612_dac;
          }
          goto write_ym2612_d;
        case 0x53: // ym2612 port 1 write
          ym2612.OPN.ST.address = *fdata++;
          ym2612.addr_A1 = 1;
          ym2612.REGS[0x100 | ym2612.OPN.ST.address] = data = *fdata++;
        write_ym2612_d:
          PsndDoFM(z80_cyclesDone());
          YM2612Write_(1, data);
          break;
        case 0x61: // wait n samples
          wait_samples = get16(&fdata);
          goto wait;
        case 0x62: // wait 735 samples
          wait_samples = 735;
          goto wait;
        case 0x63: // wait 882 samples
          wait_samples = 882;
          goto wait;
        case 0x66: // end of sound data
          if (vgm->hdr.loop_offset) {
            size_t target = (size_t)0x1c + CPU_LE4(vgm->hdr.loop_offset);
            if (target <= vgm->data_size) {
              fdata = vgm->data + target;
              break;
            }
            elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: broken loop_offset %x",
                cmd_pos, CPU_LE4(vgm->hdr.loop_offset));
          }
          elprintf(EL_VGM, "vgm: %06zx: 0x66", cmd_pos);
          fdata = vgm->data + vgm->data_size;
          break;
        case 0x67: // data block
          fdata++;
          type = *fdata++;
          length = get32(&fdata);
          if (vgm->data_pos + 7u + length > vgm->data_size) {
            elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: broken data block, len=%u",
                cmd_pos, length);
            fdata = vgm->data + vgm->data_size;
            break;
          }
          switch (type) {
          case BLOCK_TYPE_YM2612:
          case BLOCK_TYPE_RF5C164:
            i = vgm->block_count;
            if (i < ARRAY_SIZE(vgm->blocks)) {
              vgm->blocks[i].start = vgm->blocks[i].pos = vgm->data_pos + 7u;
              vgm->blocks[i].end = vgm->data_pos + 7u + length;
              vgm->block_count++;
            }
            else
              elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: too many data blocks", cmd_pos);
            break;
          case 0xc1: // RF5C164 RAM write
            if (length < 2)
              goto bad;
            doffset = get16(&fdata);
            length -= 2;
            rf5c164_write(vgm, cmd_pos, doffset, fdata, 0, length);
            break;
          default:
          bad:
            elprintf(EL_VGM | EL_ANOMALY,
                "vgm: %06zx: unhandled data block type %02x, len %x",
                cmd_pos, type, length);
          }
          fdata += length;
          break;
        case 0x68: // PCM RAM write
          fdata++;
          type = *fdata++;
          offset32 = get24(&fdata);
          doffset = get24(&fdata);
          length = get24(&fdata);
          length = ((length - 1) & 0xffffff) + 1;
          switch (type)
          {
          case BLOCK_TYPE_RF5C164:
            rf5c164_write(vgm, cmd_pos, doffset, vgm->data + vgm->blocks[0].start,
                offset32, length);
            break;
          default:
            elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: unhandled pcm ram type %02x",
                cmd_pos, type);
          }
          break;
        case 0x70 ... 0x7f: // wait
          wait_samples = (cmd & 0x0f) + 1;
        wait:
          vgm->sample_pos += wait_samples;
          samples = min(vgm->sample_pos, samples_per_frame);
          Pico.t.z80c_aim = samples * osc_cyc_per_smp / (15 * 256u);
          SekCycleCntS68k = s68k_base + (samples * scd_cyc_per_smp >> 8);
          break;
        case 0x80 ... 0x8f: // ym2612 dac write
          data = 0x80;
          if (vgm->blocks[0].pos < vgm->blocks[0].end) // always block 0?
            data = vgm->data[vgm->blocks[0].pos++];
          else
            elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: dac %x/%x",
                cmd_pos, vgm->blocks[0].pos, vgm->blocks[0].end);
          wait_samples = cmd & 0x0f;
        write_ym2612_dac:
          ym2612.OPN.ST.address = 0x2a;
          ym2612.addr_A1 = 0;
          PsndDoDAC(z80_cyclesDone());
          vgm->dacout_nonstream = ((int)data - 0x80) << DAC_SHIFT;
          ym2612.dacout = dacout_stream + vgm->dacout_nonstream;
          if (wait_samples)
            goto wait;
          break;
        case 0x90: // Stream Control
          id = *fdata++;
          type = *fdata++;
          param[0] = *fdata++;
          param[1] = *fdata++;
          if (!(stream = stream_get(vgm, cmd_pos, id)))
            break;
          stream->chip_type = type;
          stream->ioport = param[0];
          stream->reg = param[1];
          break;
        case 0x91: // Stream Data
          id = *fdata++;
          param[0] = *fdata++;
          param[1] = *fdata++;
          param[2] = *fdata++;
          if (!(stream = stream_get(vgm, cmd_pos, id)))
            break;
          if (param[0] >= ARRAY_SIZE(vgm->blocks)) {
            elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: bad block id %02x",
                cmd_pos, param[0]);
            break;
          }
          if (param[1] != 1 || param[2] != 0)
            elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: unhandled step %02x,%02x",
                cmd_pos, param[1], param[2]);
          stream->block_i = param[0];
          break;
        case 0x92: // Stream Frequency
          id = *fdata++;
          data32 = get32(&fdata);
          if (!(stream = stream_get(vgm, cmd_pos, id)))
            break;
          stream->pos_inc = ((u64)data32 << 24) / 44100u;
          break;
        case 0x93: // Start Stream
          id = *fdata++;
          offset32 = get32(&fdata);
          flags = *fdata++;
          length = get32(&fdata);
          if (!(stream = stream_get(vgm, cmd_pos, id)))
            break;
          if (!try_start_stream(vgm, stream, cmd_pos, stream->block_i, flags))
            break;
          if (offset32 != ~0) {
            struct vgm_block *block = &vgm->blocks[stream->block_i];
            if (offset32 >= block->end - block->start) {
              if (!stream->bad_start_warned) {
                elprintf(EL_VGM | EL_ANOMALY,
                    "vgm: %06zx: stream start: bad offset %x", cmd_pos, offset32);
                stream->bad_start_warned = 1;
              }
              stream->playing = 0;
              break;
            }
            stream->pos = block->start + offset32;
          }
          stream->playing = 1;
          streams_playing = 1;
          break;
        case 0x94: // Stop Stream
          id = *fdata++;
          streams_playing = 0;
          if (id == 0xff) {
            for (i = 0; i < vgm->stream_count; i++)
              vgm->streams[i].playing = 0;
            break;
          }
          if (!(stream = stream_get(vgm, cmd_pos, id)))
            break;
          if (vgm->streams[id].playing)
            vgm->streams[id].playing = 0;
          for (i = 0; i < vgm->stream_count; i++) {
            stream = &vgm->streams[i];
            if (stream->playing) {
              // refresh since the current stream is done now
              stream->new_sample = 1;
              streams_playing = 1;
            }
          }
          break;
        case 0x95: // Start Stream (fast call)
          id = *fdata++;
          data16 = get16(&fdata);
          flags = *fdata++;
          if (!(stream = stream_get(vgm, cmd_pos, id)))
            break;
          if (!try_start_stream(vgm, stream, cmd_pos, data16, flags))
            break;
          stream->playing = 1;
          streams_playing = 1;
          break;
        case 0xB1: // RF5C164, write
          addr = *fdata++;
          data = *fdata++;
          pcd_pcm_write(addr, data);
          break;
        case 0xe0: // seek to offset
          offset32 = get32(&fdata);
          if (offset32 < vgm->blocks[0].end - vgm->blocks[0].start) // always block 0?
            vgm->blocks[0].pos = vgm->blocks[0].start + offset32;
          else
            elprintf(EL_VGM | EL_ANOMALY, "vgm: %06zx: cmd E0 out of range: %x/%x",
                cmd_pos, offset32, vgm->blocks[0].end - vgm->blocks[0].start);
          break;
        default:
          elprintf(EL_VGM, "vgm: %06zx: skipping cmd %02x", cmd_pos, cmd);
          if (cmd >= 0xE0u)
            fdata += 4;
          else if (cmd >= 0xC0u)
            fdata += 3;
          else if (cmd >= 0xA0u)
            fdata += 2;
          else if ((cmd & 0xf0) == 0x40u)
            fdata += CPU_LE4(vgm->hdr.version) >= 0x160 ? 2 : 1;
          else if ((cmd & 0xf0) == 0x30u)
            fdata += 1;
      }
      vgm->data_pos = fdata - vgm->data;
    }
    if (!streams_playing) {
      sample_pos_stream = vgm->sample_pos;
      continue;
    }
    // stream to cmd catch-up
    {
      u32 stream_sample_to = min(vgm->sample_pos, samples_per_frame);
      int check_for_new_sample = 1;
      for (; sample_pos_stream < stream_sample_to; sample_pos_stream++)
      {
        if (check_for_new_sample) {
          int do_dac_write = 0;
          dacout_stream = 0;
          for (i = 0; i < vgm->stream_count; i++) {
            stream = &vgm->streams[i];
            if (!stream->playing)
              continue;
            do_dac_write |= vgm->streams[i].new_sample;
            vgm->streams[i].new_sample = 0;
            dacout_stream += ((int)vgm->data[stream->pos] - 0x80) << DAC_SHIFT;
          }
          if (do_dac_write) {
            Pico.t.z80c_aim = sample_pos_stream * osc_cyc_per_smp / (15 * 256u);
            ym2612.OPN.ST.address = 0x2a;
            ym2612.addr_A1 = 0;
            PsndDoDAC(z80_cyclesDone());
            ym2612.dacout = dacout_stream + vgm->dacout_nonstream;
          }
          check_for_new_sample = 0;
        }
        // advance all streams
        streams_playing = 0;
        for (i = 0; i < vgm->stream_count; i++) {
          u32 inc;
          stream = &vgm->streams[i];
          if (!stream->playing)
            continue;
          stream->pos_fraction += stream->pos_inc;
          inc = stream->pos_fraction >> 24;
          if (inc) {
            stream->pos_fraction &= 0xffffff;
            stream->pos += inc;
            if (stream->pos >= vgm->blocks[stream->block_i].end)
              stream->playing = 0;
            else
              check_for_new_sample = stream->new_sample = 1;
          }
          streams_playing |= stream->playing;
        }
      } // for sample_pos_stream
    }
  }
  // for a long wait, do only the current frame's worth of samples
  Pico.t.z80c_aim = samples_per_frame * osc_cyc_per_smp / (15 * 256u);
  SekCycleCntS68k = s68k_base + (samples_per_frame * scd_cyc_per_smp >> 8);
  vgm->sample_pos -= samples_per_frame;
  PsndDoSMSFM(Pico.t.z80c_aim);
  PsndGetSamples(0);
  Pico.m.frame_count++;
}

void vgm_reset(void)
{
  if (g_vgm)
    vgm_reset_p(g_vgm, NULL);
}

void vgm_finish(void)
{
  if (g_vgm) {
    free(g_vgm->streams);
    free(g_vgm);
    g_vgm = NULL;
  }
}

// vim:ts=2:sw=2:expandtab
