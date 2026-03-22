#include <stdint.h>
#include <stddef.h>

#include "wav_player.h"

#define WAV_SAMPLE_RATE   48000u
#define WAV_CHANNELS      2u
#define WAV_BITS_PER_SAMP 16u
#define WAV_BLOCK_ALIGN   4u
#define WAV_BYTE_RATE     (WAV_SAMPLE_RATE * WAV_BLOCK_ALIGN)
#define EMBEDDED_CLIP_SECONDS 2u
#define EMBEDDED_CLIP_FRAMES  (WAV_SAMPLE_RATE * EMBEDDED_CLIP_SECONDS)

typedef struct
{
	uint32_t t;
} EmbeddedSongState;

static void wav_write_u32le(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
	p[2] = (uint8_t)((v >> 16) & 0xFFu);
	p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void wav_write_u16le(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void wav_build_header(uint8_t *wav, uint32_t dataBytes)
{
	uint32_t riffSize = 36u + dataBytes;

	wav[0] = 'R'; wav[1] = 'I'; wav[2] = 'F'; wav[3] = 'F';
	wav_write_u32le(&wav[4], riffSize);
	wav[8] = 'W'; wav[9] = 'A'; wav[10] = 'V'; wav[11] = 'E';

	wav[12] = 'f'; wav[13] = 'm'; wav[14] = 't'; wav[15] = ' ';
	wav_write_u32le(&wav[16], 16u);
	wav_write_u16le(&wav[20], 1u);
	wav_write_u16le(&wav[22], (uint16_t)WAV_CHANNELS);
	wav_write_u32le(&wav[24], WAV_SAMPLE_RATE);
	wav_write_u32le(&wav[28], WAV_BYTE_RATE);
	wav_write_u16le(&wav[32], (uint16_t)WAV_BLOCK_ALIGN);
	wav_write_u16le(&wav[34], (uint16_t)WAV_BITS_PER_SAMP);

	wav[36] = 'd'; wav[37] = 'a'; wav[38] = 't'; wav[39] = 'a';
	wav_write_u32le(&wav[40], dataBytes);
}

static uint8_t embedded_clip_header[44];
static EmbeddedSongState embedded_song_state;
static WAV_EmbeddedClip embedded_clip;

static const int16_t sin_q15_quarter[256] = {
	0, 201, 402, 603, 804, 1005, 1206, 1407, 1608, 1809, 2009, 2210, 2411, 2611, 2811, 3012,
	3212, 3412, 3612, 3811, 4011, 4210, 4410, 4609, 4808, 5006, 5205, 5403, 5601, 5799, 5997, 6194,
	6391, 6588, 6784, 6980, 7176, 7372, 7567, 7762, 7956, 8150, 8344, 8537, 8730, 8923, 9115, 9306,
	9497, 9688, 9878, 10067, 10256, 10445, 10633, 10820, 11007, 11194, 11380, 11565, 11750, 11934, 12118, 12301,
	12483, 12665, 12846, 13027, 13207, 13386, 13565, 13743, 13920, 14097, 14273, 14448, 14623, 14797, 14970, 15142,
	15314, 15485, 15655, 15824, 15993, 16161, 16328, 16494, 16660, 16825, 16989, 17152, 17315, 17477, 17638, 17798,
	17957, 18116, 18273, 18430, 18586, 18741, 18895, 19048, 19201, 19352, 19503, 19653, 19802, 19950, 20097, 20243,
	20388, 20533, 20676, 20819, 20960, 21101, 21240, 21379, 21516, 21653, 21788, 21923, 22056, 22189, 22320, 22451,
	22580, 22709, 22836, 22963, 23088, 23212, 23336, 23458, 23579, 23699, 23818, 23937, 24054, 24170, 24285, 24399,
	24512, 24623, 24734, 24844, 24952, 25059, 25166, 25271, 25375, 25478, 25580, 25681, 25780, 25879, 25976, 26073,
	26168, 26262, 26355, 26447, 26538, 26628, 26716, 26804, 26890, 26976, 27060, 27143, 27225, 27306, 27386, 27465,
	27542, 27619, 27694, 27768, 27841, 27913, 27984, 28054, 28123, 28190, 28257, 28322, 28387, 28450, 28512, 28573,
	28633, 28692, 28750, 28807, 28862, 28917, 28970, 29023, 29074, 29124, 29173, 29221, 29268, 29314, 29359, 29403,
	29446, 29488, 29529, 29568, 29607, 29645, 29681, 29717, 29751, 29785, 29817, 29849, 29879, 29909, 29937, 29965,
	29991, 30017, 30041, 30065, 30087, 30109, 30129, 30149, 30167, 30185, 30201, 30217, 30231, 30245, 30258, 30269,
	30280, 30290, 30298, 30306, 30313, 30319, 30324, 30328, 30331, 30334, 30335, 30336, 30336, 30335, 30334, 30331
};

static int16_t sin_q15_from_phase(uint32_t phase)
{
	uint16_t p = (uint16_t)phase;
	uint16_t quadrant = (p >> 14) & 0x3u;
	uint16_t idx = (p >> 6) & 0xFFu;

	switch (quadrant)
	{
		case 0: return sin_q15_quarter[idx];
		case 1: return sin_q15_quarter[255u - idx];
		case 2: return (int16_t)-sin_q15_quarter[idx];
		default: return (int16_t)-sin_q15_quarter[255u - idx];
	}
}

static uint16_t note_frequency_for_frame(uint32_t frame)
{
	static const uint16_t melodyHz[8] = { 392u, 494u, 523u, 587u, 659u, 587u, 523u, 494u };
	uint32_t segment = frame / (WAV_SAMPLE_RATE / 4u);
	if (segment >= 8u)
		segment = 7u;
	return melodyHz[segment];
}

static int Embedded_MP3ish_Generator(int16_t *out, uint32_t frames, void *user)
{
	EmbeddedSongState *state = (EmbeddedSongState *)user;
	if (state == NULL || out == NULL)
		return -1;

	for (uint32_t i = 0; i < frames; i++)
	{
		uint32_t frameIndex = state->t;
		uint16_t freq = note_frequency_for_frame(frameIndex);
		uint16_t harmony = (uint16_t)(freq / 2u);
		uint32_t phaseLead = (uint32_t)(((uint64_t)frameIndex * (uint64_t)freq * 65536ull) / WAV_SAMPLE_RATE);
		uint32_t phaseHarmony = (uint32_t)(((uint64_t)frameIndex * (uint64_t)harmony * 65536ull) / WAV_SAMPLE_RATE);
		uint32_t localFrame = frameIndex % (WAV_SAMPLE_RATE / 4u);
		uint32_t attack = 400u;
		uint32_t release = 1200u;
		uint32_t noteFrames = WAV_SAMPLE_RATE / 4u;
		uint32_t envQ15 = 32767u;

		if (localFrame < attack)
		{
			envQ15 = (localFrame * 32767u) / attack;
		}
		else if (localFrame > (noteFrames - release))
		{
			uint32_t tail = noteFrames - localFrame;
			envQ15 = (tail * 32767u) / release;
		}

		int32_t lead = (sin_q15_from_phase(phaseLead) * 14000) / 32767;
		int32_t pad = (sin_q15_from_phase(phaseHarmony) * 7000) / 32767;
		int32_t bright = (((phaseLead & 0x8000u) != 0u) ? 2200 : -2200);
		int32_t mixed = lead + pad + bright;
		mixed = (mixed * (int32_t)envQ15) / 32767;

		if (mixed > 24000) mixed = 24000;
		if (mixed < -24000) mixed = -24000;

		out[2u * i + 0u] = (int16_t)mixed;
		out[2u * i + 1u] = (int16_t)(mixed * 9 / 10);
		state->t++;
	}

	return 0;
}

void WAV_Data_Init(void)
{
	wav_build_header(embedded_clip_header, EMBEDDED_CLIP_FRAMES * WAV_BLOCK_ALIGN);
	embedded_song_state.t = 0u;
	embedded_clip.wavHeader = embedded_clip_header;
	embedded_clip.wavHeaderSize = sizeof(embedded_clip_header);
	embedded_clip.totalFrames = EMBEDDED_CLIP_FRAMES;
	embedded_clip.generator = Embedded_MP3ish_Generator;
	embedded_clip.user = &embedded_song_state;
}

const WAV_EmbeddedClip *WAV_Data_GetEmbeddedClip(void)
{
	embedded_song_state.t = 0u;
	return &embedded_clip;
}

uint32_t WAV_Data_GetEmbeddedClipDurationMs(void)
{
	return EMBEDDED_CLIP_SECONDS * 1000u;
}