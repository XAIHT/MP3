#ifndef MP3_DATA_H
#define MP3_DATA_H

#include <stddef.h>
#include <stdint.h>

#include "mp3_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const uint8_t *data;
    size_t size;
    size_t offset;
} MP3_MemorySourceContext;

void MP3_Data_InitMemorySource(MP3_MemorySourceContext *context,
                               const uint8_t *data,
                               size_t size);
size_t MP3_Data_MemoryRead(void *context, uint8_t *dst, size_t maxBytes);
int MP3_Data_MemoryRewind(void *context);
void MP3_Data_BuildMemoryByteSource(MP3_ByteSource *source,
                                    MP3_MemorySourceContext *context);

const uint8_t *MP3_Data_GetStubBytes(void);
size_t MP3_Data_GetStubSize(void);

#ifdef __cplusplus
}
#endif

#endif /* MP3_DATA_H */