# MP3 project audio proof-of-concept for STM32F407G-Discovery Board

This project now plays audio through the onboard **CS43L22** codec and the audio jack using a **DMA-backed PCM output path** plus a stronger **MP3 decoder scaffold**.

## What it does now

- Initializes `I2C1` + `I2S3` + `CS43L22`
- Streams stereo 16-bit PCM at **48 kHz** through DMA-backed I2S playback
- Prefers an **MP3 decoder scaffold path** at startup
- Falls back to the embedded WAV-style clip generator if needed
- Repeats playback forever

## Important note

This is still **not true MP3 decoding yet**.

What is real now is the integration shape:

- **byte source layer**: pull compressed bytes from flash today, SD/USB later
- **decoder layer**: consume source bytes and emit PCM chunks
- **output/player layer**: feed decoded PCM into the existing DMA audio transport

That means later you should only need to replace the byte-source implementation and/or the decoder internals, not the DMA transport or `main.c` flow.

## Main files

- `Src/main.c` - board/audio init and startup selection between MP3 scaffold and fallback clip
- `Src/wav_player.c` - DMA PCM streaming to I2S
- `Inc/wav_player.h` - playback API and embedded clip descriptor
- `Src/wav_data.c` - embedded fallback clip generator
- `Inc/mp3_decoder.h` - MP3 byte-source, config, decode-result, and decoder contract
- `Src/mp3_decoder.c` - placeholder decoder implementation using the real source-fed control flow
- `Inc/mp3_player.h` - MP3 player bridge API
- `Src/mp3_player.c` - adapts decoder output into the DMA playback path
- `Inc/mp3_data.h` / `Src/mp3_data.c` - in-memory source helper and stub MP3 bytes

## Current playback contract

- Sample rate: `48000 Hz`
- Channels: `2`
- Sample format: `16-bit PCM`
- Transport: DMA double-buffer playback over I2S to the codec

## Current MP3 scaffold contract

### Byte source layer

Defined in `Inc/mp3_decoder.h`:

- `MP3_ByteSource_ReadFn`
- `MP3_ByteSource_RewindFn`
- `MP3_ByteSource`

The source contract is pull-based:

- `read(context, dst, maxBytes)` copies compressed bytes into `dst`
- `rewind(context)` resets the source back to the start when looping

Provided today in `Inc/mp3_data.h` / `Src/mp3_data.c`:

- `MP3_MemorySourceContext`
- `MP3_Data_InitMemorySource(...)`
- `MP3_Data_BuildMemoryByteSource(...)`

This is the clean swap point for:

- flash-backed MP3 bytes
- SD file reader
- USB MSC file reader

### Decoder layer

Defined in `Inc/mp3_decoder.h`:

- `MP3_DecoderConfig`
- `MP3_DecodeResult`
- `MP3_Decoder`
- `MP3_Decoder_Init(...)`
- `MP3_Decoder_Reset(...)`
- `MP3_Decoder_DecodeFrame(...)`

Today the decoder still synthesizes PCM, but it now does it through a source-fed input cache and returns decoded PCM in a reusable result object. That is the same control shape a real decoder library would use.

### Player/output bridge

Defined in `Inc/mp3_player.h`:

- `MP3_Player_InitFromMemory(...)`
- `MP3_Player_InitFromByteSource(...)`
- `MP3_Player_StartDMA(...)`
- `MP3_Player_Reset(...)`
- `MP3_Player_IsReady(...)`
- `MP3_Player_IsFinished(...)`

`Src/mp3_player.c` adapts the decoder output into the existing `wav_player` DMA path.

## How to swap in a real MP3 decoder later

### Option 1: keep flash-backed MP3

- replace the bytes behind `MP3_Data_GetStubBytes()` / `MP3_Data_GetStubSize()`
- keep using `MP3_Player_InitFromMemory(...)`

### Option 2: move to SD-backed MP3

- create an SD source context struct
- implement `read` and `rewind` callbacks matching `MP3_ByteSource`
- call `MP3_Player_InitFromByteSource(...)`

### Option 3: drop in a real decoder library

Update `Src/mp3_decoder.c` so `MP3_Decoder_DecodeFrame(...)`:

- pulls bytes from `decoder->source`
- parses one or more real MP3 frames
- decodes to PCM into `MP3_DecodeResult.pcm`
- sets `frameCount`, `bytesConsumed`, and `status`
- preserves the output format expected by the current audio path

You should keep these output constraints unless you also update the audio path:

- `48000 Hz`
- stereo
- 16-bit PCM

## Try it in STM32CubeIDE

1. Clean the project.
2. Build the project.
3. Flash it to the STM32F407 board.
4. Plug headphones or powered speakers into the audio jack.

You should hear repeating audio using the current scaffold path. Once a real decoder is dropped into `Src/mp3_decoder.c`, the same player and DMA transport can be reused for actual MP3 playback.
