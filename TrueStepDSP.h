// TrueStepDSP.h - parametrar, texter, fabrikspresets, presetfiler och ljudmotorn
// för TrueStep. Inga Windows-delar: allt här testas på alla datorer.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace truestep {

// ---------------------------------------------------------------------------
// Parametrar. Alla är 0..1 mot värdprogrammet.
//   steg 1..16, första halvan av steget (16-del) och andra halvan (32-del)
// ---------------------------------------------------------------------------
const int kNumSteps = 16;

enum
{
    kParamMix       = 0,                     // dry/wet
    kParamLenBase   = 1,                     // 1..16     längd (gate), första halvan
    kParamHPBase    = 1 + kNumSteps,         // 17..32    highpass, första halvan
    kParamLPBase    = 1 + 2 * kNumSteps,     // 33..48    lowpass, första halvan
    kParamSplitBase = 1 + 3 * kNumSteps,     // 49..64    dela steget i två 32-delar (av/på)
    kParamLen2Base  = 1 + 4 * kNumSteps,     // 65..80    längd, andra halvan
    kParamHP2Base   = 1 + 5 * kNumSteps,     // 81..96    highpass, andra halvan
    kParamLP2Base   = 1 + 6 * kNumSteps,     // 97..112   lowpass, andra halvan
    kParamPanBase   = 1 + 7 * kNumSteps,     // 113..128  panorering, första halvan (9 lägen)
    kParamResBase   = 1 + 8 * kNumSteps,     // 129..144  resonans, första halvan (9 lägen)
    kParamPan2Base  = 1 + 9 * kNumSteps,     // 145..160  panorering, andra halvan
    kParamRes2Base  = 1 + 10 * kNumSteps,    // 161..176  resonans, andra halvan
    kParamOutput    = 1 + 11 * kNumSteps,    // 177       utnivå
    kNumParams      = kParamOutput + 1       // 178
};

inline int paramLen(int step, int half) { return (half ? kParamLen2Base : kParamLenBase) + step; }
inline int paramHP (int step, int half) { return (half ? kParamHP2Base  : kParamHPBase)  + step; }
inline int paramLP (int step, int half) { return (half ? kParamLP2Base  : kParamLPBase)  + step; }
inline int paramPan(int step, int half) { return (half ? kParamPan2Base : kParamPanBase) + step; }
inline int paramRes(int step, int half) { return (half ? kParamRes2Base : kParamResBase) + step; }

// Panorering och resonans har 9 fasta lägen. Reglagevärdet 0..1 = läge / 8.
const int kNumSlots = 9;
const int kPanSteps[kNumSlots] = { -50, -36, -22, -10, 0, 10, 22, 36, 50 };

inline int slotFromValue(float v)
{
    if (!(v >= 0.0f)) return 0;
    int i = (int)(v * 8.0f + 0.5f);
    return i < 0 ? 0 : (i > 8 ? 8 : i);
}
inline float valueFromSlot(int i) { return (float)i / 8.0f; }

// Filterfrekvens: reglage 0..1 -> 20 Hz .. 20 kHz (logaritmiskt)
inline double hzFromNorm(double x) { return 20.0 * std::pow(1000.0, x); }

// OUTPUT: -24 .. +12 dB, 0 dB = 2/3
const int kOutMinDb = -24, kOutMaxDb = 12;
inline double outputDbFromNorm(double n) { return kOutMinDb + n * (kOutMaxDb - kOutMinDb); }
inline float outputNormFromDb(double db) { return (float)((db - kOutMinDb) / (double)(kOutMaxDb - kOutMinDb)); }

inline bool isSplitValue(float v) { return v >= 0.5f; }

