# Changelog

## 1.6.0
- Info box at the top (between the logo and the version): a short explanation appears when the pointer rests on a knob, button, bar or heading. It follows the manual language (ENG | SWE) and can be switched off with INFO ON | OFF on the back page (remembered).

## 1.5.2
- Fix: opening a saved project in Live now keeps the saved sound. Live asks for program 0 (or the same factory preset) right after loading; this no longer overwrites your own preset, a Random pattern or your edits of a factory preset with Default Gate.

## 1.5.1
- Manual: no line breaks inside "0 dB", "20 kHz", "50 %" or "32-delar"; short words stay with the next word (no lonely "En" at the end of a line). The heading is now "WHAT TrueStep DOES" / "VAD TrueStep GÖR".

## 1.5.0
- Swedish manual: ENG | SWE after the word MANUAL on the back page. Only the manual text changes language; the rest of the window stays English. The choice is remembered (Documents\TrueStep Presets\language.txt).
- Back page header now says VST2 / VST3; manual note mentions the VST3 SDK (MIT).

## 1.4.0
- Square BANK button next to RANDOMIZE: first press shows Random bank 1 in the PRESET box and the arrows browse its patterns; next press = next bank; after the last bank the button closes and the normal presets return.
- RANDOMIZE button wider, same text size as SAVE/LOAD.

## 1.3.0
- S/H (sample and hold) knob next to HUMANIZE: a new random cutoff for every step, held for the whole step. Tied to the song position, so export sounds like playback.
- New layout of the DRY/WET (wider), OUTPUT (narrower) and the bottom row, following the new GUI image.

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
