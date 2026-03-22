#include "mp3_decoder.h"

#include <string.h>

/*
 * Decoder scaffold contract
 * -------------------------
 * Inputs:
 * - Compressed MP3 byte stream via MP3_ByteSource
 * Outputs:
 * - Decoded stereo 16-bit PCM frames at 48 kHz
 * Error modes:
 * - NEED_MORE_INPUT while waiting for more bytes
 * - STREAM_END when no more frames remain
 * - UNSUPPORTED/ERROR for real decoder failures later
 *
 * Current placeholder behavior:
 * - Validates the control flow only.
 * - Emits a short synthetic musical phrase chunk-by-chunk.
 * - Advances source offset so later real frame consumption fits the same shape.
 */

#define MP3_SCAFFOLD_TOTAL_FRAMES 96000u
#define MP3_SCAFFOLD_CHUNK_FRAMES 1152u
#define MP3_SCAFFOLD_SIMULATED_FRAME_BYTES 144u

static int16_t g_mp3_scaffold_pcm[MP3_SCAFFOLD_CHUNK_FRAMES * MP3_DECODER_OUTPUT_CHANNELS];

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

static int16_t mp3_scaffold_sin_q15(uint32_t phase)
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

static uint16_t mp3_scaffold_note_for_frame(uint32_t frameIndex)
{
    static const uint16_t melodyHz[8] = { 392u, 440u, 494u, 523u, 587u, 659u, 523u, 440u };
    uint32_t segment = frameIndex / (MP3_DECODER_OUTPUT_SAMPLE_RATE / 4u);
    if (segment >= 8u)
    {
        segment = 7u;
    }
    return melodyHz[segment];
}

static void mp3_scaffold_generate(int16_t *dst, uint32_t startFrame, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; i++)
    {
        uint32_t frameIndex = startFrame + i;
        uint16_t leadHz = mp3_scaffold_note_for_frame(frameIndex);
        uint16_t bassHz = (uint16_t)(leadHz / 2u);
        uint32_t leadPhase = (uint32_t)(((uint64_t)frameIndex * (uint64_t)leadHz * 65536ull) / MP3_DECODER_OUTPUT_SAMPLE_RATE);
        uint32_t bassPhase = (uint32_t)(((uint64_t)frameIndex * (uint64_t)bassHz * 65536ull) / MP3_DECODER_OUTPUT_SAMPLE_RATE);
        int32_t lead = (mp3_scaffold_sin_q15(leadPhase) * 13000) / 32767;
        int32_t bass = (mp3_scaffold_sin_q15(bassPhase) * 6500) / 32767;
        int32_t shimmer = ((leadPhase & 0x8000u) != 0u) ? 1800 : -1800;
        int32_t mix = lead + bass + shimmer;

        if (mix > 22000) mix = 22000;
        if (mix < -22000) mix = -22000;

        dst[2u * i + 0u] = (int16_t)mix;
        dst[2u * i + 1u] = (int16_t)(mix * 9 / 10);
    }
}

static size_t mp3_decoder_refill_input(MP3_Decoder *decoder)
{
    if (decoder == NULL || decoder->source.read == NULL)
    {
        return 0u;
    }

    size_t freeBytes = sizeof(decoder->inputCache) - decoder->cachedBytes;
    if (freeBytes == 0u)
    {
        return 0u;
    }

    size_t readCount = decoder->source.read(decoder->source.context,
                                            &decoder->inputCache[decoder->cachedBytes],
                                            freeBytes);
    decoder->cachedBytes += readCount;
    return readCount;
}

void MP3_ByteSource_Init(MP3_ByteSource *source,
                         MP3_ByteSource_ReadFn readFn,
                         MP3_ByteSource_RewindFn rewindFn,
                         void *context)
{
    if (source == NULL)
    {
        return;
    }

    source->read = readFn;
    source->rewind = rewindFn;
    source->context = context;
}

void MP3_Decoder_GetDefaultConfig(MP3_DecoderConfig *config)
{
    if (config == NULL)
    {
        return;
    }

    config->outputSampleRate = MP3_DECODER_OUTPUT_SAMPLE_RATE;
    config->outputChannels = MP3_DECODER_OUTPUT_CHANNELS;
    config->outputBitsPerSample = MP3_DECODER_OUTPUT_BITS;
}

