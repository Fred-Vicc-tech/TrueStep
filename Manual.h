// Manual.h - texten på TrueSteps baksida (klicka på loggan).
// Engelska, som resten av gränssnittet. Varje rad är ett stycke:
//   kManHead = rubrik, kManBody = brödtext, kManNote = källa/notis (mindre).
// Texten är UTF-8 (\xC2\xB7 = mittpunkt, \xC2\xB0 = gradtecken).
#pragma once

enum { kManHead = 0, kManBody = 1, kManNote = 2 };

struct ManualPara
{
    int kind;
    const char* text;
};

static const ManualPara kManual[] = {
    { kManHead, "WHAT TRUESTEP DOES" },
    { kManBody, "TrueStep is a rhythmic step filter and gate. It follows the tempo and song position of your host and divides every bar "
                "into 16 steps (16th notes). Each step has its own gate length, highpass and lowpass filter, resonance and stereo "
                "position, and any step can be split into two 32nd notes that have settings of their own. Drag the edges of the bars in "
                "the STEP GRID to shape the sound of every step." },
    { kManBody, "When the host is not playing, the sound passes through unchanged (OUTPUT still applies). Every setting can be automated "
                "and is saved with your project." },

    { kManHead, "THE STEP GRID" },
    { kManBody, "Each column is one step of the bar, from 1 to 16. The bar in a column shows what is let through: its width is the gate "
                "length, its top edge is the lowpass cutoff and its bottom edge is the highpass cutoff. Everything between the edges "
                "passes, the rest is filtered away. The vertical scale is 20 Hz to 20 kHz (logarithmic) with lines at 100 Hz, 1 kHz and "
                "10 kHz. The step that is playing right now is highlighted in orange." },
    { kManBody, "Drag the right edge for LENGTH, the top edge for LOWPASS, the bottom edge for HIGHPASS, and the middle of the bar to move "
                "both filters together. Click above a bar to move the lowpass edge there, below it to move the highpass edge, or to the "
                "right of it to set the length. Double-click a column to reset it." },

    { kManHead, "GATE LENGTH" },
    { kManBody, "LENGTH is how much of the step the gate is open: 0 % is closed, 100 % is open for the whole step. The gate opens and "
                "closes in about 2 ms, so it does not click. Several steps at 100 % in a row give a continuous sound." },

    { kManHead, "LOWPASS AND HIGHPASS" },
    { kManBody, "Both filters are 24 dB per octave (4th order Butterworth). With the lowpass edge at the top and the highpass edge at the "
                "bottom the filters are switched off and the sound is not filtered at all. The lowpass edge cannot go below the highpass "
                "edge. Changes between steps glide in about 5 ms." },

    { kManHead, "RESONANCE (RES)" },
    { kManBody, "Nine levels, 0 to 8. Resonance emphasizes the frequencies around the lowpass cutoff, like the squelch of a classic "
                "acid bass synthesizer. Level 0 is flat, level 8 gives a boost of about 15 dB at the cutoff. It is heard when the lowpass "
                "edge is moved down from the top." },

    { kManHead, "1/32 SPLIT" },
    { kManBody, "The 1/32 button under a step splits it into two halves (two 32nd notes). Each half gets its own bar, filter, resonance and "
                "pan. When you split a step, the second half starts as a copy of the first. Press the button again to join the halves; "
                "the settings of the second half are kept in case you split the step again." },

    { kManHead, "PAN" },
    { kManBody, "Nine positions per step: 50L, 36L, 22L, 10L, C, 10R, 22R, 36R and 50R. C leaves the stereo image as it is." },
    { kManBody, "TrueStep uses true stereo constant power panning. The position p (-1 to +1 = step / 50) is applied to the left and the right "
                "input channel separately. For p <= 0 the left channel is placed at -1 and the right channel at 1 + 2p; for p > 0 the right "
                "channel is placed at +1 and the left channel at -1 + 2p. Each channel then goes to master left and right with the gains "
                "cos((pan + 1) x pi / 4) and sin((pan + 1) x pi / 4). The whole stereo image is moved instead of just balanced." },
    { kManNote, "Note: at 50L and 50R both input channels end up on one side and add together, so a mono sound is up to 6 dB louder there. "
                "Use OUTPUT to compensate if you need to. The dry part of DRY/WET is not panned." },

    { kManHead, "DRY/WET AND OUTPUT" },
    { kManBody, "DRY/WET mixes the original sound (0 %) with the processed sound (100 %). OUTPUT sets the level after the effect, from -24 to "
                "+12 dB (0 dB = unchanged). OVER 0 dB lights up red if the output goes over 0 dBFS. The dot in the top right corner "
                "lights up when sound is passing." },
    { kManBody, "Knobs: drag up/down, hold Shift for fine adjustment, use the mouse wheel for small steps and double-click to return to the "
                "default value." },

    { kManHead, "HUMANIZE" },
    { kManBody, "The small knob next to DRY/WET moves the start of every step a little earlier or later than the grid, up to 25 ms, "
                "for a looser, more human feel. 0 ms is off. The shifts are not random jumps: they follow pink (1/f) noise, so "
                "they drift smoothly from step to step, the way the timing of a real musician does (Hennig et al., 2011, "
                "\"The nature and perception of fluctuations in human musical rhythms\"). Most steps stay close to the grid "
                "and only a few move far from it. The pattern is fixed to the position in the song, so playback and export "
                "sound the same every time. HUMANIZE adds no delay. It moves the gate and the filters in time, not the sound." },

    { kManHead, "PRESETS" },
    { kManBody, "TrueStep comes with 12 factory presets, which also appear in the preset list of your host. The arrows step through the "
                "factory presets and then your own. SAVE stores your own preset (.tsp) in the folder Documents\\TrueStep Presets, which is "
                "created automatically. LOAD opens a preset file." },

    { kManHead, "SYNC AND TEMPO" },
    { kManBody, "TrueStep needs tempo and position from the host, so the steps lock to the song, to loops and to jumps in the timeline. "
                "One bar of 4/4 is 16 steps. Pattern length is always one bar; use the host's tempo to change the speed." },

    { kManHead, "TIPS" },
    { kManBody, "Try 50 % length on all steps for a classic trance gate. Move the lowpass edge down and add resonance on every second "
                "step for a rhythmic wah. Split the last step of a bar for a quick roll. Put TruePan after TrueStep for even more movement "
                "in the stereo field." },

    { kManNote, "TrueStep is built with its own VST2 interface (no third-party SDK) and runs on Windows 7 and later." },
};

static const int kManualCount = (int)(sizeof(kManual) / sizeof(kManual[0]));
