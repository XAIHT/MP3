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

typedef enum
{
    MP3_DECODER_STATUS_OK = 0,
    MP3_DECODER_STATUS_NEED_MORE_INPUT = 1,
    MP3_DECODER_STATUS_STREAM_END = 2,
    MP3_DECODER_STATUS_UNSUPPORTED = -1,
    MP3_DECODER_STATUS_ERROR = -2
} MP3_DecodeStatus;

typedef struct
{
    const uint8_t *data;
    size_t size;
    size_t offset;
} MP3_ByteSource;

typedef struct
{
    const int16_t *samples;
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
    uint8_t initialized;
    uint8_t streamEnded;
    uint32_t totalFramesProduced;
    uint32_t nextFrameIndex;
} MP3_Decoder;

void MP3_ByteSource_Init(MP3_ByteSource *source, const uint8_t *data, size_t size);
void MP3_Decoder_Init(MP3_Decoder *decoder, const MP3_ByteSource *source);
void MP3_Decoder_Reset(MP3_Decoder *decoder);
MP3_DecodeStatus MP3_Decoder_DecodeFrame(MP3_Decoder *decoder, MP3_DecodeResult *result);
uint8_t MP3_Decoder_IsFinished(const MP3_Decoder *decoder);

#ifdef __cplusplus
}
#endif

#endif /* MP3_DECODER_H */
