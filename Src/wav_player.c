#include "wav_player.h"

#include "i2s.h"
#include "stm32f4xx_hal.h"

#include <string.h>

extern I2S_HandleTypeDef hi2s3;

#define WAV_MIN_SIZE 44u
#define DMA_CHUNK_FRAMES 128u
#define DMA_TX_WORDS_PER_FRAME 4u
#define DMA_HALF_WORDS (DMA_CHUNK_FRAMES * DMA_TX_WORDS_PER_FRAME)
#define DMA_BUFFER_WORDS (DMA_HALF_WORDS * 2u)

typedef struct
{
	const WAV_EmbeddedClip *clip;
	uint32_t framesRemaining;
	uint8_t active;
	uint8_t done;
} WAV_DmaState;

static WAV_DmaState g_dmaState;
static int16_t g_dmaPcmScratch[DMA_CHUNK_FRAMES * 2u];
static uint16_t g_dmaTxBuffer[DMA_BUFFER_WORDS];

static uint32_t rd_u32le(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t rd_u16le(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int WAV_Player_ValidateHeader(const uint8_t *wavHeader, size_t wavHeaderSize)
{
	if (wavHeader == NULL || wavHeaderSize < WAV_MIN_SIZE)
		return -1;
	if (memcmp(wavHeader + 0, "RIFF", 4) != 0)
		return -2;
	if (memcmp(wavHeader + 8, "WAVE", 4) != 0)
		return -3;

	size_t off = 12u;
	const uint8_t *fmt = NULL;
	uint32_t fmtSize = 0u;

	while (off + 8u <= wavHeaderSize)
	{
		const uint8_t *chunk = wavHeader + off;
		uint32_t chunkSize = rd_u32le(chunk + 4);
		size_t payloadOff = off + 8u;
		size_t next = payloadOff + chunkSize;
		if (next & 1u)
			next++;
		if (next > wavHeaderSize)
			break;

		if (memcmp(chunk, "fmt ", 4) == 0)
		{
			fmt = wavHeader + payloadOff;
			fmtSize = chunkSize;
			break;
		}
		off = next;
	}

	if (!fmt || fmtSize < 16u)
		return -4;

	if (rd_u16le(fmt + 0) != 1u)
		return -6;
	if (rd_u16le(fmt + 2) != 2u)
		return -7;
	if (rd_u16le(fmt + 14) != 16u)
		return -8;
	if (rd_u32le(fmt + 4) != 48000u)
		return -9;

	return 0;
}

static void WAV_Player_PackPcmToI2S(const int16_t *pcm16, uint16_t *tx, uint32_t frames)
{
	for (uint32_t i = 0; i < frames; i++)
	{
		int16_t l = pcm16[2u * i + 0u];
		int16_t r = pcm16[2u * i + 1u];
		tx[4u * i + 0u] = 0u;
		tx[4u * i + 1u] = (uint16_t)l;
		tx[4u * i + 2u] = 0u;
		tx[4u * i + 3u] = (uint16_t)r;
	}
}

static int WAV_Player_FillDmaHalf(uint16_t *dst)
{
	if (!g_dmaState.active || g_dmaState.clip == NULL || dst == NULL)
		return -30;

	uint32_t frames = g_dmaState.framesRemaining;
	if (frames > DMA_CHUNK_FRAMES)
		frames = DMA_CHUNK_FRAMES;

	if (frames > 0u)
	{
		memset(g_dmaPcmScratch, 0, frames * 2u * sizeof(int16_t));
		if (g_dmaState.clip->generator(g_dmaPcmScratch, frames, g_dmaState.clip->user) != 0)
		{
			memset(dst, 0, DMA_HALF_WORDS * sizeof(uint16_t));
			g_dmaState.framesRemaining = 0u;
			g_dmaState.done = 1u;
			return -31;
		}
		WAV_Player_PackPcmToI2S(g_dmaPcmScratch, dst, frames);
		g_dmaState.framesRemaining -= frames;
	}

	if (frames < DMA_CHUNK_FRAMES)
	{
		memset(&dst[frames * DMA_TX_WORDS_PER_FRAME], 0,
				(DMA_CHUNK_FRAMES - frames) * DMA_TX_WORDS_PER_FRAME * sizeof(uint16_t));
	}

	if (g_dmaState.framesRemaining == 0u)
	{
		g_dmaState.done = 1u;
	}

	return 0;
}

int WAV_Player_PlayFromMemory(const uint8_t *wav, size_t wavSize)
{
	if (wav == NULL || wavSize < WAV_MIN_SIZE)
		return -1;

	// RIFF header
	if (memcmp(wav + 0, "RIFF", 4) != 0)
		return -2;
	if (memcmp(wav + 8, "WAVE", 4) != 0)
		return -3;

	// Walk chunks until we find 'fmt ' and 'data'
	size_t off = 12;
	const uint8_t *fmt = NULL;
	uint32_t fmtSize = 0;
	const uint8_t *data = NULL;
	uint32_t dataSize = 0;

	while (off + 8 <= wavSize)
	{
		const uint8_t *chunk = wav + off;
		uint32_t chunkSize = rd_u32le(chunk + 4);
		size_t payloadOff = off + 8;
		size_t next = payloadOff + chunkSize;
		// Chunks are word-aligned
		if (next & 1u)
			next++;

		if (next > wavSize)
			break;

		if (memcmp(chunk, "fmt ", 4) == 0)
		{
			fmt = wav + payloadOff;
			fmtSize = chunkSize;
		}
		else if (memcmp(chunk, "data", 4) == 0)
		{
			data = wav + payloadOff;
			dataSize = chunkSize;
		}

		off = next;
		if (fmt && data)
			break;
	}

	if (!fmt || fmtSize < 16)
		return -4;
	if (!data || dataSize == 0)
		return -5;

	uint16_t audioFormat = rd_u16le(fmt + 0);
	uint16_t numChannels = rd_u16le(fmt + 2);
	uint32_t sampleRate = rd_u32le(fmt + 4);
	uint16_t bitsPerSample = rd_u16le(fmt + 14);

	if (audioFormat != 1)
		return -6; // only PCM
	if (numChannels != 2)
		return -7; // only stereo
	if (bitsPerSample != 16)
		return -8; // only 16-bit

	// Expect the project I2S to be set to this sample rate.
	// We don't reconfigure clocks here, we just fail early.
	(void)sampleRate;
	if (sampleRate != 48000u)
		return -9;

	// dataSize must be multiple of frame size (4 bytes for stereo 16-bit)
	if ((dataSize % 4u) != 0u)
		return -10;

	// Each stereo frame in the WAV is 2x16-bit = 4 bytes.
	// With I2S 16-bit extended, each channel is 32-bit slot so we must send
	// 4 half-words per frame: 0, L, 0, R.
	enum { CHUNK_FRAMES = 128 };
	uint16_t tx[CHUNK_FRAMES * 4];

	const int16_t *pcm = (const int16_t *)(const void *)data;
	uint32_t framesTotal = dataSize / 4u;

	for (uint32_t frame = 0; frame < framesTotal;)
	{
		uint32_t frames = framesTotal - frame;
		if (frames > CHUNK_FRAMES)
			frames = CHUNK_FRAMES;

		for (uint32_t i = 0; i < frames; i++)
		{
			int16_t l = pcm[2u * (frame + i) + 0u];
			int16_t r = pcm[2u * (frame + i) + 1u];
			tx[4u * i + 0u] = 0u;
			tx[4u * i + 1u] = (uint16_t)l;
			tx[4u * i + 2u] = 0u;
			tx[4u * i + 3u] = (uint16_t)r;
		}

		uint16_t sendWords = (uint16_t)(frames * 4u);
		if (HAL_I2S_Transmit(&hi2s3, tx, sendWords, HAL_MAX_DELAY) != HAL_OK)
			return -11;

		frame += frames;
	}

	return 0;
}

int WAV_Player_PlayGenerated(const uint8_t *wavHeader, size_t wavHeaderSize,
		uint32_t framesToPlay,
		WAV_PcmGenerator gen,
		void *user)
{
	if (wavHeader == NULL || wavHeaderSize < WAV_MIN_SIZE || gen == NULL)
		return -1;

	// Validate header same as WAV_Player_PlayFromMemory()
	if (memcmp(wavHeader + 0, "RIFF", 4) != 0)
		return -2;
	if (memcmp(wavHeader + 8, "WAVE", 4) != 0)
		return -3;

	// Find fmt chunk
	size_t off = 12;
	const uint8_t *fmt = NULL;
	uint32_t fmtSize = 0;

	while (off + 8 <= wavHeaderSize)
	{
		const uint8_t *chunk = wavHeader + off;
		uint32_t chunkSize = rd_u32le(chunk + 4);
		size_t payloadOff = off + 8;
		size_t next = payloadOff + chunkSize;
		if (next & 1u)
			next++;
		if (next > wavHeaderSize)
			break;

		if (memcmp(chunk, "fmt ", 4) == 0)
		{
			fmt = wavHeader + payloadOff;
			fmtSize = chunkSize;
			break;
		}
		off = next;
	}

	if (!fmt || fmtSize < 16)
		return -4;

	uint16_t audioFormat = rd_u16le(fmt + 0);
	uint16_t numChannels = rd_u16le(fmt + 2);
	uint32_t sampleRate = rd_u32le(fmt + 4);
	uint16_t bitsPerSample = rd_u16le(fmt + 14);

	if (audioFormat != 1)
		return -6;
	if (numChannels != 2)
		return -7;
	if (bitsPerSample != 16)
		return -8;
	if (sampleRate != 48000u)
		return -9;

	// Stream using a small staging buffer.
	// We generate stereo 16-bit PCM frames, then pack into the I2S TX format.
	enum { CHUNK_FRAMES = 128 };
	int16_t pcm16[CHUNK_FRAMES * 2];
	uint16_t tx[CHUNK_FRAMES * 4]; // 16b extended: 4 half-words per stereo frame

	uint32_t framesDone = 0;
	while (framesDone < framesToPlay)
	{
		uint32_t frames = framesToPlay - framesDone;
		if (frames > CHUNK_FRAMES)
			frames = CHUNK_FRAMES;

		memset(pcm16, 0, frames * 2u * sizeof(int16_t));
		if (gen(pcm16, frames, user) != 0)
			return -12;

		// Pack into 16-bit extended (32-bit channel length):
		// [L0]=0, [L1]=sampleL, [R0]=0, [R1]=sampleR
		for (uint32_t i = 0; i < frames; i++)
		{
			int16_t l = pcm16[2u * i + 0u];
			int16_t r = pcm16[2u * i + 1u];
			tx[4u * i + 0u] = 0u;
			tx[4u * i + 1u] = (uint16_t)l;
			tx[4u * i + 2u] = 0u;
			tx[4u * i + 3u] = (uint16_t)r;
		}

		uint16_t words = (uint16_t)(frames * 4u);
		if (HAL_I2S_Transmit(&hi2s3, tx, words, HAL_MAX_DELAY) != HAL_OK)
			return -11;

		framesDone += frames;
	}

	return 0;
}

int WAV_Player_PlayClip(const WAV_EmbeddedClip *clip)
{
	if (clip == NULL)
		return -20;
	if (clip->wavHeader == NULL || clip->wavHeaderSize < WAV_MIN_SIZE)
		return -21;
	if (clip->generator == NULL)
		return -22;
	if (clip->totalFrames == 0u)
		return -23;

	return WAV_Player_PlayGenerated(clip->wavHeader,
			clip->wavHeaderSize,
			clip->totalFrames,
			clip->generator,
			clip->user);
}

int WAV_Player_StartClipDMA(const WAV_EmbeddedClip *clip)
{
	if (clip == NULL)
		return -20;
	if (clip->generator == NULL)
		return -22;
	if (clip->totalFrames == 0u)
		return -23;

	int rc = WAV_Player_ValidateHeader(clip->wavHeader, clip->wavHeaderSize);
	if (rc != 0)
		return rc;

	if (g_dmaState.active)
		WAV_Player_StopDMA();

	memset(&g_dmaState, 0, sizeof(g_dmaState));
	g_dmaState.clip = clip;
	g_dmaState.framesRemaining = clip->totalFrames;
	g_dmaState.active = 1u;
	g_dmaState.done = 0u;

	if (WAV_Player_FillDmaHalf(&g_dmaTxBuffer[0]) != 0)
	{
		WAV_Player_StopDMA();
		return -32;
	}
	if (WAV_Player_FillDmaHalf(&g_dmaTxBuffer[DMA_HALF_WORDS]) != 0)
	{
		WAV_Player_StopDMA();
		return -33;
	}

	if (HAL_I2S_Transmit_DMA(&hi2s3, g_dmaTxBuffer, DMA_BUFFER_WORDS) != HAL_OK)
	{
		WAV_Player_StopDMA();
		return -34;
	}

	return 0;
}

void WAV_Player_Process(void)
{
	if (!g_dmaState.active)
		return;

	if (g_dmaState.done != 0u)
	{
		if (HAL_I2S_GetState(&hi2s3) == HAL_I2S_STATE_READY)
		{
			WAV_Player_StopDMA();
		}
	}
}

uint8_t WAV_Player_IsBusy(void)
{
	return g_dmaState.active;
}

uint8_t WAV_Player_IsClipDone(void)
{
	return g_dmaState.done;
}

void WAV_Player_StopDMA(void)
{
	if (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_RESET)
	{
		(void)HAL_I2S_DMAStop(&hi2s3);
	}
	memset(g_dmaTxBuffer, 0, sizeof(g_dmaTxBuffer));
	memset(g_dmaPcmScratch, 0, sizeof(g_dmaPcmScratch));
	memset(&g_dmaState, 0, sizeof(g_dmaState));
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if (hi2s != &hi2s3 || !g_dmaState.active)
		return;

	if (g_dmaState.done == 0u)
	{
		(void)WAV_Player_FillDmaHalf(&g_dmaTxBuffer[0]);
	}
	else
	{
		memset(&g_dmaTxBuffer[0], 0, DMA_HALF_WORDS * sizeof(uint16_t));
	}
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if (hi2s != &hi2s3 || !g_dmaState.active)
		return;

	if (g_dmaState.done == 0u)
	{
		(void)WAV_Player_FillDmaHalf(&g_dmaTxBuffer[DMA_HALF_WORDS]);
	}
	else
	{
		memset(&g_dmaTxBuffer[DMA_HALF_WORDS], 0, DMA_HALF_WORDS * sizeof(uint16_t));
		(void)HAL_I2S_DMAStop(&hi2s3);
		g_dmaState.active = 0u;
	}
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
	if (hi2s == &hi2s3)
	{
		WAV_Player_StopDMA();
	}
}