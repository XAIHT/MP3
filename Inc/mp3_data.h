#ifndef MP3_DATA_H
#define MP3_DATA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const uint8_t *MP3_Data_GetStubBytes(void);
size_t MP3_Data_GetStubSize(void);

#ifdef __cplusplus
}
#endif

#endif /* MP3_DATA_H */
