#include "mp3_data.h"

#include <string.h>

/*
 * Placeholder MP3 byte stream container.
 *
 * Replace this array later with real MP3 frame bytes from flash, or ignore this
 * file entirely when your byte source comes from SD/USB.
 */
static const uint8_t g_mp3_stub_bytes[] = {
    'I','D','3', 0x04, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x15,
    'T','I','T','2', 0x00, 0x00, 0x00, 0x05,
    0x00, 0x00, 0x03, 'D', 'e', 'm', 'o'
};

void MP3_Data_InitMemorySource(MP3_MemorySourceContext *context,
                               const uint8_t *data,
                               size_t size)
{
    if (context == NULL)
    {
        return;
    }

    context->data = data;
    context->size = size;
    context->offset = 0u;
}

size_t MP3_Data_MemoryRead(void *context, uint8_t *dst, size_t maxBytes)
{
    MP3_MemorySourceContext *memory = (MP3_MemorySourceContext *)context;
    if (memory == NULL || dst == NULL || maxBytes == 0u || memory->data == NULL)
    {
        return 0u;
    }

    size_t remaining = 0u;
    if (memory->offset < memory->size)
    {
        remaining = memory->size - memory->offset;
    }

    size_t chunk = (remaining < maxBytes) ? remaining : maxBytes;
    if (chunk == 0u)
    {
        return 0u;
    }

    memcpy(dst, &memory->data[memory->offset], chunk);
    memory->offset += chunk;
    return chunk;
}

int MP3_Data_MemoryRewind(void *context)
{
    MP3_MemorySourceContext *memory = (MP3_MemorySourceContext *)context;
    if (memory == NULL)
    {
        return -1;
    }

    memory->offset = 0u;
    return 0;
}

void MP3_Data_BuildMemoryByteSource(MP3_ByteSource *source,
                                    MP3_MemorySourceContext *context)
{
    MP3_ByteSource_Init(source, MP3_Data_MemoryRead, MP3_Data_MemoryRewind, context);
}

const uint8_t *MP3_Data_GetStubBytes(void)
{
    return g_mp3_stub_bytes;
}

size_t MP3_Data_GetStubSize(void)
{
    return sizeof(g_mp3_stub_bytes);
}