// Standardvärden: en vanlig 16-dels-gate (längd 50 %), inget filter, mitten, ingen resonans.
inline float defaultParam(int i)
{
    if (i == kParamMix) return 1.0f;
    if (i == kParamOutput) return outputNormFromDb(0.0);
    if (i >= kParamLenBase && i < kParamHPBase) return 0.5f;
    if (i >= kParamHPBase && i < kParamLPBase) return 0.0f;
    if (i >= kParamLPBase && i < kParamSplitBase) return 1.0f;
    if (i >= kParamSplitBase && i < kParamLen2Base) return 0.0f;
    if (i >= kParamLen2Base && i < kParamHP2Base) return 0.5f;
    if (i >= kParamHP2Base && i < kParamLP2Base) return 0.0f;
    if (i >= kParamLP2Base && i < kParamPanBase) return 1.0f;
    if (i >= kParamPanBase && i < kParamResBase) return valueFromSlot(4);
    if (i >= kParamResBase && i < kParamPan2Base) return 0.0f;
    if (i >= kParamPan2Base && i < kParamRes2Base) return valueFromSlot(4);
    if (i >= kParamRes2Base && i < kParamOutput) return 0.0f;
    return 0.0f;
}

// ---------------------------------------------------------------------------
// Namn och texter (värdprogrammet visar max 8 tecken)
// ---------------------------------------------------------------------------
namespace detail {
struct NameTable
{
    char n[kNumParams][12];
    NameTable()
    {
        std::memset(n, 0, sizeof n);
        std::snprintf(n[kParamMix], 12, "Dry/Wet");
        std::snprintf(n[kParamOutput], 12, "Output");
        for (int s = 0; s < kNumSteps; ++s) {
            const char num[3] = { (char)('0' + (s + 1) / 10), (char)('0' + (s + 1) % 10), 0 };
            const char* const suffix[11] = { "Len", "HP", "LP", "Spl", "Ln2", "HP2", "LP2", "Pan", "Res", "Pn2", "Rs2" };
            for (int k = 0; k < 11; ++k) {
                char* dst = n[1 + k * kNumSteps + s];
                dst[0] = 'S'; dst[1] = num[0]; dst[2] = num[1]; dst[3] = ' ';
                int j = 4;
                for (const char* c = suffix[k]; *c && j < 11; ++c) dst[j++] = *c;
                dst[j] = 0;
            }
        }
    }
};
} // namespace detail

inline const char* paramName(int i)
{
    static const detail::NameTable table;
    return (i >= 0 && i < kNumParams) ? table.n[i] : "";
}

inline bool isFreqParam(int i)
{
    return (i >= kParamHPBase && i < kParamSplitBase) || (i >= kParamHP2Base && i < kParamPanBase);
}
inline bool isPanParam(int i)
{
    return (i >= kParamPanBase && i < kParamResBase) || (i >= kParamPan2Base && i < kParamRes2Base);
}
inline bool isResParam(int i)
{
    return (i >= kParamResBase && i < kParamPan2Base) || (i >= kParamRes2Base && i < kParamOutput);
}
inline bool isSplitParam(int i) { return i >= kParamSplitBase && i < kParamLen2Base; }

inline const char* paramLabel(int i)
{
    if (isFreqParam(i)) return "Hz";
    if (i == kParamOutput) return "dB";
    if (isPanParam(i) || isResParam(i) || isSplitParam(i)) return "";
    return "%";
}

inline void panText(int slot, char* out, size_t n)
{
    const int v = kPanSteps[slot < 0 ? 0 : (slot > 8 ? 8 : slot)];
    if (v < 0) std::snprintf(out, n, "%dL", -v);
    else if (v > 0) std::snprintf(out, n, "%dR", v);
    else std::snprintf(out, n, "C");
}

inline void hzText(float norm, char* out, size_t n)
{
    const double hz = hzFromNorm(norm);
    if (hz < 1000.0) std::snprintf(out, n, "%d Hz", (int)(hz + 0.5));
    else std::snprintf(out, n, "%.1f kHz", hz / 1000.0);
}

