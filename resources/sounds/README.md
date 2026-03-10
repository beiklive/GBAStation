# UI Sound Files

This directory contains WAV sound-effect files used by BeikLiveStation's
desktop audio player (`BKAudioPlayer`).

## Format requirements
- Format : PCM (uncompressed)
- Bit depth : 16-bit signed
- Channels : Mono or Stereo (mono is automatically upmixed to stereo)
- Sample rate : 44100 Hz or 48000 Hz recommended

## Required files

| File name            | Triggered by                                    |
|----------------------|-------------------------------------------------|
| `focus_change.wav`   | Focus moves between UI elements                 |
| `focus_error.wav`    | Impossible navigation attempt (highlight shake) |
| `click.wav`          | Button / action confirmed                       |
| `back.wav`           | Back / cancel action                            |
| `focus_sidebar.wav`  | Focus moves to a sidebar item                   |
| `click_error.wav`    | Click on a disabled button                      |
| `honk.wav`           | Special honk effect                             |
| `click_sidebar.wav`  | Sidebar item confirmed                          |
| `touch_unfocus.wav`  | Touch focus interrupted                         |
| `touch.wav`          | Generic touch event                             |
| `slider_tick.wav`    | Slider value changes (each step)                |
| `slider_release.wav` | Slider released                                 |

## Notes
- If a file is missing the sound effect is silently skipped (no crash).
- On Nintendo Switch the system uses the built-in qlaunch BFSAR sounds
  (via libpulsar); these WAV files are **not** used on Switch.
