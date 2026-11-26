# Changelog

## 1.2.0
- RANDOMIZE button under HUMANIZE: invents a whole new playable pattern (length, filter, RES, PAN, 1/32 splits). DRY/WET, OUTPUT and HUMANIZE are left alone. RES is only used on longer steps, where it is audible.
- Every random pattern is saved automatically as Random 0001, Random 0002 ... in Documents\TrueStep Presets\Random. After 9999 patterns a new folder (Random 2, Random 3 ...) is started.

## 1.1.1
- GUI polish: pixel-exact (non-antialiased) step bars with new colours, straight-line top on PAN/RES level bars, wider DRY/WET box, narrower STEP box, subtitle lowered 2 px.
- Fix: bars and small bars were blurry on Windows (GDI+ antialiasing/half-pixel offset); rectangles are now drawn with antialiasing off and pixel-exact.

## 1.1.0
- HUMANIZE knob (next to DRY/WET): steps start up to 25 ms earlier or later, following pink 1/f noise (Hennig et al. 2011). No added latency.
- A preset saved or loaded in the window stays active in Live.
- PRESET and STEP panels swapped; OUTPUT moved to the right.
- VST3 version.

## 1.0.0
- TrueStep: the step filter/gate from HighLowPass StepFilter, renamed and with the TrueSub/TruePan look.
- 16 steps per bar, each with LENGTH, LOWPASS/HIGHPASS bar, RESONANCE (0-8) and PAN (50L..50R, true stereo constant power).
- 1/32 split per step (each half has its own settings).
- DRY/WET and OUTPUT (-24..+12 dB).
- 12 factory presets (also shown as programs in the host), own presets as .tsp files (SAVE/LOAD).
- Back side (click the logo): credits and manual.
- VST2 and VST3, 64-bit, Windows 7 and later. New plugin ID (TrSt): the old HighLowPass StepFilter plugin is a separate plugin.