void MP3_Decoder_Init(MP3_Decoder *decoder,
                      const MP3_ByteSource *source,
                      const MP3_DecoderConfig *config)
{
    if (decoder == NULL)
    {
        return;
    }

    memset(decoder, 0, sizeof(*decoder));

    if (source != NULL)
    {
        decoder->source = *source;
    }

    if (config != NULL)
    {
        decoder->config = *config;
    }
    else
    {
        MP3_Decoder_GetDefaultConfig(&decoder->config);
    }

    decoder->initialized = 1u;
}

void MP3_Decoder_Reset(MP3_Decoder *decoder)
{
    if (decoder == NULL)
    {
        return;
    }

    decoder->cachedBytes = 0u;
    decoder->streamOffset = 0u;
    decoder->streamEnded = 0u;
    decoder->totalFramesProduced = 0u;
    decoder->nextFrameIndex = 0u;

    if (decoder->source.rewind != NULL)
    {
        if (decoder->source.rewind(decoder->source.context) != 0)
        {
            decoder->streamEnded = 1u;
        }
    }
}

MP3_DecodeStatus MP3_Decoder_DecodeFrame(MP3_Decoder *decoder, MP3_DecodeResult *result)
{
    if (decoder == NULL || result == NULL || decoder->initialized == 0u)
    {
        return MP3_DECODER_STATUS_ERROR;
    }

    memset(result, 0, sizeof(*result));
    result->sampleRate = decoder->config.outputSampleRate;
    result->channels = decoder->config.outputChannels;
    result->bitsPerSample = decoder->config.outputBitsPerSample;
    result->status = MP3_DECODER_STATUS_ERROR;

    if (decoder->streamEnded != 0u)
    {
        result->status = MP3_DECODER_STATUS_STREAM_END;
        return result->status;
    }

    if (decoder->source.read == NULL)
    {
        result->status = MP3_DECODER_STATUS_NEED_MORE_INPUT;
        return result->status;
    }

    while (decoder->cachedBytes < MP3_SCAFFOLD_SIMULATED_FRAME_BYTES)
    {
        size_t pulled = mp3_decoder_refill_input(decoder);
        if (pulled == 0u)
        {
            break;
        }
    }

    if (decoder->cachedBytes == 0u)
    {
        decoder->streamEnded = 1u;
        result->status = MP3_DECODER_STATUS_STREAM_END;
        return result->status;
    }

    if (decoder->cachedBytes < MP3_SCAFFOLD_SIMULATED_FRAME_BYTES && decoder->totalFramesProduced == 0u)
    {
        result->status = MP3_DECODER_STATUS_NEED_MORE_INPUT;
        return result->status;
    }

    uint32_t framesLeft = MP3_SCAFFOLD_TOTAL_FRAMES - decoder->totalFramesProduced;
    uint32_t framesThisChunk = framesLeft;
    if (framesThisChunk > MP3_SCAFFOLD_CHUNK_FRAMES)
    {
        framesThisChunk = MP3_SCAFFOLD_CHUNK_FRAMES;
    }

    if (framesThisChunk == 0u)
    {
        decoder->streamEnded = 1u;
        result->status = MP3_DECODER_STATUS_STREAM_END;
        return result->status;
    }

    mp3_scaffold_generate(result->pcm, decoder->nextFrameIndex, framesThisChunk);

    size_t consumed = decoder->cachedBytes;
    if (consumed > MP3_SCAFFOLD_SIMULATED_FRAME_BYTES)
    {
        consumed = MP3_SCAFFOLD_SIMULATED_FRAME_BYTES;
    }
    if (consumed < decoder->cachedBytes)
    {
        memmove(decoder->inputCache,
                &decoder->inputCache[consumed],
                decoder->cachedBytes - consumed);
    }
    decoder->cachedBytes -= consumed;
    decoder->streamOffset += consumed;

    decoder->nextFrameIndex += framesThisChunk;
    decoder->totalFramesProduced += framesThisChunk;
    if (decoder->totalFramesProduced >= MP3_SCAFFOLD_TOTAL_FRAMES)
    {
        decoder->streamEnded = 1u;
    }

    result->frameCount = framesThisChunk;
    result->bytesConsumed = consumed;
    result->status = (decoder->streamEnded != 0u) ? MP3_DECODER_STATUS_STREAM_END : MP3_DECODER_STATUS_OK;

    return result->status;
}

uint8_t MP3_Decoder_IsFinished(const MP3_Decoder *decoder)
{
    if (decoder == NULL)
    {
        return 1u;
    }

    return decoder->streamEnded;
}