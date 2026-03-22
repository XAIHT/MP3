# MP3 project audio proof-of-concept

This project now plays a short **embedded 2-second audio clip** from flash through the onboard **CS43L22** codec and the audio jack.

## What it does now

- Initializes `I2C1` + `I2S3` + `CS43L22`
- Streams stereo 16-bit PCM at **48 kHz** through DMA-backed I2S playback
- Prefers an **MP3 decoder scaffold path** at startup
- Falls back to the embedded WAV-style clip generator if needed
- Repeats playback forever

## Important note

This is **not true MP3 decoding yet**.

The project now contains a **real integration scaffold** for future MP3 support:

- byte source abstraction (`flash` today, `SD/USB` later)
- decoder abstraction (`stub` today, real decoder later)
- player bridge that converts decoded PCM chunks into the existing DMA audio path

Right now the decoder scaffold outputs generated PCM while preserving the control flow you’ll need for a real MP3 library.

## Main files

- `Src/main.c` - board/audio init and startup selection between MP3 scaffold and fallback clip
- `Src/wav_player.c` - DMA PCM streaming to I2S
- `Inc/wav_player.h` - playback API and embedded clip descriptor
- `Src/wav_data.c` - embedded fallback clip generator
- `Inc/mp3_decoder.h` - MP3 decoder and byte-source contract
- `Src/mp3_decoder.c` - placeholder decoder implementation with decode-result flow
- `Inc/mp3_player.h` - MP3 player bridge API
- `Src/mp3_player.c` - adapts decoder output into the DMA audio path
- `Inc/mp3_data.h` / `Src/mp3_data.c` - stub MP3 byte container in flash

## Current playback contract

- Sample rate: `48000 Hz`
- Channels: `2`
- Sample format: `16-bit PCM`
- Transport: DMA double-buffer playback over I2S to the codec

## How to swap in a real MP3 decoder later

### 1) Replace the byte source

Today:
- `MP3_Player_InitFromMemory(&ctx, MP3_Data_GetStubBytes(), MP3_Data_GetStubSize())`

Later options:
- flash-backed MP3 bytes
- SD-backed file reader
- USB MSC-backed file reader

The byte source contract lives in `Inc/mp3_decoder.h` as `MP3_ByteSource`.

### 2) Replace the decoder internals

Update `Src/mp3_decoder.c` so `MP3_Decoder_DecodeFrame(...)`:
- parses real MP3 frames
- decodes them to PCM
- fills `MP3_DecodeResult`
- advances `bytesConsumed`
- returns `MP3_DECODER_STATUS_OK`, `...NEED_MORE_INPUT`, or `...STREAM_END`

You should keep these output constraints unless you also update the audio path:
- `48000 Hz`
- stereo
- 16-bit PCM

### 3) Keep the player bridge

`Src/mp3_player.c` already adapts decoder output into the existing DMA playback path. In the normal case, you should not need to change the app loop in `Src/main.c`.

## Try it in STM32CubeIDE

1. Clean the project.
2. Build the project.
3. Flash it to the STM32F407 board.
4. Plug headphones or powered speakers into the audio jack.

You should hear repeating audio using the current scaffold path. Once a real decoder is dropped in, the same path can play decoded MP3 PCM.