# Drum Glue by ROHIT JI

An analog-style **bus compressor channel strip** for drum groups (also great on the mix bus). One page, three sections, every one with its own ON switch.

**GLUE COMPRESSOR > TONE EQ > SATURATION > ANALOG COLOUR**

| Section | What it does |
|---|---|
| **Glue Compressor** | Stereo-linked VCA-style bus compressor. Threshold, Makeup and parallel **Mix**; stepped **Ratio** (1.5 / 2 / 4 / 10), **Attack** (0.1 - 30 ms), **Release** (0.1 - 1.2 s or **AUTO**), **Knee** (Hard / Med / Soft), **NEW** (feed-forward, tight) or **OLD** (feed-back, smoother) character, and **THRUST** so the kick doesn't pump the whole group. |
| **Tone EQ** | **Low Cut**, **Warmth** (low shelf 60-220 Hz), **Bite** (1.5k / 3k / 5k for snare crack) and **Air** (8k / 12k / 16k). |
| **Saturation** | 2x oversampled emulations: **Triode**, **Warmth**, **Tape**, **Analog** (console) and **Retro**, with Drive and Mix. |
| **Analog Colour** | Subtle transformer-style tone on the whole output (can be switched off). |

Analog VU meter (GR / IN / OUT) and 11 presets: Drum Bus Glue, Punchy Kick & Snare, Smash Room, Tape Warmth, Analog Console, Triode Glow, Retro Grit, Gentle Mix Bus, Parallel Crush, Air & Sparkle, Lo-Fi Drums.

## Download

Go to **[Releases](../../releases/latest)** and pick:

| File | For |
|---|---|
| **Drum-Glue-Setup-64bit.exe** | 64-bit FL Studio, Cubase and other modern DAWs (most people) |
| **Drum-Glue-Setup-32bit.exe** | Older 32-bit DAWs only |

Run it and click **Next > Install > Finish**.

## How it is built

Every push to `main` builds the plugin with MSVC (x64 and Win32) on GitHub Actions, packages each
into its own setup wizard (`Installer/setup.iss`, Inno Setup) and publishes them as the latest release.
