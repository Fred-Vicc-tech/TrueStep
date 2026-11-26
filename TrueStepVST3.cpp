// TrueStepVST3.cpp - VST3-versionen av TrueStep. Samma ljudmotor (TrueStepDSP.h),
// samma kärna (TrueStepCore.h) och samma fönster (TrueStepEditor.cpp) som
// VST2-versionen; bara kopplingen till värdprogrammet skiljer sig.
//
// Byggs med Steinbergs VST 3 SDK (MIT-licens, se LICENSE-VST3-SDK.txt).
// Projekt sparas i samma format som VST2-versionen.
#include "TrueStepCore.h"

#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/common/pluginview.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vsttypes.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {

// Egna, slumpade id-nummer för TrueStep VST3 (får aldrig ändras)
static const FUID kTrueStepUid(0x7C1E3A95, 0x4D2B8F60, 0xB9E47A13, 0x2A6C5D88);

// Långa parameternamn (VST3 visar fler tecken än VST2), t.ex. "Step 03b Highpass"
void longName(int index, char* out, int n)
{
    using namespace truestep;
    if (index == kParamMix) { std::snprintf(out, (size_t)n, "Dry/Wet"); return; }
    if (index == kParamOutput) { std::snprintf(out, (size_t)n, "Output"); return; }
    if (index == kParamHumanize) { std::snprintf(out, (size_t)n, "Humanize"); return; }
    static const char* const what[11] = { "Length", "Highpass", "Lowpass", "Split 1/32", "Length", "Highpass", "Lowpass",
                                          "Pan", "Resonance", "Pan", "Resonance" };
    const int k = (index - 1) / kNumSteps, s = (index - 1) % kNumSteps;
    // 0-2 = halva 1, 3 = delning, 4-6 = halva 2, 7-8 = halva 1, 9-10 = halva 2
    const bool second = (k >= 4 && k <= 6) || k >= 9;
    const int num = s + 1;
    std::snprintf(out, (size_t)n, "Step %02d%s %s", num, k == 3 ? "" : (second ? "b" : "a"), what[k < 11 ? k : 0]);
}

// Antal steg för valparametrar (0 = steglös)
int32 stepCountFor(int index)
{
    using namespace truestep;
    if (isSplitParam(index)) return 1;
    if (isPanParam(index) || isResParam(index)) return kNumSlots - 1;
    return 0;
}

void toTChar(const char* in, TChar* out, int maxLen)
{
    int i = 0;
    for (; in && in[i] && i < maxLen - 1; ++i) out[i] = (TChar)(unsigned char)in[i];
    out[i] = 0;
}

void fromTChar(const TChar* in, char* out, int maxLen)
{
    int i = 0;
    for (; in && in[i] && i < maxLen - 1; ++i) out[i] = (in[i] < 128) ? (char)in[i] : '?';
    out[i] = 0;
}

// Parameter som visar sitt värde med samma text som VST2-versionen
class TSParameter : public Parameter
{
public:
    TSParameter(int index)
    {
        char ln[48];
        longName(index, ln, sizeof ln);
        toTChar(ln, info.title, 128);
        toTChar(truestep::paramName(index), info.shortTitle, 128);
        toTChar(truestep::paramLabel(index), info.units, 128);
        info.id = (ParamID)index;
        info.stepCount = stepCountFor(index);
        info.defaultNormalizedValue = truestep::defaultParam(index);
        info.unitId = kRootUnitId;
        info.flags = ParameterInfo::kCanAutomate | (truestep::isSplitParam(index) ? ParameterInfo::kIsList : 0);
        valueNormalized = info.defaultNormalizedValue;
    }

    void toString(ParamValue normValue, String128 string) const override
    {
        char buf[64];
        truestep::paramDisplayText((int)info.id, (float)normValue, buf, sizeof buf);
        toTChar(buf, string, 128);
    }

    bool fromString(const TChar*, ParamValue&) const override { return false; }   // värden skriver in värden som reglage
};

class TrueStepView;

// Bypass (VST3-standard): värden visar den som sin egen bypass-knapp.
const ParamID kBypassId = 1000;

// ---------------------------------------------------------------------------
// Effekten: ljud och parametrar i samma objekt (SingleComponentEffect)
// ---------------------------------------------------------------------------
class TrueStepVst3 : public SingleComponentEffect, public truestep::Core
{
public:
    static FUnknown* createInstance(void*) { return (IAudioProcessor*)new TrueStepVst3; }

