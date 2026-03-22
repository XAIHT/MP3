#include "mp3_data.h"

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

const uint8_t *MP3_Data_GetStubBytes(void)
{
    return g_mp3_stub_bytes;
}

size_t MP3_Data_GetStubSize(void)
{
    return sizeof(g_mp3_stub_bytes);
}
