#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct { int dummy; } drmp3;

int drmp3_init_file(drmp3 *pMP3, const char *pFilePath, void *pAllocationCallbacks);
void drmp3_uninit(drmp3 *pMP3);
int drmp3_get_pcm_frame_count(drmp3 *pMP3);
int drmp3_read_pcm_frames_s16(drmp3 *pMP3, int framesToRead, short *pBufferOut);
int drmp3_get_channels(drmp3 *pMP3);
int drmp3_get_sample_rate(drmp3 *pMP3);
int drmp3_seek_to_pcm_frame(drmp3 *pMP3, int frameIndex);

#ifdef __cplusplus
}
#endif
