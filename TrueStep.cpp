// TrueStep.cpp - VST2-versionen: kopplar kärnan (TrueStepCore.h) och ljudmotorn
// (TrueStepDSP.h) till värdprogrammet via VST2-gränssnittet. Fönstret finns i
// TrueStepEditor.cpp.
//
// Trådar:
//   - Ljudtråden anropar processReplacing. Där sker ingen allokering, inga
//     lås och inget ritande.
//   - Gränssnittet (fönstret) läser bara atomiska värden som ljudtråden
//     skriver, och skriver parametrar via atomiska värden.
#include "vst2.h"
#include "TrueStepCore.h"

#include <cstdio>
#include <cstring>
#include <new>

#if defined(_WIN32)
  #define TRUESTEP_HAS_EDITOR 1
#else
  #define TRUESTEP_HAS_EDITOR 0
#endif

using namespace truestep;

namespace {

const VstInt32 kUniqueId = ('T' << 24) | ('r' << 16) | ('S' << 8) | 't';
const VstInt32 kVersion = 1000;           // TrueStep 1.0.0

struct Plugin : Core
{
    AEffect effect;
    audioMasterCallback host = nullptr;
    ERect rect;

    void hostAutomate(int index, float value) override
    {
        if (host) host(&effect, audioMasterAutomate, index, 0, nullptr, value);
    }
    void hostBeginEdit(int index) override { if (host) host(&effect, audioMasterBeginEdit, index, 0, nullptr, 0.0f); }
    void hostEndEdit(int index) override { if (host) host(&effect, audioMasterEndEdit, index, 0, nullptr, 0.0f); }
    void hostUpdateDisplay() override { if (host) host(&effect, audioMasterUpdateDisplay, 0, 0, nullptr, 0.0f); }
};

Plugin* fromEffect(AEffect* e) { return e ? static_cast<Plugin*>(e->object) : nullptr; }

// ---------------------------------------------------------------------------
// Ljudbehandling
// ---------------------------------------------------------------------------

void processBlock(Plugin* p, float** inputs, float** outputs, VstInt32 n, bool accumulate)
{
    if (!p || !outputs || !outputs[0] || n <= 0) return;

    // Tempo och position från värden (stegen följer takten när den spelar)
    TransportInfo t;
    if (p->host) {
        const VstIntPtr r = p->host(&p->effect, audioMasterGetTime, 0, kVstTempoValid | kVstPpqPosValid, nullptr, 0.0f);
        const VstTimeInfo* ti = reinterpret_cast<const VstTimeInfo*>(r);
        if (ti) {
            t.tempoValid = (ti->flags & kVstTempoValid) != 0;
            t.tempo = ti->tempo;
            t.playing = (ti->flags & kVstTransportPlaying) != 0;
            t.ppqValid = (ti->flags & kVstPpqPosValid) != 0;
            t.ppq = ti->ppqPos;
        }
    }
    const float* inL = inputs ? inputs[0] : nullptr;
    const float* inR = inputs ? inputs[1] : nullptr;
    p->processAudio(inL, inR, outputs[0], outputs[1], n, accumulate, t);
}

void VSTCALLBACK_processReplacing(AEffect* e, float** in, float** out, VstInt32 n)
{
    processBlock(fromEffect(e), in, out, n, false);
}

void VSTCALLBACK_process(AEffect* e, float** in, float** out, VstInt32 n)
{
    processBlock(fromEffect(e), in, out, n, true);
}

// ---------------------------------------------------------------------------
// Parametrar
// ---------------------------------------------------------------------------

void setParameter(AEffect* e, VstInt32 index, float value)
{
    if (Plugin* p = fromEffect(e)) p->setParam(index, value);
}

float getParameter(AEffect* e, VstInt32 index)
{
    Plugin* p = fromEffect(e);
    if (!p || index < 0 || index >= kNumParams) return 0.0f;
    return p->params[index].load();
}

// ---------------------------------------------------------------------------
// Dispatcher: värdens kommandon till pluginet
// ---------------------------------------------------------------------------

VstIntPtr dispatcher(AEffect* e, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
    Plugin* p = fromEffect(e);
    if (!p) return 0;

    switch (opcode) {
    case effOpen: return 0;

    case effClose:
#if TRUESTEP_HAS_EDITOR
        editorClose(p);
#endif
        delete p;
        return 0;

    case effSetSampleRate:
        p->engine.prepare(opt);
        return 0;

    case effSetBlockSize: return 0;

    case effMainsChanged:
        if (value) p->engine.reset();
        return 0;

    case effGetParamName:
        if (ptr && index >= 0 && index < kNumParams) copyString((char*)ptr, paramName(index), 8);
        return 0;
    case effGetParamLabel:
        if (ptr && index >= 0 && index < kNumParams) copyString((char*)ptr, paramLabel(index), 8);
        return 0;
    case effGetParamDisplay:
        if (ptr && index >= 0 && index < kNumParams) {
            char buf[32];
            paramDisplayText(index, p->params[index].load(), buf, sizeof buf);
            copyString((char*)ptr, buf, 8);
        }
        return 0;
    case effCanBeAutomated:
        return (index >= 0 && index < kNumParams) ? 1 : 0;

    // Fabrikspresets visas som "program" i Lives presetlista.
    case effGetProgram: return p->program < 0 ? 0 : p->program;
    case effSetProgram:
        // Live ber om program 0 strax efter SAVE/LOAD: behåll då det sparade ljudet
        if (p->program < 0 && value == 0 && (p->editorPresetRecent() || p->stateLoadRecent())) return 0;
        // Projekt som öppnas: Live ber om samma program som sparades. Det sparade ljudet (med dina ändringar
        // av en fabrikspreset) är det som gäller, så programmet ska inte laddas om.
        if (p->stateLoadRecent() && p->program >= 0 && value == p->program) return 0;
        p->loadFactoryPreset((int)value, false);
        return 0;
    case effGetProgramName:
        if (ptr) copyString((char*)ptr, p->presetName, 24);
        return 0;
    case effSetProgramName:
        if (ptr) copyString(p->presetName, cleanPresetName((const char*)ptr).c_str(), sizeof p->presetName);
        return 0;
    case effGetProgramNameIndexed:
        if (ptr && index >= 0 && index < kNumFactoryPresets) { copyString((char*)ptr, factoryPresetName(index), 24); return 1; }
        return 0;

    case effGetChunk: {
        if (!ptr) return 0;
        *static_cast<void**>(ptr) = (void*)&p->saveState();
        return (VstIntPtr)sizeof(StateChunk);
    }
    case effSetChunk:
        if (!ptr || value <= 0) return 0;
        if (!p->loadState(ptr, (size_t)value)) return 0;
        p->markStateLoaded();
        return 1;

    case effGetPlugCategory: return kPlugCategEffect;
    case effGetEffectName:
        if (ptr) copyString((char*)ptr, "TrueStep", 32);
        return 1;
    case effGetProductString:
        if (ptr) copyString((char*)ptr, "TrueStep Rhythmic Step Filter Gate", 64);
        return 1;
    case effGetVendorString:
        if (ptr) copyString((char*)ptr, "TrueStep", 64);
        return 1;
    case effGetVendorVersion: return kVersion;
    case effGetVstVersion: return 2400;
    case effGetTailSize: return 0;

    case effCanDo: {
        const char* s = static_cast<const char*>(ptr);
        if (!s) return 0;
        if (!std::strcmp(s, "receiveVstTimeInfo")) return 1;
        return 0;
    }

#if TRUESTEP_HAS_EDITOR
    case effEditGetRect:
        if (ptr) *static_cast<ERect**>(ptr) = &p->rect;
        return 1;
    case effEditOpen:
        editorOpen(p, ptr);
        return 1;
    case effEditClose:
        editorClose(p);
        return 1;
    case effEditIdle: return 0;
#endif

    default: return 0;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Ingång: värden anropar denna funktion för att skapa pluginet
// ---------------------------------------------------------------------------
VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback host)
{
    if (!host) return nullptr;
    if (host(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f) == 0) return nullptr;

    Plugin* p = new (std::nothrow) Plugin();
    if (!p) return nullptr;
    p->host = host;

    AEffect& e = p->effect;
    std::memset(&e, 0, sizeof(AEffect));
    e.magic = kEffectMagic;
    e.dispatcher = dispatcher;
    e.process = VSTCALLBACK_process;
    e.processReplacing = VSTCALLBACK_processReplacing;
    e.setParameter = setParameter;
    e.getParameter = getParameter;
    e.numPrograms = kNumFactoryPresets;
    e.numParams = kNumParams;
    e.numInputs = 2;
    e.numOutputs = 2;
    e.flags = effFlagsCanReplacing | effFlagsProgramChunks
#if TRUESTEP_HAS_EDITOR
            | effFlagsHasEditor
#endif
            ;
    e.object = p;
    e.uniqueID = kUniqueId;
    e.version = kVersion;

    p->rect.top = 0; p->rect.left = 0;
    p->rect.bottom = (short)kEditorHeight; p->rect.right = (short)kEditorWidth;

    return &e;   // motorn är redan förberedd för 44,1 kHz (Live skickar rätt frekvens före start)
}