inline void paramDisplayText(int i, float v, char* out, size_t n)
{
    if (isFreqParam(i)) std::snprintf(out, n, "%d", (int)(hzFromNorm(v) + 0.5));
    else if (isPanParam(i)) panText(slotFromValue(v), out, n);
    else if (isResParam(i)) std::snprintf(out, n, "%d", slotFromValue(v));
    else if (isSplitParam(i)) std::snprintf(out, n, "%s", isSplitValue(v) ? "On" : "Off");
    else if (i == kParamOutput) std::snprintf(out, n, "%.1f", outputDbFromNorm(v));
    else std::snprintf(out, n, "%d", (int)(v * 100.0f + 0.5f));
}

// ---------------------------------------------------------------------------
// Fabrikspresets (syns som program i Lives presetlista)
// ---------------------------------------------------------------------------
const int kNumFactoryPresets = 12;

inline const char* factoryPresetName(int i)
{
    static const char* const names[kNumFactoryPresets] = {
        "Default Gate", "Open (Bypass)", "Trance Gate", "Offbeat Pulse",
        "Low Pass Rise", "High Pass Rise", "Telephone", "Wah Wah",
        "Acid Squelch", "Ping-Pong Pan", "Rolling 32nds", "Hi-Hat Stutter" };
    return (i >= 0 && i < kNumFactoryPresets) ? names[i] : "";
}

inline void factoryPresetValues(int idx, float* v)
{
    for (int i = 0; i < kNumParams; ++i) v[i] = defaultParam(i);
    auto clamp01 = [](float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); };
    auto setStep = [&](int s, int h, float len, float hp, float lp, int pan, int res) {
        v[paramLen(s, h)] = clamp01(len);
        v[paramHP(s, h)] = clamp01(hp);
        v[paramLP(s, h)] = clamp01(lp);
        v[paramPan(s, h)] = valueFromSlot(pan);
        v[paramRes(s, h)] = valueFromSlot(res);
    };
    static const float acid[8] = { 0.35f, 0.55f, 0.40f, 0.75f, 0.30f, 0.65f, 0.50f, 0.85f };
    for (int s = 0; s < kNumSteps; ++s) {
        switch (idx) {
        case 1:  setStep(s, 0, 1.0f, 0.0f, 1.0f, 4, 0); break;                               // Open (Bypass)
        case 2:  setStep(s, 0, (s % 4 == 0) ? 0.75f : 0.40f, 0.0f, 1.0f, 4, 0); break;       // Trance Gate
        case 3:  setStep(s, 0, (s % 4 == 2) ? 0.80f : 0.0f, 0.0f, 1.0f, 4, 0); break;        // Offbeat Pulse
        case 4:  setStep(s, 0, 1.0f, 0.0f, 0.30f + 0.70f * s / 15.0f, 4, 0); break;          // Low Pass Rise
        case 5:  setStep(s, 0, 1.0f, 0.65f * s / 15.0f, 1.0f, 4, 0); break;                  // High Pass Rise
        case 6:  setStep(s, 0, 1.0f, 0.52f, 0.78f, 4, 0); break;                              // Telephone
        case 7: {                                                                              // Wah Wah
            const int k = s % 8;
            const float tri = (k < 4 ? k : 8 - k) / 4.0f;
            setStep(s, 0, 1.0f, 0.0f, 0.35f + 0.50f * tri, 4, 5);
            break; }
        case 8:  setStep(s, 0, 0.60f, 0.0f, acid[s % 8], 4, 7); break;                       // Acid Squelch
        case 9:  setStep(s, 0, 0.80f, 0.0f, 1.0f, (s % 2 == 0) ? 1 : 7, 0); break;           // Ping-Pong Pan
        case 10:                                                                               // Rolling 32nds
            v[kParamSplitBase + s] = 1.0f;
            setStep(s, 0, 0.50f, 0.0f, 1.0f, 2, 0);
            setStep(s, 1, 0.50f, 0.0f, 1.0f, 6, 0);
            break;
        case 11:                                                                               // Hi-Hat Stutter
            setStep(s, 0, 0.25f, 0.70f, 1.0f, 4, 0);
            if (s % 4 == 3) {
                v[kParamSplitBase + s] = 1.0f;
                setStep(s, 1, 0.40f, 0.85f, 1.0f, 5, 0);
            }
            break;
        default: break;                                                                        // 0 = Default Gate
        }
    }
}

