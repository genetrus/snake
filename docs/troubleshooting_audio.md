# Audio Troubleshooting

This guide explains how the audio pipeline is initialized and how to interpret the diagnostics added for SDL_mixer.

## Required init sequence

At startup the game logs each step in this order:

1. `SDL_InitSubSystem(SDL_INIT_AUDIO)`
2. `Mix_Init(0)` (WAV does not require additional codecs, but the result is logged)
3. `Mix_OpenAudio(freq, format, channels, chunk_size)`
4. `Mix_QuerySpec()` (actual device format)
5. `Mix_AllocateChannels(count)`

If any step fails, you will see the error from `SDL_GetError` or `Mix_GetError` in the log and the debug overlay will show `AUDIO: FAIL`.

## What the logs mean

The logs are intended to show *exactly* where audio fails:

- **Working directory**: the resolved `assets/sounds/*.wav` paths are relative to the current working directory. If this is unexpected, no WAV files will be found.
- **Mix_OpenAudio parameters**: the requested format (freq/format/channels/chunk) is logged before opening the device.
- **Mix_QuerySpec**: confirms the *actual* format used by the device.
- **Mix_AllocateChannels**: shows how many channels were created.
- **Missing files**: `SFX missing file: ...` is emitted for any missing WAV.
- **Load failures**: `SFX: failed to load WAV ...` includes `Mix_GetError`.
- **Playback failures**: `SFX: Mix_PlayChannel failed ...` includes `Mix_GetError`.
- **Volume and mute**: the applied config values are logged every time audio settings change.

## Common causes of no sound

- **Wrong working directory**: running the executable from a different directory means `assets/sounds/` cannot be resolved.
- **Missing assets copy**: `assets/sounds/*.wav` must be present in your runtime folder.
- **Volume is zero**: `audio.master_volume` or `audio.sfx_volume` set to `0` will mute playback.
- **Audio disabled**: `audio.enabled = false` mutes all audio output.
- **Device open failure**: SDL_mixer could not open the audio device (exclusive mode / device locked / driver issues).
- **Unsupported output format**: the device uses a format SDL_mixer cannot initialize.

## Debug overlay

The audio overlay is toggled with **F10**. It uses the built-in bitmap text fallback, so it does not require SDL_ttf.

Overlay fields:

- `AUDIO: OK/FAIL`
- `Device opened: yes/no`
- `Enabled: yes/no`
- `Master volume: N`
- `SFX volume: N`
- `Loaded sounds: N/expected (fallback M)`
- `Last error: ...`
- `Last play: sound_id -> channel / fail`

## Procedural fallback beeps

If a WAV is missing or fails to load, the game generates a short procedural beep in memory (no disk assets required). This ensures you can still hear:

- Menu click
- Food eaten
- Game over

Distinct pitches are used so you can confirm which event fired.