    tresult PLUGIN_API initialize(FUnknown* context) override
    {
        const tresult r = SingleComponentEffect::initialize(context);
        if (r != kResultOk) return r;
        addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
        addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
        for (int i = 0; i < truestep::kNumParams; ++i) parameters.addParameter(new TSParameter(i));
        parameters.addParameter(STR16("Bypass"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kBypassId);
        // Tempo, position och transport behövs för stegen
        processContextRequirements.needTempo().needProjectTimeMusic().needTransportState();
        return kResultOk;
    }

    tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                          SpeakerArrangement* outputs, int32 numOuts) override
    {
        // Bara stereo in och stereo ut
        if (numIns == 1 && numOuts == 1 && inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo)
            return SingleComponentEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
        return kResultFalse;
    }

    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override
    {
        return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
    }

    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) override
    {
        engine.prepare(setup.sampleRate);
        const int32 maxN = setup.maxSamplesPerBlock > 0 ? setup.maxSamplesPerBlock : 4096;
        dryL_.assign((size_t)maxN, 0.0f);
        dryR_.assign((size_t)maxN, 0.0f);
        bypassCoef_ = 1.0f - (float)std::exp(-1.0 / (0.010 * (setup.sampleRate > 0.0 ? setup.sampleRate : 44100.0)));
        return SingleComponentEffect::setupProcessing(setup);
    }

    tresult PLUGIN_API setActive(TBool state) override
    {
        if (state) engine.reset();
        return SingleComponentEffect::setActive(state);
    }

    tresult PLUGIN_API process(ProcessData& data) override
    {
        // Parameterändringar från värden (automation): senaste värdet i blocket
        if (IParameterChanges* changes = data.inputParameterChanges) {
            const int32 count = changes->getParameterCount();
            for (int32 i = 0; i < count; ++i) {
                IParamValueQueue* q = changes->getParameterData(i);
                if (!q) continue;
                const int32 points = q->getPointCount();
                int32 offset = 0;
                ParamValue value = 0.0;
                if (points > 0 && q->getPoint(points - 1, offset, value) == kResultTrue) {
                    if (q->getParameterId() == kBypassId) {
                        bypass_ = value >= 0.5;
                        // Värden förväntar sig att bypass-läget syns direkt i parametern
                        if (Parameter* bp = parameters.getParameter(kBypassId)) bp->setNormalized(value);
                    }
                    else setParam((int)q->getParameterId(), (float)value);
                }
            }
        }
        if (data.numSamples <= 0 || data.numOutputs < 1 || !data.outputs) return kResultOk;   // bara parametrar
        AudioBusBuffers& out = data.outputs[0];
        if (out.numChannels < 1 || !out.channelBuffers32) return kResultOk;

        truestep::TransportInfo t;
        if (const ProcessContext* c = data.processContext) {
            t.tempoValid = (c->state & ProcessContext::kTempoValid) != 0;
            t.tempo = c->tempo;
            t.playing = (c->state & ProcessContext::kPlaying) != 0;
            t.ppqValid = (c->state & ProcessContext::kProjectTimeMusicValid) != 0;
            t.ppq = c->projectTimeMusic;
        }

        const float* inL = nullptr;
        const float* inR = nullptr;
        if (data.numInputs > 0 && data.inputs && data.inputs[0].numChannels > 0 && data.inputs[0].channelBuffers32) {
            inL = data.inputs[0].channelBuffers32[0];
            inR = data.inputs[0].numChannels > 1 ? data.inputs[0].channelBuffers32[1] : inL;
        }
        float* outL = out.channelBuffers32[0];
        float* outR = out.numChannels > 1 ? out.channelBuffers32[1] : nullptr;
        const int32 n = data.numSamples;
        // Bypass: spara originalet (in och ut kan vara samma minne) och tona över på 10 ms
        const bool fading = bypass_ || bypassMix_ > 0.0f;
        const bool canFade = fading && (size_t)n <= dryL_.size();
        if (canFade) {
            for (int32 i = 0; i < n; ++i) {
                dryL_[(size_t)i] = inL ? inL[i] : 0.0f;
                dryR_[(size_t)i] = inR ? inR[i] : 0.0f;
            }
        }
        processAudio(inL, inR, outL, outR, n, false, t);
        if (canFade) {
            const float target = bypass_ ? 1.0f : 0.0f;
            for (int32 i = 0; i < n; ++i) {
                bypassMix_ += (target - bypassMix_) * bypassCoef_;
                if (std::fabs(bypassMix_ - target) < 1e-5f) bypassMix_ = target;
                outL[i] += (dryL_[(size_t)i] - outL[i]) * bypassMix_;
                if (outR) outR[i] += (dryR_[(size_t)i] - outR[i]) * bypassMix_;
            }
        }
        out.silenceFlags = 0;
        return kResultOk;
    }

    // --- Sparat tillstånd (samma format som VST2) ---
    tresult PLUGIN_API getState(IBStream* state) override
    {
        if (!state) return kInvalidArgument;
        const truestep::StateChunk& c = saveState();
        int32 written = 0;
        if (state->write((void*)&c, (int32)sizeof c, &written) != kResultOk || written != (int32)sizeof c) return kResultFalse;
        // Efter chunken: bypass-läget (bara VST3; VST2 läser inte så här långt)
        unsigned char tail[8] = { 'T', 'S', 'b', 'y', 0, 0, 0, 0 };
        tail[4] = bypass_ ? 1 : 0;
        return state->write(tail, 8, &written) == kResultOk && written == 8 ? kResultOk : kResultFalse;
    }

    tresult PLUGIN_API setState(IBStream* state) override
    {
        if (!state) return kInvalidArgument;
        std::vector<unsigned char> data;
        unsigned char buf[512];
        for (;;) {
            int32 got = 0;
            if (state->read(buf, (int32)sizeof buf, &got) != kResultOk || got <= 0) break;
            data.insert(data.end(), buf, buf + got);
            if (data.size() > 65536) break;
        }
        if (!loadState(data.data(), data.size())) return kResultFalse;
        // Bypass-läget efter chunken (finns bara i VST3-projekt)
        bool bp = false;
        const size_t tailAt = sizeof(truestep::StateChunk);
        if (data.size() >= tailAt + 8 && std::memcmp(&data[tailAt], "TSby", 4) == 0) bp = data[tailAt + 4] != 0;
        bypass_ = bp;
        bypassMix_ = bp ? 1.0f : 0.0f;
        SingleComponentEffect::setParamNormalized(kBypassId, bp ? 1.0 : 0.0);
        // Visa de inlästa värdena för värden
        for (int i = 0; i < truestep::kNumParams; ++i) SingleComponentEffect::setParamNormalized((ParamID)i, params[i].load());
        return kResultOk;
    }

    // --- Parametrar från värdens gränssnitt ---
    tresult PLUGIN_API setParamNormalized(ParamID id, ParamValue value) override
    {
        const tresult r = SingleComponentEffect::setParamNormalized(id, value);
        if (id == kBypassId) bypass_ = value >= 0.5;
        else setParam((int)id, (float)value);
        return r;
    }

    // --- Fönstret ---
    IPlugView* PLUGIN_API createView(FIDString name) override;

    // --- Kopplingar från kärnan (fönstret ändrar parametrar) ---
    void hostAutomate(int index, float value) override
    {
        SingleComponentEffect::setParamNormalized((ParamID)index, value);
        EditController::performEdit((ParamID)index, value);
    }
    void hostBeginEdit(int index) override { EditController::beginEdit((ParamID)index); }
    void hostEndEdit(int index) override { EditController::endEdit((ParamID)index); }

