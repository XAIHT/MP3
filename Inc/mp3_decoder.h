#ifndef MP3_DECODER_H
#define MP3_DECODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MP3_DECODER_OUTPUT_SAMPLE_RATE 48000u
#define MP3_DECODER_OUTPUT_CHANNELS    2u
#define MP3_DECODER_OUTPUT_BITS        16u
#define MP3_DECODER_MAX_PCM_FRAMES     1152u
#define MP3_DECODER_MAX_FRAME_BYTES    2048u

typedef enum
{
    MP3_DECODER_STATUS_OK = 0,
    MP3_DECODER_STATUS_NEED_MORE_INPUT = 1,
    MP3_DECODER_STATUS_STREAM_END = 2,
    MP3_DECODER_STATUS_UNSUPPORTED = -1,
    MP3_DECODER_STATUS_ERROR = -2
} MP3_DecodeStatus;

typedef size_t (*MP3_ByteSource_ReadFn)(void *context, uint8_t *dst, size_t maxBytes);
typedef int (*MP3_ByteSource_RewindFn)(void *context);

typedef struct
{
    MP3_ByteSource_ReadFn read;
    MP3_ByteSource_RewindFn rewind;
    void *context;
} MP3_ByteSource;

typedef struct
{
    uint32_t outputSampleRate;
    uint8_t outputChannels;
    uint8_t outputBitsPerSample;
} MP3_DecoderConfig;

typedef struct
{
    int16_t pcm[MP3_DECODER_MAX_PCM_FRAMES * MP3_DECODER_OUTPUT_CHANNELS];
    uint32_t frameCount;
    uint32_t sampleRate;
    uint8_t channels;
    uint8_t bitsPerSample;
    MP3_DecodeStatus status;
    size_t bytesConsumed;
} MP3_DecodeResult;

typedef struct
{
    MP3_ByteSource source;
    MP3_DecoderConfig config;
    uint8_t inputCache[MP3_DECODER_MAX_FRAME_BYTES];
    size_t cachedBytes;
    size_t streamOffset;
    uint8_t initialized;
    uint8_t streamEnded;
    uint32_t totalFramesProduced;
    uint32_t nextFrameIndex;
} MP3_Decoder;

void MP3_ByteSource_Init(MP3_ByteSource *source,
                         MP3_ByteSource_ReadFn readFn,
                         MP3_ByteSource_RewindFn rewindFn,
                         void *context);
void MP3_Decoder_GetDefaultConfig(MP3_DecoderConfig *config);
void MP3_Decoder_Init(MP3_Decoder *decoder,
                      const MP3_ByteSource *source,
                      const MP3_DecoderConfig *config);
void MP3_Decoder_Reset(MP3_Decoder *decoder);
MP3_DecodeStatus MP3_Decoder_DecodeFrame(MP3_Decoder *decoder, MP3_DecodeResult *result);
uint8_t MP3_Decoder_IsFinished(const MP3_Decoder *decoder);

#ifdef __cplusplus
}
#endif

#endif /* MP3_DECODER_H */