#ifndef WAV_PLAYER_H
#define WAV_PLAYER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Minimal WAV (RIFF) player for PCM 16-bit stereo streams.
 *
 * Contract:
 * - Input WAV must be RIFF/WAVE, PCM (audioFormat=1), 16-bit, 2 channels.
 * - Sample rate must match the configured I2S clock (project default: 48 kHz).
 * - Data is streamed out via blocking HAL_I2S_Transmit() in small chunks.
 *
 * Returns 0 on success, negative on error.
 */
int WAV_Player_PlayFromMemory(const uint8_t *wav, size_t wavSize);

/**
 * WAV playback with a PCM generator callback.
 *
 * Plays a valid RIFF/WAVE header from memory (used only for format metadata),
 * but PCM samples are produced on-demand by the callback and streamed out.
 *
 * The callback must fill `frames` of stereo 16-bit interleaved PCM into `out`.
 * Return 0 on success.
 */
typedef int (*WAV_PcmGenerator)(int16_t *out, uint32_t frames, void *user);

int WAV_Player_PlayGenerated(const uint8_t *wavHeader, size_t wavHeaderSize,
		uint32_t framesToPlay,
		WAV_PcmGenerator gen,
		void *user);

/**
 * Small source descriptor for embedded flash-backed audio clips.
 *
 * Current implementation still streams PCM via a generator callback, but this
 * shape is intentionally compatible with future SD/MP3 decode plumbing.
 */
typedef struct
{
	const uint8_t *wavHeader;
	size_t wavHeaderSize;
	uint32_t totalFrames;
	WAV_PcmGenerator generator;
	void *user;
} WAV_EmbeddedClip;

int WAV_Player_PlayClip(const WAV_EmbeddedClip *clip);

int WAV_Player_StartClipDMA(const WAV_EmbeddedClip *clip);
void WAV_Player_Process(void);
uint8_t WAV_Player_IsBusy(void);
uint8_t WAV_Player_IsClipDone(void);
void WAV_Player_StopDMA(void);

#ifdef __cplusplus
}
#endif

#endif /* WAV_PLAYER_H */