private:
    bool bypass_ = false;
    float bypassMix_ = 0.0f, bypassCoef_ = 0.003f;
    std::vector<float> dryL_, dryR_;
};

// ---------------------------------------------------------------------------
// Fönstret: samma GDI+-fönster som i VST2-versionen
// ---------------------------------------------------------------------------
class TrueStepView : public CPluginView
{
public:
    explicit TrueStepView(truestep::Core* core) : CPluginView(nullptr), core_(core)
    {
        rect = ViewRect(0, 0, truestep::kEditorWidth, truestep::kEditorHeight);
    }

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override
    {
#if defined(_WIN32)
        return (type && std::strcmp(type, kPlatformTypeHWND) == 0) ? kResultTrue : kResultFalse;
#else
        (void)type;
        return kResultFalse;
#endif
    }

    tresult PLUGIN_API attached(void* parent, FIDString type) override
    {
        if (isPlatformTypeSupported(type) != kResultTrue) return kResultFalse;
        const tresult r = CPluginView::attached(parent, type);
#if defined(_WIN32)
        truestep::editorOpen(core_, parent);
#endif
        return r;
    }

    tresult PLUGIN_API removed() override
    {
#if defined(_WIN32)
        truestep::editorClose(core_);
#endif
        return CPluginView::removed();
    }

    tresult PLUGIN_API canResize() override { return kResultFalse; }

private:
    truestep::Core* core_;
};

IPlugView* PLUGIN_API TrueStepVst3::createView(FIDString name)
{
#if defined(_WIN32)
    if (name && std::strcmp(name, ViewType::kEditor) == 0) return new TrueStepView(this);
#else
    (void)name;
#endif
    return nullptr;
}

} // namespace

// ---------------------------------------------------------------------------
// Fabriken: värden hittar TrueStep här
// ---------------------------------------------------------------------------
BEGIN_FACTORY_DEF("2F Productions", "https://2f.se", "mailto:info@2f.se")

    DEF_CLASS2(INLINE_UID_FROM_FUID(kTrueStepUid),
               PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               "TrueStep",
               0,                       // SingleComponentEffect: ljud och parametrar i samma objekt
               "Fx|Filter",
               "1.0.0",
               kVstVersionString,
               TrueStepVst3::createInstance)

END_FACTORY
