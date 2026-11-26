# TrueStep

Rhythmic step filter and gate (VST2 and VST3, 64-bit, Windows 7 and later).
16 steps per bar; per step: gate length, highpass/lowpass (drag the bar), resonance, pan, and a 1/32 split.

Built by Fredrik Frigren, 2F Productions (info@2f.se).

## Build
Push to GitHub: the workflow in `.github/workflows/build.yml` builds `TrueStep.dll` (VST2) and `TrueStep.vst3` (VST3)
(Visual Studio 2022, toolset v143, static runtime) and runs the tests.
Download `TrueStep-win64` under **Artifacts** on the run summary page.

## Install
VST2: copy `TrueStep.dll` to your VST plug-in folder.
VST3: copy the `TrueStep.vst3` folder to `C:\Program Files\Common Files\VST3`.
Then rescan in your host.

## Files
- `TrueStepDSP.h` - audio engine, parameters, presets (platform independent)
- `TrueStepCore.h` - parameters shared with the host, state, processing
- `TrueStepGui.h` - the whole GUI (layout, drawing commands, mouse logic, manual)
- `TrueStepEditor.cpp` - Windows window and GDI+ drawing
- `TrueStepVST3.cpp` - VST3 wrapper (Steinberg VST 3 SDK, MIT license, see LICENSE-VST3-SDK.txt)
- `TrueStep.cpp` - VST2 wrapper; `vst2.h` - own minimal VST2 interface
- `TrueStepTests.cpp`, `HostTests.cpp` - automatic tests