// ---------------------------------------------------------------------------
// Egna presets som textfil (Dokument\TrueStep Presets\*.tsp)
// ---------------------------------------------------------------------------
static const char* const kPresetHeader = "TrueStep preset 1";

inline std::string cleanPresetName(const std::string& in)
{
    std::string out;
    for (char c : in) {
        const unsigned char u = (unsigned char)c;
        if (u >= 32 && u < 127 && c != '=') out += c;
        else if (u >= 128) out += '_';
        if (out.size() >= 31) break;
    }
    return out.empty() ? std::string("Untitled") : out;
}

inline std::string presetToText(const std::string& name, const float* v)
{
    std::string t = kPresetHeader;
    t += "\r\nName=" + cleanPresetName(name) + "\r\n";
    char buf[64];
    for (int i = 0; i < kNumParams; ++i) {
        std::snprintf(buf, sizeof buf, "%s=%.6f\r\n", paramName(i), (double)v[i]);
        t += buf;
    }
    return t;
}

// Föreslår ett namn som inte redan används: "Wide", "Wide 2", "Wide 3" ...
template <typename ExistsFn>
inline std::string uniquePresetName(const std::string& base, ExistsFn exists)
{
    const std::string clean = cleanPresetName(base);
    if (!exists(clean)) return clean;
    std::string stem = clean;
    int n = 2;
    const size_t sp = clean.find_last_of(' ');
    if (sp != std::string::npos && sp + 1 < clean.size()) {
        bool digits = true;
        for (size_t i = sp + 1; i < clean.size(); ++i) digits = digits && clean[i] >= '0' && clean[i] <= '9';
        if (digits && clean.size() - sp - 1 <= 3) { stem = clean.substr(0, sp); n = std::atoi(clean.c_str() + sp + 1) + 1; }
    }
    for (; n < 1000; ++n) {
        const std::string candidate = stem + " " + std::to_string(n);
        if (!exists(candidate)) return candidate;
    }
    return clean;
}

