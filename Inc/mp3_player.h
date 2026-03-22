#ifndef MP3_PLAYER_H
#define MP3_PLAYER_H

#include <stdint.h>

#include "mp3_decoder.h"
#include "wav_player.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    MP3_Decoder decoder;
    WAV_EmbeddedClip clip;
    uint8_t active;
} MP3_PlayerContext;

void MP3_Player_InitFromMemory(MP3_PlayerContext *context, const uint8_t *mp3Data, size_t mp3Size);
const WAV_EmbeddedClip *MP3_Player_GetClip(MP3_PlayerContext *context);
int MP3_Player_StartDMA(MP3_PlayerContext *context);
uint8_t MP3_Player_IsReady(const MP3_PlayerContext *context);

#ifdef __cplusplus
}
#endif

#endif /* MP3_PLAYER_H */
