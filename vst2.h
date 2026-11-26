// vst2.h - minimal egen beskrivning av VST2-gränssnittet (ABI) för TrueStep.
// Samma egna header som i TrueSub och TruePan (stereo effekt, tidsinfo från värden).
// Innehåller ingen kod eller text från Steinbergs VST2 SDK.
#pragma once
#include <stdint.h>

#if defined(_WIN32)
  #define VST_EXPORT extern "C" __declspec(dllexport)
#else
  #define VST_EXPORT extern "C" __attribute__((visibility("default")))
#endif

typedef int32_t  VstInt32;
typedef intptr_t VstIntPtr;

#pragma pack(push, 8)

struct AEffect;
typedef VstIntPtr (*audioMasterCallback)(AEffect* effect, VstInt32 opcode, VstInt32 index,
                                         VstIntPtr value, void* ptr, float opt);
typedef VstIntPtr (*AEffectDispatcherProc)(AEffect* effect, VstInt32 opcode, VstInt32 index,
                                           VstIntPtr value, void* ptr, float opt);
typedef void  (*AEffectProcessProc)(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames);
typedef void  (*AEffectSetParameterProc)(AEffect* effect, VstInt32 index, float parameter);
typedef float (*AEffectGetParameterProc)(AEffect* effect, VstInt32 index);

struct AEffect
{
    VstInt32 magic;
    AEffectDispatcherProc dispatcher;
    AEffectProcessProc process;
    AEffectSetParameterProc setParameter;
    AEffectGetParameterProc getParameter;
    VstInt32 numPrograms;
    VstInt32 numParams;
    VstInt32 numInputs;
    VstInt32 numOutputs;
    VstInt32 flags;
    VstIntPtr resvd1;
    VstIntPtr resvd2;
    VstInt32 initialDelay;
    VstInt32 realQualities;
    VstInt32 offQualities;
    float ioRatio;
    void* object;
    void* user;
    VstInt32 uniqueID;
    VstInt32 version;
    AEffectProcessProc processReplacing;
    void (*processDoubleReplacing)(AEffect*, double**, double**, VstInt32);
    char future[56];
};

struct VstTimeInfo
{
    double samplePos;
    double sampleRate;
    double nanoSeconds;
    double ppqPos;
    double tempo;
    double barStartPos;
    double cycleStartPos;
    double cycleEndPos;
    VstInt32 timeSigNumerator;
    VstInt32 timeSigDenominator;
    VstInt32 smpteOffset;
    VstInt32 smpteFrameRate;
    VstInt32 samplesToNextClock;
    VstInt32 flags;
};

#pragma pack(pop)

struct ERect
{
    int16_t top, left, bottom, right;
};

enum
{
    kEffectMagic = 0x56737450,   // 'VstP'

    // flaggor
    effFlagsHasEditor    = 1 << 0,
    effFlagsCanReplacing = 1 << 4,
    effFlagsProgramChunks = 1 << 5,

    // dispatcher-kommandon
    effOpen = 0, effClose = 1, effSetProgram = 2, effGetProgram = 3,
    effSetProgramName = 4, effGetProgramName = 5,
    effGetParamLabel = 6, effGetParamDisplay = 7, effGetParamName = 8,
    effSetSampleRate = 10, effSetBlockSize = 11, effMainsChanged = 12,
    effEditGetRect = 13, effEditOpen = 14, effEditClose = 15, effEditIdle = 19,
    effGetChunk = 23, effSetChunk = 24,
    effGetProgramNameIndexed = 29,
    effCanBeAutomated = 26,
    effGetPlugCategory = 35,
    effGetEffectName = 45, effGetVendorString = 47, effGetProductString = 48,
    effGetVendorVersion = 49, effCanDo = 51, effGetTailSize = 52,
    effGetVstVersion = 58,

    // kommandon plugin -> värd
    audioMasterAutomate = 0,
    audioMasterUpdateDisplay = 42,
    audioMasterBeginEdit = 43,
    audioMasterEndEdit = 44,
    audioMasterVersion = 1,
    audioMasterGetTime = 7,

    // tidsinfo-flaggor
    kVstTransportPlaying = 1 << 1,
    kVstPpqPosValid      = 1 << 9,
    kVstTempoValid       = 1 << 10,

    kPlugCategEffect = 1
};