// Läser en preset. Börjar från standardvärden; giltiga rader skriver över.
// Returnerar false om texten inte är en TrueStep-preset.
inline bool presetFromText(const std::string& text, float* v, std::string* name)
{
    if (text.compare(0, std::strlen(kPresetHeader), kPresetHeader) != 0) return false;
    for (int i = 0; i < kNumParams; ++i) v[i] = defaultParam(i);
    if (name) *name = "Untitled";
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find_first_of("\r\n", pos);
        if (end == std::string::npos) end = text.size();
        const std::string line = text.substr(pos, end - pos);
        pos = end + 1;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq), val = line.substr(eq + 1);
        if (key == "Name") { if (name) *name = cleanPresetName(val); continue; }
        for (int i = 0; i < kNumParams; ++i) {
            if (key != paramName(i)) continue;
            char* endp = nullptr;
            const double d = std::strtod(val.c_str(), &endp);
            if (endp != val.c_str() && d >= 0.0 && d <= 1.0) v[i] = (float)d;   // ogiltigt ignoreras
            break;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Ljudmotorn
// ---------------------------------------------------------------------------
namespace dsp {

const double kPi = 3.14159265358979323846;

struct BiquadCoef { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
struct BiquadState { double z1 = 0, z2 = 0; };

inline double runBiquad(const BiquadCoef& c, BiquadState& s, double x)
{
    const double y = c.b0 * x + s.z1;
    s.z1 = c.b1 * x - c.a1 * y + s.z2;
    s.z2 = c.b2 * x - c.a2 * y;
    return y;
}

inline BiquadCoef makeBiquad(bool highpass, double freq, double q, double sr)
{
    const double maxF = 0.45 * sr;
    if (freq > maxF) freq = maxF;
    if (freq < 10.0) freq = 10.0;
    const double w0 = 2.0 * kPi * freq / sr;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double a0 = 1.0 + alpha;
    BiquadCoef c;
    if (highpass) { c.b0 = (1.0 + cw) * 0.5; c.b1 = -(1.0 + cw); c.b2 = (1.0 + cw) * 0.5; }
    else          { c.b0 = (1.0 - cw) * 0.5; c.b1 = 1.0 - cw;    c.b2 = (1.0 - cw) * 0.5; }
    c.a1 = -2.0 * cw;
    c.a2 = 1.0 - alpha;
    c.b0 /= a0; c.b1 /= a0; c.b2 /= a0; c.a1 /= a0; c.a2 /= a0;
    return c;
}

// Två biquads i serie ger 24 dB/oktav (Butterworth, 4:e ordningen)
const double kQ1 = 0.5411961;
const double kQ2 = 1.3065630;
const double kQMax = 10.0;   // Q i lowpassens andra sektion vid högsta resonans

} // namespace dsp

class Engine
{
public:
    Engine()
    {
        for (int i = 0; i < kNumParams; ++i) p_[i] = defaultParam(i);
        prepare(44100.0);
    }

    void prepare(double sampleRate)
    {
        if (sampleRate >= 8000.0 && sampleRate <= 384000.0) sr_ = sampleRate;
        gateSmooth_ = 1.0 - std::exp(-1.0 / (0.002 * sr_));     // ca 2 ms
        cutoffSmooth_ = 1.0 - std::exp(-1.0 / (0.005 * sr_));   // ca 5 ms
        outSmooth_ = 1.0 - std::exp(-1.0 / (0.010 * sr_));      // ca 10 ms
        reset();
    }

    void reset()
    {
        gateGain_ = 1.0;
        resetFilters();
        outGain_ = targetOutGain();
        displayPos_ = -1;
    }

    void setParams(const float* v) { for (int i = 0; i < kNumParams; ++i) p_[i] = v[i]; }

    void setTransport(bool tempoValid, double tempo, bool playing, bool ppqValid, double ppq)
    {
        tempoValid_ = tempoValid; tempo_ = tempo; playing_ = playing; ppqValid_ = ppqValid; ppq_ = ppq;
    }

    int displayPos() const { return displayPos_; }     // steg * 2 + halva, -1 = står still
    float peakL() const { return peakL_; }
    float peakR() const { return peakR_; }

    // Bearbetar ett block. in och out får vara samma buffert. accumulate = lägg till i out.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, bool accumulate)
    {
        if (!outL || n <= 0) return;
        if (!outR) outR = outL;
        if (!inL) {
            if (!accumulate) { for (int i = 0; i < n; ++i) { outL[i] = 0.0f; outR[i] = 0.0f; } }
            peakL_ = peakR_ = 0.0f;
            return;
        }
        if (!inR) inR = inL;

        const bool playing = playing_ && tempoValid_ && ppqValid_ && tempo_ > 0.0 && tempo_ < 1000.0
                             && std::isfinite(ppq_);
        const double mix = clamp01(p_[kParamMix]);
        const double outTarget = targetOutGain();
        float pkL = 0.0f, pkR = 0.0f;

        if (!playing) {
            // Står uppspelningen still släpps ljudet igenom (med OUTPUT) och gaten startar om
            gateGain_ = 1.0;
            resetFilters();
            displayPos_ = -1;
            for (int i = 0; i < n; ++i) {
                outGain_ += (outTarget - outGain_) * outSmooth_;
                const double a = inL[i] * outGain_, b = inR[i] * outGain_;
                store(outL, i, (float)a, accumulate);
                store(outR, i, (float)b, accumulate);
                if (std::fabs((float)a) > pkL) pkL = std::fabs((float)a);
                if (std::fabs((float)b) > pkR) pkR = std::fabs((float)b);
            }
            peakL_ = pkL; peakR_ = pkR;
            return;
        }

        const double ppqPerSample = (tempo_ / 60.0) / sr_;
        const int kChunk = 32;
        int lastPos = 0;

        for (int start = 0; start < n; start += kChunk) {
            const int len = (n - start < kChunk) ? (n - start) : kChunk;
            for (int i = 0; i < len; ++i) {
                const int pos = start + i;
                const double pos16 = (ppq_ + (double)pos * ppqPerSample) * 4.0;   // 16-delar
                const double fl = std::floor(pos16);
                const double frac = pos16 - fl;
                const long long idx = (long long)fl;
                const int step = (int)(((idx % kNumSteps) + kNumSteps) % kNumSteps);

                // delat steg: två 32-delar med egna inställningar
                int half = 0;
                double localFrac = frac;
                if (isSplitValue(p_[kParamSplitBase + step])) {
                    half = (frac < 0.5) ? 0 : 1;
                    localFrac = (frac - 0.5 * half) * 2.0;
                }
                lastPos = step * 2 + half;

                const double openLen = clamp01(p_[paramLen(step, half)]);
                const double tgtHP = clamp01(p_[paramHP(step, half)]);
                double tgtLP = clamp01(p_[paramLP(step, half)]);
                if (tgtLP < tgtHP) tgtLP = tgtHP;                       // lowpass kan inte ligga under highpass
                const double tgtRes = (double)slotFromValue(p_[paramRes(step, half)]) / 8.0;
                const double tgtPan = (double)kPanSteps[slotFromValue(p_[paramPan(step, half)])] / 50.0;

                if (!primed_) {
                    curHP_ = tgtHP; curLP_ = tgtLP; curRes_ = tgtRes; curPan_ = tgtPan;
                    primed_ = true;
                    refreshFilterCoefs();
                }
                curHP_  += (tgtHP  - curHP_)  * cutoffSmooth_;
                curLP_  += (tgtLP  - curLP_)  * cutoffSmooth_;
                curRes_ += (tgtRes - curRes_) * cutoffSmooth_;
                curPan_ += (tgtPan - curPan_) * cutoffSmooth_;
                if (i == 0) refreshFilterCoefs();                      // koefficienter var 32:a sampel

                const double target = (localFrac < openLen) ? 1.0 : 0.0;
                gateGain_ += (target - gateGain_) * gateSmooth_;
                outGain_ += (outTarget - outGain_) * outSmooth_;

                double dryv[2], wetv[2];
                const double in2[2] = { (double)inL[pos], (double)inR[pos] };
                for (int ch = 0; ch < 2; ++ch) {
                    double y = in2[ch];
                    if (!bypassHP_) {
                        y = dsp::runBiquad(hpC_[0], hpS_[ch][0], y);
                        y = dsp::runBiquad(hpC_[1], hpS_[ch][1], y);
                    }
                    if (!bypassLP_) {
                        y = dsp::runBiquad(lpC_[0], lpS_[ch][0], y);
                        y = dsp::runBiquad(lpC_[1], lpS_[ch][1], y);
                    }
                    dryv[ch] = in2[ch];
                    wetv[ch] = y * gateGain_;
                }

                // True stereo constant power-panorering: vänster och höger ingångskanal får varsin
                // pan-position och summeras till master L/R. Mitten = oförändrat.
                const double pn = curPan_;
                double panL, panR;
                if (pn <= 0.0) { panL = -1.0;          panR = 1.0 + pn * 2.0; }
                else           { panR = 1.0;           panL = -1.0 + pn * 2.0; }
                const double aL = (panL + 1.0) * dsp::kPi * 0.25;
                const double aR = (panR + 1.0) * dsp::kPi * 0.25;
                const double wetL = wetv[0] * std::cos(aL) + wetv[1] * std::cos(aR);
                const double wetR = wetv[0] * std::sin(aL) + wetv[1] * std::sin(aR);

                const float oL = (float)((dryv[0] * (1.0 - mix) + wetL * mix) * outGain_);
                const float oR = (float)((dryv[1] * (1.0 - mix) + wetR * mix) * outGain_);
                store(outL, pos, oL, accumulate);
                store(outR, pos, oR, accumulate);
                if (std::fabs(oL) > pkL) pkL = std::fabs(oL);
                if (std::fabs(oR) > pkR) pkR = std::fabs(oR);
            }
        }
        displayPos_ = lastPos;
        peakL_ = pkL; peakR_ = pkR;
    }

private:
    static double clamp01(double x) { return !(x >= 0.0) ? 0.0 : (x > 1.0 ? 1.0 : x); }
    static void store(float* out, int i, float v, bool accumulate)
    {
        if (accumulate) out[i] += v; else out[i] = v;
    }
    double targetOutGain() const
    {
        return std::pow(10.0, outputDbFromNorm(clamp01(p_[kParamOutput])) / 20.0);
    }

    void resetFilters()
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int k = 0; k < 2; ++k) { hpS_[ch][k] = dsp::BiquadState(); lpS_[ch][k] = dsp::BiquadState(); }
        primed_ = false;
        curHP_ = 0.0; curLP_ = 1.0; curRes_ = 0.0; curPan_ = 0.0;
        bypassHP_ = bypassLP_ = true;
    }

    void refreshFilterCoefs()
    {
        const bool nbHP = curHP_ < 0.01;
        const bool nbLP = curLP_ > 0.99;
        if (nbHP && !bypassHP_)
            for (int ch = 0; ch < 2; ++ch) for (int k = 0; k < 2; ++k) hpS_[ch][k] = dsp::BiquadState();
        if (nbLP && !bypassLP_)
            for (int ch = 0; ch < 2; ++ch) for (int k = 0; k < 2; ++k) lpS_[ch][k] = dsp::BiquadState();
        bypassHP_ = nbHP;
        bypassLP_ = nbLP;
        if (!bypassHP_) {
            const double f = hzFromNorm(curHP_);
            hpC_[0] = dsp::makeBiquad(true, f, dsp::kQ1, sr_);
            hpC_[1] = dsp::makeBiquad(true, f, dsp::kQ2, sr_);
        }
        if (!bypassLP_) {
            const double f = hzFromNorm(curLP_);
            // resonans: höj Q i andra sektionen (liknar TB-303:s squelch); 0 = platt Butterworth
            const double q2 = dsp::kQ2 * std::pow(dsp::kQMax / dsp::kQ2, curRes_);
            lpC_[0] = dsp::makeBiquad(false, f, dsp::kQ1, sr_);
            lpC_[1] = dsp::makeBiquad(false, f, q2, sr_);
        }
    }

    float p_[kNumParams];
    double sr_ = 44100.0;
    double gateSmooth_ = 0.01, cutoffSmooth_ = 0.01, outSmooth_ = 0.01;
    double gateGain_ = 1.0, outGain_ = 1.0;
    bool primed_ = false, bypassHP_ = true, bypassLP_ = true;
    double curHP_ = 0.0, curLP_ = 1.0, curRes_ = 0.0, curPan_ = 0.0;
    dsp::BiquadCoef hpC_[2], lpC_[2];
    dsp::BiquadState hpS_[2][2], lpS_[2][2];   // [kanal][sektion]

    bool tempoValid_ = false, playing_ = false, ppqValid_ = false;
    double tempo_ = 120.0, ppq_ = 0.0;
    int displayPos_ = -1;
    float peakL_ = 0.0f, peakR_ = 0.0f;
};

} // namespace truestep
