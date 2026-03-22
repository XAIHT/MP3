#include "mp3_player.h"

#include <string.h>

#define MP3_PLAYER_FAKE_WAV_HEADER_SIZE 44u
#define MP3_PLAYER_FALLBACK_TOTAL_FRAMES 96000u

typedef struct
{
    MP3_PlayerContext *context;
    int16_t frameScratch[128u * 2u];
    uint32_t scratchFrames;
    uint32_t scratchIndex;
} MP3_PlayerGeneratorState;

static uint8_t g_mp3_fake_wav_header[MP3_PLAYER_FAKE_WAV_HEADER_SIZE];
static MP3_PlayerGeneratorState g_mp3_generator_state;

static void mp3_write_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void mp3_write_u16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void MP3_Player_BuildPcmHeader(uint8_t *wav, uint32_t dataBytes)
{
    uint32_t riffSize = 36u + dataBytes;

    wav[0] = 'R'; wav[1] = 'I'; wav[2] = 'F'; wav[3] = 'F';
    mp3_write_u32le(&wav[4], riffSize);
    wav[8] = 'W'; wav[9] = 'A'; wav[10] = 'V'; wav[11] = 'E';

    wav[12] = 'f'; wav[13] = 'm'; wav[14] = 't'; wav[15] = ' ';
    mp3_write_u32le(&wav[16], 16u);
    mp3_write_u16le(&wav[20], 1u);
    mp3_write_u16le(&wav[22], MP3_DECODER_OUTPUT_CHANNELS);
    mp3_write_u32le(&wav[24], MP3_DECODER_OUTPUT_SAMPLE_RATE);
    mp3_write_u32le(&wav[28], MP3_DECODER_OUTPUT_SAMPLE_RATE * MP3_DECODER_OUTPUT_CHANNELS * 2u);
    mp3_write_u16le(&wav[32], MP3_DECODER_OUTPUT_CHANNELS * 2u);
    mp3_write_u16le(&wav[34], MP3_DECODER_OUTPUT_BITS);

    wav[36] = 'd'; wav[37] = 'a'; wav[38] = 't'; wav[39] = 'a';
    mp3_write_u32le(&wav[40], dataBytes);
}

static int MP3_Player_Generator(int16_t *out, uint32_t frames, void *user)
{
    MP3_PlayerGeneratorState *state = (MP3_PlayerGeneratorState *)user;
    if (state == NULL || state->context == NULL || out == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < frames; i++)
    {
        if (state->scratchIndex >= state->scratchFrames)
        {
            MP3_DecodeResult result;
            MP3_DecodeStatus status = MP3_Decoder_DecodeFrame(&state->context->decoder, &result);
            if (status == MP3_DECODER_STATUS_NEED_MORE_INPUT || result.samples == NULL || result.frameCount == 0u)
            {
                return -2;
            }
            if (result.sampleRate != MP3_DECODER_OUTPUT_SAMPLE_RATE ||
                result.channels != MP3_DECODER_OUTPUT_CHANNELS ||
                result.bitsPerSample != MP3_DECODER_OUTPUT_BITS)
            {
                return -3;
            }

            memcpy(state->frameScratch,
                   result.samples,
                   result.frameCount * MP3_DECODER_OUTPUT_CHANNELS * sizeof(int16_t));
            state->scratchFrames = result.frameCount;
            state->scratchIndex = 0u;
        }

        out[2u * i + 0u] = state->frameScratch[2u * state->scratchIndex + 0u];
        out[2u * i + 1u] = state->frameScratch[2u * state->scratchIndex + 1u];
        state->scratchIndex++;
    }

    return 0;
}

void MP3_Player_InitFromMemory(MP3_PlayerContext *context, const uint8_t *mp3Data, size_t mp3Size)
{
    MP3_ByteSource source;

    if (context == NULL)
    {
        return;
    }

    memset(context, 0, sizeof(*context));
    memset(&g_mp3_generator_state, 0, sizeof(g_mp3_generator_state));

    MP3_ByteSource_Init(&source, mp3Data, mp3Size);
    MP3_Decoder_Init(&context->decoder, &source);

    MP3_Player_BuildPcmHeader(g_mp3_fake_wav_header,
                              MP3_PLAYER_FALLBACK_TOTAL_FRAMES * MP3_DECODER_OUTPUT_CHANNELS * 2u);

    g_mp3_generator_state.context = context;

    context->clip.wavHeader = g_mp3_fake_wav_header;
    context->clip.wavHeaderSize = sizeof(g_mp3_fake_wav_header);
    context->clip.totalFrames = MP3_PLAYER_FALLBACK_TOTAL_FRAMES;
    context->clip.generator = MP3_Player_Generator;
    context->clip.user = &g_mp3_generator_state;
    context->active = 1u;
}

const WAV_EmbeddedClip *MP3_Player_GetClip(MP3_PlayerContext *context)
{
    if (context == NULL || context->active == 0u)
    {
        return NULL;
    }

    MP3_Decoder_Reset(&context->decoder);
    g_mp3_generator_state.context = context;
    g_mp3_generator_state.scratchFrames = 0u;
    g_mp3_generator_state.scratchIndex = 0u;
    return &context->clip;
}

int MP3_Player_StartDMA(MP3_PlayerContext *context)
{
    const WAV_EmbeddedClip *clip = MP3_Player_GetClip(context);
    if (clip == NULL)
    {
        return -1;
    }

    return WAV_Player_StartClipDMA(clip);
}

uint8_t MP3_Player_IsReady(const MP3_PlayerContext *context)
{
    return (context != NULL) && (context->active != 0u);
}
