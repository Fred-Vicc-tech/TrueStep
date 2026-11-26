// TrueStepTests.cpp - automatiska tester av ljudmotorn, kärnan och presets
// (inga Windows-delar). Körs i varje bygge (ctest) och på alla datorer.
#include "TrueStepCore.h"
#include "TrueStepGui.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace truestep;

static int g_fails = 0, g_checks = 0;
#define CHECK(c) do { ++g_checks; if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); ++g_fails; } } while (0)
static bool near_(double a, double b, double e) { return std::fabs(a - b) <= e; }

static const double kSr = 44100.0;

// Kör motorn med tempo 120 (en 16-del = 5512,5 sampel) och position från start.
struct Rig
{
    Engine e;
    float v[kNumParams];
    Rig(double sr = kSr) { e.prepare(sr); for (int i = 0; i < kNumParams; ++i) v[i] = defaultParam(i); }
    void allOpen() { for (int s = 0; s < kNumSteps; ++s) { v[paramLen(s, 0)] = 1.0f; v[paramLen(s, 1)] = 1.0f; } }
    // spelar n sampel (i block om blockSize), ger utsignalen
    void run(const std::vector<float>& inL, const std::vector<float>& inR, std::vector<float>& outL, std::vector<float>& outR,
             int blockSize = 256, bool playing = true, double ppq0 = 0.0, double tempo = 120.0, double sr = kSr)
    {
        const int N = (int)inL.size();
        outL.assign(N, 0.0f); outR.assign(N, 0.0f);
        e.setParams(v);
        for (int s0 = 0; s0 < N; s0 += blockSize) {
            const int n = (N - s0 < blockSize) ? (N - s0) : blockSize;
            e.setTransport(true, tempo, playing, true, ppq0 + (s0 / sr) * (tempo / 60.0));
            e.process(&inL[s0], &inR[s0], &outL[s0], &outR[s0], n, false);
        }
    }
};

static double amp(const std::vector<float>& y, int a, int b, double f, double sr = kSr)
{
    double sc = 0, ss = 0;
    for (int i = a; i < b; ++i) { sc += y[i] * std::cos(2 * dsp::kPi * f * i / sr); ss += y[i] * std::sin(2 * dsp::kPi * f * i / sr); }
    return 2.0 * std::sqrt(sc * sc + ss * ss) / (b - a);
}

static void testParams()
{
    std::set<std::string> seen;
    for (int i = 0; i < kNumParams; ++i) {
        const char* n = paramName(i);
        CHECK(n[0] != 0 && std::strlen(n) <= 8);
        CHECK(seen.insert(n).second);                         // unika namn (används i presetfiler)
        CHECK(defaultParam(i) >= 0.0f && defaultParam(i) <= 1.0f);
    }
    CHECK(kNumParams == 180 && kParamHumanize == 178 && kParamSampleHold == 179);
    CHECK(near_(outputDbFromNorm(defaultParam(kParamOutput)), 0.0, 1e-4));
    char b[32];
    paramDisplayText(kParamPanBase, valueFromSlot(0), b, sizeof b); CHECK(!std::strcmp(b, "50L"));
    paramDisplayText(kParamPanBase, valueFromSlot(3), b, sizeof b); CHECK(!std::strcmp(b, "10L"));
    paramDisplayText(kParamPanBase, valueFromSlot(4), b, sizeof b); CHECK(!std::strcmp(b, "C"));
    paramDisplayText(kParamPan2Base + 3, valueFromSlot(8), b, sizeof b); CHECK(!std::strcmp(b, "50R"));
    paramDisplayText(kParamResBase, valueFromSlot(8), b, sizeof b); CHECK(!std::strcmp(b, "8"));
    paramDisplayText(kParamSplitBase, 1.0f, b, sizeof b); CHECK(!std::strcmp(b, "On"));
    paramDisplayText(kParamHPBase, 0.0f, b, sizeof b); CHECK(!std::strcmp(b, "20"));
    paramDisplayText(kParamLPBase, 1.0f, b, sizeof b); CHECK(!std::strcmp(b, "20000"));
    paramDisplayText(kParamOutput, outputNormFromDb(-6.0), b, sizeof b); CHECK(!std::strcmp(b, "-6.0"));
    paramDisplayText(kParamMix, 0.5f, b, sizeof b); CHECK(!std::strcmp(b, "50"));
    for (int s = 0; s <= 8; ++s) CHECK(slotFromValue(valueFromSlot(s)) == s);
    CHECK(slotFromValue(-1.0f) == 0 && slotFromValue(2.0f) == 8 && slotFromValue(NAN) == 0);
}

static void testGate()
{
    Rig r;
    const int N = (int)(kSr * 0.5);
    std::vector<float> one(N, 1.0f), oL, oR;
    r.run(one, one, oL, oR);
    const double st = kSr * 0.125;
    for (int s = 0; s < 4; ++s) {
        CHECK(oL[(int)((s + 0.25) * st)] > 0.99f);   // öppen första halvan (längd 50 %)
        CHECK(oL[(int)((s + 0.75) * st)] < 0.01f);   // stängd andra halvan
    }
    CHECK(r.e.displayPos() >= 0 && r.e.displayPos() < 32);
    // längd 100 % = alltid öppen, 0 % = alltid stängd
    r.v[paramLen(1, 0)] = 1.0f; r.v[paramLen(2, 0)] = 0.0f;
    r.run(one, one, oL, oR);
    CHECK(oL[(int)(1.75 * st)] > 0.99f);
    CHECK(oL[(int)(2.25 * st)] < 0.01f);
    // mjuka övergångar: inga hopp större än 0,2 per sampel
    float maxStep = 0.0f;
    for (int i = 1; i < N; ++i) maxStep = std::fmax(maxStep, std::fabs(oL[i] - oL[i - 1]));
    CHECK(maxStep < 0.2f);
    // uppspelningen står still: ljudet orört, displayPos = -1
    r.run(one, one, oL, oR, 256, false);
    CHECK(near_(oL[1000], 1.0, 1e-4) && near_(oR[N - 1], 1.0, 1e-4) && r.e.displayPos() == -1);
    // ogiltiga värden från värden räknas som stillastående
    r.e.setParams(r.v);
    r.e.setTransport(true, 0.0, true, true, 0.0);
    std::vector<float> a(512, 0.5f), b(512), c(512);
    r.e.process(a.data(), a.data(), b.data(), c.data(), 512, false);
    CHECK(near_(b[100], 0.5, 1e-4));
    r.e.setTransport(true, 120.0, true, true, NAN);
    r.e.process(a.data(), a.data(), b.data(), c.data(), 512, false);
    CHECK(near_(b[100], 0.5, 1e-4));
    // positionen styr steget: börja mitt i steg 5 (index 4) med längd 0 -> tyst
    Rig q; q.v[paramLen(4, 0)] = 0.0f;
    std::vector<float> two(2000, 1.0f);
    q.run(two, two, oL, oR, 256, true, 1.0 + 0.05);   // 1 beat = 4 sextondelar -> steg index 4
    CHECK(oL[1500] < 0.01f);
}

static void testFilters()
{
    Rig r; r.allOpen();
    const int N = (int)(kSr * 1.0);
    std::vector<float> in(N);
    for (int i = 0; i < N; ++i)
        in[i] = (float)(0.5 * std::sin(2 * dsp::kPi * 100 * i / kSr) + 0.5 * std::sin(2 * dsp::kPi * 1000 * i / kSr)
                        + 0.5 * std::sin(2 * dsp::kPi * 8000 * i / kSr));
    r.v[paramLP(0, 0)] = 0.30f;                                         // steg 1: lowpass ca 159 Hz
    r.v[paramHP(1, 0)] = 0.70f;                                         // steg 2: highpass ca 2,5 kHz
    r.v[paramHP(2, 0)] = 0.50f; r.v[paramLP(2, 0)] = 0.70f;             // steg 3: band
    std::vector<float> oL, oR;
    r.run(in, in, oL, oR);
    const double st = kSr * 0.125;
    auto at = [&](int step, double f) { return amp(oL, (int)((step + 0.5) * st), (int)((step + 0.95) * st), f); };
    CHECK(at(0, 100) > 0.45 && at(0, 1000) < 0.02 && at(0, 8000) < 0.005);
    CHECK(at(1, 100) < 0.005 && at(1, 1000) < 0.05 && at(1, 8000) > 0.45);
    CHECK(at(2, 100) < 0.01 && at(2, 1000) > 0.4 && at(2, 8000) < 0.01);
    CHECK(at(3, 100) > 0.45 && at(3, 1000) > 0.45 && at(3, 8000) > 0.45);   // öppet
    for (float y : oL) CHECK(std::isfinite(y));
    // lowpass kan inte ligga under highpass (korsning): inga konstiga värden
    Rig q; q.allOpen(); q.v[paramHP(0, 0)] = 0.8f; q.v[paramLP(0, 0)] = 0.2f;
    q.run(in, in, oL, oR);
    float mx = 0; for (float y : oL) { CHECK(std::isfinite(y)); mx = std::fmax(mx, std::fabs(y)); }
    CHECK(mx < 4.0f);
}

static void testResonance()
{
    auto measure = [](int slot) {
        Rig r; r.allOpen();
        const double f = hzFromNorm(0.5663);
        for (int s = 0; s < kNumSteps; ++s) { r.v[paramLP(s, 0)] = 0.5663f; r.v[paramRes(s, 0)] = valueFromSlot(slot); }
        const int N = (int)(kSr * 0.12);
        std::vector<float> in(N), oL, oR;
        for (int i = 0; i < N; ++i) in[i] = (float)std::sin(2 * dsp::kPi * f * i / kSr);
        r.run(in, in, oL, oR);
        double g = amp(oL, N / 2, N - 200, f);
        for (float y : oL) CHECK(std::isfinite(y));
        return g;
    };
    const double g0 = measure(0), g4 = measure(4), g8 = measure(8);
    std::printf("resonance gain at cutoff: %.2f %.2f %.2f\n", g0, g4, g8);
    CHECK(g0 > 0.6 && g0 < 0.8);       // Butterworth: ca -3 dB
    CHECK(g4 > g0 * 1.5 && g8 > g4 * 1.5);
    CHECK(g8 < 8.0);
}

static void testSplit()
{
    Rig r; r.allOpen();
    r.v[kParamSplitBase + 0] = 1.0f; r.v[paramLen(0, 0)] = 1.0f; r.v[paramLen(0, 1)] = 0.0f;
    r.v[kParamSplitBase + 1] = 1.0f; r.v[paramLen(1, 0)] = 0.0f; r.v[paramLen(1, 1)] = 1.0f;
    r.v[kParamSplitBase + 2] = 1.0f; r.v[paramLen(2, 0)] = 0.5f; r.v[paramLen(2, 1)] = 0.5f;
    const int N = (int)(kSr * 0.5);
    std::vector<float> one(N, 1.0f), oL, oR;
    r.run(one, one, oL, oR);
    const double st = kSr * 0.125;
    auto at = [&](double p) { return oL[(int)(p * st)]; };
    CHECK(at(0.25) > 0.99 && at(0.75) < 0.01);
    CHECK(at(1.25) < 0.01 && at(1.75) > 0.99);
    CHECK(at(2.12) > 0.99 && at(2.37) < 0.01 && at(2.62) > 0.99 && at(2.87) < 0.01);
    // halvorna har egna filter
    Rig q; q.allOpen();
    q.v[kParamSplitBase + 0] = 1.0f; q.v[paramLP(0, 1)] = 0.30f;       // andra halvan: lowpass
    std::vector<float> in(N);
    for (int i = 0; i < N; ++i) in[i] = (float)(0.5 * std::sin(2 * dsp::kPi * 100 * i / kSr) + 0.5 * std::sin(2 * dsp::kPi * 8000 * i / kSr));
    q.run(in, in, oL, oR);
    CHECK(amp(oL, (int)(0.15 * st), (int)(0.45 * st), 8000) > 0.4);    // första halvan öppen
    CHECK(amp(oL, (int)(0.65 * st), (int)(0.95 * st), 8000) < 0.01);   // andra halvan: diskant borta
}

static void testPan()
{
    const int N = (int)(kSr * 0.1);
    struct T { int slot; double l, r; };
    // L = 1, R = 0,5 (stereo). Formeln: p = steg / 50; p <= 0: panL = -1, panR = 1 + 2p
    double p22 = -22.0 / 50.0; double aR = ((1.0 + 2 * p22) + 1.0) * dsp::kPi * 0.25;
    T ts[] = { { 4, 1.0, 0.5 }, { 0, 1.5, 0.0 }, { 8, 0.0, 1.5 }, { 2, 1.0 + 0.5 * std::cos(aR), 0.5 * std::sin(aR) } };
    for (const T& t : ts) {
        Rig r; r.allOpen();
        for (int s = 0; s < kNumSteps; ++s) r.v[paramPan(s, 0)] = valueFromSlot(t.slot);
        std::vector<float> L(N, 1.0f), R(N, 0.5f), oL, oR;
        r.run(L, R, oL, oR);
        CHECK(near_(oL[N / 2], t.l, 0.01) && near_(oR[N / 2], t.r, 0.01));
    }
    // symmetri: höger sida är spegelbilden av vänster
    for (int k = 1; k <= 4; ++k) {
        Rig a, b; a.allOpen(); b.allOpen();
        for (int s = 0; s < kNumSteps; ++s) { a.v[paramPan(s, 0)] = valueFromSlot(4 - k); b.v[paramPan(s, 0)] = valueFromSlot(4 + k); }
        std::vector<float> L(N, 0.8f), R(N, 0.3f), aL, aR2, bL, bR;
        a.run(L, R, aL, aR2); b.run(R, L, bL, bR);       // spegelvänd ingång
        CHECK(near_(aL[N / 2], bR[N / 2], 1e-4) && near_(aR2[N / 2], bL[N / 2], 1e-4));
    }
    // pan-läget byts mjukt mellan steg (inga hopp)
    Rig r; r.allOpen();
    for (int s = 0; s < kNumSteps; ++s) r.v[paramPan(s, 0)] = valueFromSlot((s % 2) ? 8 : 0);
    const int M = (int)(kSr * 0.4);
    std::vector<float> one(M, 1.0f), oL, oR;
    r.run(one, one, oL, oR);
    float maxStep = 0; for (int i = 1; i < M; ++i) maxStep = std::fmax(maxStep, std::fabs(oL[i] - oL[i - 1]));
    CHECK(maxStep < 0.3f);
    // total effekt (sqrt(L^2+R^2)) för en enda kanal är konstant över alla lägen (constant power)
    for (int slot = 0; slot <= 8; ++slot) {
        Rig q; q.allOpen();
        for (int s = 0; s < kNumSteps; ++s) q.v[paramPan(s, 0)] = valueFromSlot(slot);
        std::vector<float> L(N, 1.0f), R(N, 0.0f), aL, aR2;
        q.run(L, R, aL, aR2);
        const double power = std::sqrt((double)aL[N / 2] * aL[N / 2] + (double)aR2[N / 2] * aR2[N / 2]);
        CHECK(near_(power, 1.0, 0.01));
    }
}

static void testMixOutput()
{
    Rig r; r.allOpen();
    const int N = (int)(kSr * 0.3);
    std::vector<float> one(N, 0.5f), oL, oR;
    for (int s = 0; s < kNumSteps; ++s) r.v[paramLen(s, 0)] = 0.0f;       // allt stängt
    r.v[kParamMix] = 0.0f; r.run(one, one, oL, oR);
    CHECK(near_(oL[N / 2], 0.5, 1e-4));                                    // mix 0 = bara dry
    r.v[kParamMix] = 1.0f; r.run(one, one, oL, oR);
    CHECK(std::fabs(oL[N / 2]) < 0.01);                                    // mix 100 = bara wet (tyst)
    r.v[kParamMix] = 0.5f; r.run(one, one, oL, oR);
    CHECK(near_(oL[N / 2], 0.25, 0.01));
    // OUTPUT -6 dB och +12 dB (efter mjukt nå)
    r.v[kParamMix] = 0.0f; r.v[kParamOutput] = outputNormFromDb(-6.0);
    r.run(one, one, oL, oR);
    CHECK(near_(oL[N - 1], 0.5 * std::pow(10.0, -6.0 / 20.0), 0.002));
    r.v[kParamOutput] = outputNormFromDb(12.0);
    r.run(one, one, oL, oR);
    CHECK(near_(oL[N - 1], 0.5 * std::pow(10.0, 12.0 / 20.0), 0.01));
    r.v[kParamOutput] = outputNormFromDb(-24.0);
    r.run(one, one, oL, oR);
    CHECK(near_(oL[N - 1], 0.5 * std::pow(10.0, -24.0 / 20.0), 0.002));
    // OUTPUT gäller även när uppspelningen står still
    r.run(one, one, oL, oR, 256, false);
    CHECK(near_(oL[N - 1], 0.5 * std::pow(10.0, -24.0 / 20.0), 0.002));
    // toppnivån rapporteras
    r.v[kParamOutput] = outputNormFromDb(12.0);
    std::vector<float> big(2000, 0.9f);
    r.run(big, big, oL, oR, 2000);
    CHECK(r.e.peakL() > 1.0f);     // 0,9 * 4 (+12 dB) > 1 (över 0 dBFS)
}

static void testBuffers()
{
    // in == out (på plats) och accumulate
    Rig r; r.allOpen();
    for (int s = 0; s < kNumSteps; ++s) { r.v[paramLP(s, 0)] = 0.6f; r.v[paramPan(s, 0)] = valueFromSlot(1); }
    const int N = 4000;
    std::vector<float> in(N), a, b, c, d;
    for (int i = 0; i < N; ++i) in[i] = (float)std::sin(2 * dsp::kPi * 300 * i / kSr);
    std::vector<float> in2 = in;
    r.run(in, in2, a, b);                                    // separata buffertar
    std::vector<float> pl = in, pr = in2;
    r.e.reset(); r.e.setParams(r.v); r.e.setTransport(true, 120.0, true, true, 0.0);
    r.e.process(pl.data(), pr.data(), pl.data(), pr.data(), N, false);   // på plats, ett enda block
    // jämför med motsvarande körning i ett enda block
    Rig r2; r2.v[0] = 0; for (int i = 0; i < kNumParams; ++i) r2.v[i] = r.v[i];
    r2.e.reset(); r2.run(in, in2, c, d, N);
    bool same = true; for (int i = 0; i < N; ++i) if (std::fabs(pl[i] - c[i]) > 1e-6f || std::fabs(pr[i] - d[i]) > 1e-6f) same = false;
    CHECK(same);
    // accumulate lägger till
    std::vector<float> acc(N, 1.0f), acc2(N, 1.0f);
    r2.e.reset(); r2.e.setParams(r2.v); r2.e.setTransport(true, 120.0, true, true, 0.0);
    r2.e.process(in.data(), in2.data(), acc.data(), acc2.data(), N, true);
    same = true; for (int i = 0; i < N; ++i) if (std::fabs(acc[i] - (1.0f + c[i])) > 1e-5f) same = false;
    CHECK(same);
    // blockstorlek påverkar inte ljudet (utom i hur ofta filter uppdateras: samma 32-sampelrutnät)
    std::vector<float> e1, e2, f1, f2;
    Rig r3; for (int i = 0; i < kNumParams; ++i) r3.v[i] = r.v[i];
    r3.run(in, in2, e1, e2, 32); Rig r4; for (int i = 0; i < kNumParams; ++i) r4.v[i] = r.v[i];
    r4.run(in, in2, f1, f2, 512);
    double maxd = 0; for (int i = 0; i < N; ++i) maxd = std::fmax(maxd, std::fabs(e1[i] - f1[i]));
    CHECK(maxd < 1e-3);
    // mono-ut (outR = null) och utan indata kraschar inte
    r.e.process(in.data(), nullptr, a.data(), nullptr, 100, false);
    r.e.process(nullptr, nullptr, a.data(), b.data(), 100, false);
    CHECK(a[10] == 0.0f);
    // andra samplingsfrekvenser
    for (double sr : { 22050.0, 48000.0, 96000.0, 192000.0 }) {
        Rig q(sr); q.allOpen(); q.v[paramLP(0, 0)] = 0.5f; q.v[paramRes(0, 0)] = valueFromSlot(8);
        std::vector<float> x((int)(sr * 0.3)), o1, o2;
        for (size_t i = 0; i < x.size(); ++i) x[i] = (float)std::sin(2 * dsp::kPi * 440 * i / sr);
        q.run(x, x, o1, o2, 256, true, 0.0, 120.0, sr);
        bool fin = true; for (float y : o1) fin = fin && std::isfinite(y) && std::fabs(y) < 20.0f;
        CHECK(fin);
    }
}

static void testPresetsAndState()
{
    // fabrikspresets: giltiga värden, namn, ljud utan konstigheter
    std::set<std::string> names;
    for (int p = 0; p < kNumFactoryPresets; ++p) {
        float v[kNumParams];
        factoryPresetValues(p, v);
        bool ok = true; for (int i = 0; i < kNumParams; ++i) ok = ok && v[i] >= 0.0f && v[i] <= 1.0f;
        CHECK(ok);
        CHECK(std::strlen(factoryPresetName(p)) > 0 && std::strlen(factoryPresetName(p)) <= 23);
        CHECK(names.insert(factoryPresetName(p)).second);
        Rig r; for (int i = 0; i < kNumParams; ++i) r.v[i] = v[i];
        const int N = (int)(kSr * 2.1);
        std::vector<float> in(N), oL, oR;
        for (int i = 0; i < N; ++i) in[i] = (float)(0.4 * std::sin(2 * dsp::kPi * 220 * i / kSr) + 0.3 * std::sin(2 * dsp::kPi * 3300 * i / kSr));
        r.run(in, in, oL, oR);
        bool good = true; float mx = 0; for (float y : oL) { good = good && std::isfinite(y); mx = std::fmax(mx, std::fabs(y)); }
        CHECK(good); CHECK(mx < 6.0f);
    }
    float d0[kNumParams], d1[kNumParams];
    factoryPresetValues(0, d0);
    for (int i = 0; i < kNumParams; ++i) CHECK(d0[i] == defaultParam(i));    // första = standard
    factoryPresetValues(1, d1);
    CHECK(d1[paramLen(5, 0)] == 1.0f);

    // presetfiler: spara och läs tillbaka
    float v[kNumParams], w[kNumParams];
    for (int i = 0; i < kNumParams; ++i) v[i] = (float)((i * 53) % 101) / 100.0f;
    std::string name;
    const std::string text = presetToText("My Preset", v);
    CHECK(presetFromText(text, w, &name));
    CHECK(name == "My Preset");
    bool same = true; for (int i = 0; i < kNumParams; ++i) if (std::fabs(v[i] - w[i]) > 1e-5f) same = false;
    CHECK(same);
    CHECK(!presetFromText("hello world", w, &name));
    CHECK(!presetFromText("", w, &name));
    // trasiga rader ignoreras, saknade rader får standardvärde
    CHECK(presetFromText(std::string(kPresetHeader) + "\r\nName=Odd\r\nS01 Len=abc\r\nS02 Len=7\r\nS03 Len=0.25\r\nfoo=1\r\n=\r\n", w, &name));
    CHECK(w[paramLen(0, 0)] == defaultParam(paramLen(0, 0)));
    CHECK(w[paramLen(1, 0)] == defaultParam(paramLen(1, 0)));
    CHECK(near_(w[paramLen(2, 0)], 0.25, 1e-6));
    CHECK(w[kParamMix] == 1.0f && name == "Odd");
    // namn
    CHECK(cleanPresetName("") == "Untitled");
    CHECK(cleanPresetName("a=b") == "ab");
    CHECK(cleanPresetName(std::string(60, 'x')).size() == 31);
    auto exists = [](const std::string& n) { return n == "Wide" || n == "Wide 2"; };
    CHECK(uniquePresetName("Wide", exists) == "Wide 3");
    CHECK(uniquePresetName("New", exists) == "New");

    // Core: parametrar, tillstånd
    Core a, b;
    a.setParam(kParamMix, NAN); a.setParam(kParamMix, 2.0f); a.setParam(-1, 0.5f); a.setParam(kNumParams, 0.5f);
    CHECK(a.params[kParamMix].load() == 1.0f);
    for (int i = 0; i < kNumParams; ++i) a.setParam(i, v[i]);
    a.program = -1; copyString(a.presetName, "Hello", sizeof a.presetName);
    const StateChunk& sc = a.saveState();
    CHECK(b.loadState(&sc, sizeof sc));
    same = true; for (int i = 0; i < kNumParams; ++i) if (a.params[i].load() != b.params[i].load()) same = false;
    CHECK(same && b.program == -1 && !std::strcmp(b.presetName, "Hello"));
    // gamla/korta/trasiga tillstånd
    Core c;
    unsigned char junk[100]; std::memset(junk, 0x7F, sizeof junk);
    CHECK(!c.loadState(junk, sizeof junk));
    CHECK(!c.loadState(nullptr, 100));
    CHECK(!c.loadState(&sc, 10));
    std::vector<unsigned char> shortChunk((const unsigned char*)&sc, (const unsigned char*)&sc + kChunkHeader + 4 * 50);
    uint32_t cnt = 50; std::memcpy(shortChunk.data() + 8, &cnt, 4);
    CHECK(c.loadState(shortChunk.data(), shortChunk.size()));
    CHECK(c.params[10].load() == v[10] && c.params[100].load() == defaultParam(100));
    StateChunk bad = sc; for (int i = 0; i < 20; ++i) bad.values[i] = (i % 2) ? NAN : 9.0f;
    CHECK(c.loadState(&bad, sizeof bad));
    for (int i = 0; i < 20; ++i) CHECK(c.params[i].load() == defaultParam(i));
    // fabrikspreset via Core
    c.loadFactoryPreset(3, false);
    CHECK(c.program == 3 && !std::strcmp(c.presetName, "Offbeat Pulse"));
    c.loadFactoryPreset(99, false); CHECK(c.program == 3);
    // processAudio fyller toppnivå och position
    Core p;
    TransportInfo t; t.tempoValid = t.ppqValid = t.playing = true; t.tempo = 120; t.ppq = 0;
    std::vector<float> x(1024, 0.5f), y1(1024), y2(1024);
    p.processAudio(x.data(), x.data(), y1.data(), y2.data(), 1024, false, t);
    CHECK(p.playPos.load() >= 0 && p.peakL.load() > 0.4f);
    t.playing = false;
    p.processAudio(x.data(), x.data(), y1.data(), y2.data(), 1024, false, t);
    CHECK(p.playPos.load() == -1);
}


// ---------------------------------------------------------------------------
// Gränssnittet (musstyrning och ritning utan Windows)
// ---------------------------------------------------------------------------
struct FakeGuiHost : GuiHost
{
    float p[kNumParams];
    int begins = 0, ends = 0, sets = 0, pos = -1;
    std::string name = "Default Gate";
    float peak = 0.0f;
    FakeGuiHost() { for (int i = 0; i < kNumParams; ++i) p[i] = defaultParam(i); }
    float get(int i) override { return p[i]; }
    void set(int i, float v) override { p[i] = v; ++sets; }
    void beginEdit(int) override { ++begins; }
    void endEdit(int) override { ++ends; }
    const char* presetName() override { return name.c_str(); }
    int playPos() override { return pos; }
    void takePeaks(float& l, float& r) override { l = r = peak; }
};

struct CountCanvas : Canvas
{
    int prims = 0, clips = 0, unclips = 0;
    std::vector<std::string> texts;
    void fillRect(float, float, float, float, Rgba) override { ++prims; }
    void gradientV(float, float, float, float, Rgba, Rgba) override { ++prims; }
    void line(float, float, float, float, Rgba, float) override { ++prims; }
    void roundRect(float, float, float, float, float, Rgba, Rgba, float) override { ++prims; }
    void ellipse(float, float, float, float, Rgba, Rgba, float) override { ++prims; }
    void arc(float, float, float, float, float, Rgba, float) override { ++prims; }
    void polygon(const float*, int, Rgba) override { ++prims; }
    void text(float, float, float, const char* s, float, int, Rgba, int) override { texts.push_back(s); }
    float textHeight(const char* s, float w, float size, int) override
    {
        const float chars = (float)std::strlen(s) * size * 0.52f;
        return (std::floor(chars / w) + 1.0f) * size * 1.33f;
    }
    float textWidth(const char* s, float size, int) override { return (float)std::strlen(s) * size * 0.52f; }
    void clip(float, float, float, float) override { ++clips; }
    void unclip() override { ++unclips; }
    bool has(const char* t) const { for (const std::string& x : texts) if (x == t) return true; return false; }
};

static void testGui()
{
    // geometri (se TrueStepGui.h): kolumn s börjar vid x = 78 + 48 s, stapelns topp/botten y = 134 / 304
    FakeGuiHost h;
    StepGui g(&h);
    CountCanvas cv;
    g.draw(cv, 1000);                                   // mäter logon (loggans klickyta)
    CHECK(cv.prims > 300 && cv.has("STEP GRID") && cv.has("PRESET") && cv.has("DRY/WET") && cv.has("OUTPUT"));
    CHECK(cv.has("Default Gate") && cv.has("100 %") && cv.has("0.0 dB") && cv.has("TrueStep  v1.0"));
    CHECK(cv.clips == cv.unclips);

    // 1) längd: dra stapelns högerkant (steg 3: x 175..198, len 50 %)
    g.mouseDown(198, 200, false);
    g.mouseMove(212, 200, false);
    g.mouseUp();
    CHECK(near_(h.p[paramLen(2, 0)], 0.80, 0.03));
    CHECK(h.begins == h.ends && h.begins >= 1);
    CHECK(near_(h.p[paramLen(1, 0)], 0.5, 1e-6));       // grannsteget orört
    // 2) lowpass: dra överkanten nedåt
    g.mouseDown(185, 134, false); g.mouseMove(185, 219, false); g.mouseUp();
    CHECK(near_(h.p[paramLP(2, 0)], 0.5, 0.01));
    // 3) highpass: dra underkanten uppåt
    g.mouseDown(185, 304, false); g.mouseMove(185, 270, false); g.mouseUp();
    CHECK(near_(h.p[paramHP(2, 0)], 0.2, 0.01));
    // 4) kroppen flyttar båda filtren lika mycket
    const float hp0 = h.p[paramHP(2, 0)], lp0 = h.p[paramLP(2, 0)];
    g.mouseDown(185, 245, false); g.mouseMove(185, 228, false); g.mouseUp();
    CHECK(near_(h.p[paramHP(2, 0)] - hp0, 0.1, 0.01) && near_(h.p[paramLP(2, 0)] - lp0, 0.1, 0.01));
    // 5) lowpass kan inte korsa highpass
    g.mouseDown(185, (int)(134 + (1.0f - h.p[paramLP(2, 0)]) * 170), false); g.mouseMove(185, 300, false); g.mouseUp();
    CHECK(h.p[paramLP(2, 0)] >= h.p[paramHP(2, 0)] + 0.0199f);
    // 6) klick ovanför/under stapeln flyttar lowpass/highpass dit, klick till höger sätter längden
    h.p[paramLP(5, 0)] = 0.4f; h.p[paramHP(5, 0)] = 0.2f; h.p[paramLen(5, 0)] = 0.2f;       // steg 6: x 318..366
    g.mouseDown(350, 140, false); g.mouseUp();                                                // över stapeln, till höger om den
    CHECK(h.p[paramLP(5, 0)] > 0.9f);
    g.mouseDown(350, 300, false); g.mouseUp();
    CHECK(h.p[paramHP(5, 0)] < 0.1f);
    h.p[paramLP(5, 0)] = 0.8f; h.p[paramHP(5, 0)] = 0.2f;
    g.mouseDown(360, 200, false); g.mouseUp();                                                // mellan kanterna, till höger
    CHECK(h.p[paramLen(5, 0)] > 0.8f);
    // 7) dubbelklick nollställer hela steget
    g.mouseDown(185, 200, true); g.mouseUp();
    CHECK(h.p[paramLen(2, 0)] == 1.0f && h.p[paramHP(2, 0)] == 0.0f && h.p[paramLP(2, 0)] == 1.0f
          && near_(h.p[paramPan(2, 0)], 0.5, 1e-6) && h.p[paramRes(2, 0)] == 0.0f);
    CHECK(h.begins == h.ends);

    // 8) 1/32-knappen delar steget (steg 5: x 270..318) och kopierar första halvan
    h.p[paramLen(4, 0)] = 0.7f; h.p[paramHP(4, 0)] = 0.3f; h.p[paramLP(4, 0)] = 0.8f; h.p[paramPan(4, 0)] = valueFromSlot(6); h.p[paramRes(4, 0)] = valueFromSlot(3);
    g.mouseDown(280, 320, false); g.mouseUp();
    CHECK(h.p[kParamSplitBase + 4] == 1.0f);
    CHECK(h.p[paramLen(4, 1)] == 0.7f && near_(h.p[paramHP(4, 1)], 0.3, 1e-6) && near_(h.p[paramLP(4, 1)], 0.8, 1e-6));
    CHECK(h.p[paramPan(4, 1)] == valueFromSlot(6) && h.p[paramRes(4, 1)] == valueFromSlot(3));
    // 9) andra halvan (x 294..318) styrs för sig: dra dess högerkant
    g.mouseDown(306, 200, false); g.mouseMove(302, 200, false); g.mouseUp();
    CHECK(near_(h.p[paramLen(4, 1)], 7.0 / 22.0, 0.06) && h.p[paramLen(4, 0)] == 0.7f);
    // 10) igen = slå ihop (värdena i andra halvan sparas)
    const float keep = h.p[paramLen(4, 1)];
    g.mouseDown(280, 320, false); g.mouseUp();
    CHECK(h.p[kParamSplitBase + 4] == 0.0f && h.p[paramLen(4, 1)] == keep);

    // 11) PAN och RES (9 lägen): steg 1 har väljare x 81..123
    g.mouseDown(122, 350, false); g.mouseUp(); CHECK(h.p[paramPan(0, 0)] == valueFromSlot(8));
    g.mouseDown(82, 350, false); g.mouseUp();  CHECK(h.p[paramPan(0, 0)] == valueFromSlot(0));
    g.mouseDown(102, 350, false); g.mouseUp(); CHECK(h.p[paramPan(0, 0)] == valueFromSlot(4));
    g.mouseDown(122, 375, false); g.mouseUp(); CHECK(h.p[paramRes(0, 0)] == valueFromSlot(8));
    g.mouseDown(100, 350, false); g.mouseMove(85, 350, false); g.mouseUp(); CHECK(h.p[paramPan(0, 0)] == valueFromSlot(0));
    g.mouseDown(100, 350, true); g.mouseUp(); CHECK(near_(h.p[paramPan(0, 0)], 0.5, 1e-6));    // dubbelklick = C
    g.mouseDown(100, 375, true); g.mouseUp(); CHECK(h.p[paramRes(0, 0)] == 0.0f);
    g.wheel(100, 350, +1); CHECK(h.p[paramPan(0, 0)] == valueFromSlot(5));
    g.wheel(100, 350, -3); CHECK(h.p[paramPan(0, 0)] == valueFromSlot(4));
    // delat steg: andra halvans väljare (steg 4, x 294+2..318-2)
    h.p[kParamSplitBase + 3] = 1.0f;                           // steg 4: x 222..270, halvor 222..246 och 246..270
    g.mouseDown(268, 350, false); g.mouseUp();
    CHECK(h.p[paramPan(3, 1)] == valueFromSlot(8) && h.p[paramPan(3, 0)] == valueFromSlot(4));

    // 12) rattar: DRY/WET (center 123,509) och OUTPUT (center 777,509)
    g.mouseDown(99, 516, false); g.mouseMove(99, 466, false); g.mouseUp();
    CHECK(near_(h.p[kParamMix], 1.0, 1e-6));                   // redan på max
    g.mouseDown(99, 516, false); g.mouseMove(99, 566, false); g.mouseUp();
    CHECK(near_(h.p[kParamMix], 0.8, 1e-4));                   // 50 px ner = -0,2
    g.mouseDown(99, 516, false); g.mouseMove(99, 566, true); g.mouseUp();
    CHECK(near_(h.p[kParamMix], 0.75, 1e-4));                  // Shift = 4 gånger finare (-0,05)
    g.mouseDown(99, 516, true); g.mouseUp(); CHECK(h.p[kParamMix] == 1.0f);
    g.mouseDown(805, 516, false); g.mouseMove(341, 491, false); g.mouseUp();
    CHECK(near_(h.p[kParamOutput], defaultParam(kParamOutput) + 0.1, 1e-4));
    g.mouseDown(805, 516, true); g.mouseUp(); CHECK(near_(h.p[kParamOutput], defaultParam(kParamOutput), 1e-6));
    g.wheel(805, 516, 2); CHECK(near_(h.p[kParamOutput], defaultParam(kParamOutput) + 2 * 0.5 / 36.0, 1e-5));
    g.mouseDown(805, 516, false); g.mouseMove(341, 5000, false); g.mouseUp(); CHECK(h.p[kParamOutput] >= 0.0f && h.p[kParamOutput] <= 1.0f);
    CHECK(h.begins == h.ends);

    // 13) presetknapparna ger åtgärder
    CHECK(g.mouseDown(541, 476, false) == StepGui::kActPresetPrev);
    CHECK(g.mouseDown(689, 476, false) == StepGui::kActPresetNext);
    CHECK(g.mouseDown(556, 520, false) == StepGui::kActSave);
    CHECK(g.mouseDown(656, 520, false) == StepGui::kActLoad);
    CHECK(g.mouseDown(228, 569, false) == StepGui::kActRandom);
    g.mouseDown(173 + 20, 492, false); g.mouseMove(193, 422, false); g.mouseUp();   // S/H-ratten dras uppåt
    CHECK(h.p[kParamSampleHold] > 0.2f);
    CHECK(g.mouseDown(5, 5, false) == StepGui::kActNone);

    // 14) loggan vänder pluginet; manualen går att rulla
    CHECK(!g.showBack);
    g.mouseDown(40, 25, false); g.mouseUp();
    CHECK(g.showBack);
    CountCanvas bc;
    g.draw(bc, 1016); g.draw(bc, 1032);
    CHECK(bc.has("MANUAL") && bc.has("WHAT TRUESTEP DOES") && bc.has("BACK") && bc.clips == bc.unclips && bc.clips >= 2);
    bool credit = false; for (const std::string& t : bc.texts) if (t.find(" 2F Productions 2026") != std::string::npos) credit = true;
    CHECK(credit);
    g.wheel(400, 400, -5);                                       // rulla ner
    CountCanvas bc2; g.draw(bc2, 1048);
    CHECK(!bc2.has("WHAT TRUESTEP DOES"));                      // första rubriken har rullat ut ur bild
    g.wheel(400, 400, 100); CountCanvas bc3; g.draw(bc3, 1064); CHECK(bc3.has("WHAT TRUESTEP DOES")); // rullar upp igen, stannar överst
    g.wheel(400, 400, -1000); g.draw(bc3, 1080);                  // längst ner, kraschar inte
    g.mouseDown(855, 300, false);                                // dra rullningslisten
    g.mouseMove(855, 500, false); g.mouseMove(855, -300, false); g.mouseUp();
    g.mouseDown(780, 130, false); g.mouseUp();                   // BACK
    CHECK(!g.showBack);

    // 15) uppspelningsposition, nivåmätare och konstiga värden kraschar inte
    h.pos = 9; h.peak = 2.0f;                                   // över 0 dBFS
    for (int s = 0; s < kNumSteps; ++s) h.p[kParamSplitBase + s] = (s % 3 == 0) ? 1.0f : 0.0f;
    CountCanvas fc; g.draw(fc, 2000); CHECK(fc.has("OVER 0 dB"));
    g.mouseDown(-100, -100, false); g.mouseMove(99999, 99999, true); g.mouseUp();
    g.mouseDown(9999, 9999, true); g.mouseUp(); g.wheel(-5, -5, 3);
    for (int x = 0; x < StepGui::W; x += 7) for (int y = 0; y < StepGui::H; y += 7) {
        g.mouseDown(x, y, (x + y) % 3 == 0); g.mouseMove(x + 30, y - 40, (x % 2) != 0); g.mouseUp(); g.wheel(x, y, (y % 2) ? 1 : -1);
        if (g.showBack) g.mouseDown(40, 25, false);              // tillbaka till framsidan om loggan råkade tas
    }
    bool valid = true; for (int i = 0; i < kNumParams; ++i) valid = valid && h.p[i] >= 0.0f && h.p[i] <= 1.0f;
    CHECK(valid);
    CHECK(h.begins == h.ends);
    CHECK(g.cursorAt(40, 25) == kCurHand);
}


// HUMANIZE: rosa brus (1/f) som förskjuter stegens start
static void testHumanize()
{
    // Statistik över många steg (depth 25 ms, tempo 120)
    const int M = 20000;
    std::vector<double> o(M);
    double mx = 0, sum = 0, sq = 0;
    for (int k = 0; k < M; ++k) {
        o[k] = Engine::humanOffsetMs(k, 25.0, 120.0);
        mx = std::fmax(mx, std::fabs(o[k]));
        sum += o[k]; sq += o[k] * o[k];
    }
    const double mean = sum / M, sd = std::sqrt(sq / M - mean * mean);
    CHECK(mx <= 25.001);                                   // aldrig mer än HUMANIZE-värdet
    CHECK(std::fabs(mean) < 1.0);
    CHECK(sd > 9.0 && sd < 14.0);                          // ungefär halva värdet (+-2 sigma = värdet)
    auto corr = [&](int lag) {
        double c = 0;
        for (int k = 0; k + lag < M; ++k) c += (o[k] - mean) * (o[k + lag] - mean);
        return c / (M - lag) / (sd * sd);
    };
    CHECK(corr(1) > 0.3 && corr(1) < 0.9);                 // hänger ihop över tid (1/f), inte vitt brus
    CHECK(corr(4) > 0.1);
    CHECK(corr(1) > corr(8) && corr(8) > -0.1);            // korrelationen avtar med avståndet
    // jämn fördelning åt båda hållen
    int pos = 0; for (int k = 0; k < M; ++k) pos += o[k] > 0;
    CHECK(pos > M * 0.45 && pos < M * 0.55);
    CHECK(Engine::humanOffsetMs(7, 25.0, 120.0) == Engine::humanOffsetMs(7, 25.0, 120.0));   // samma steg, samma värde
    CHECK(Engine::humanOffsetMs(-3, 25.0, 120.0) != Engine::humanOffsetMs(3, 25.0, 120.0));  // negativa steg fungerar
    CHECK(Engine::humanOffsetMs(5, 0.0, 120.0) == 0.0);
    // snabbt tempo + stort värde: stegen kan aldrig korsa varandra (gräns 0,4 av en 16-del)
    double worst = 0;
    for (int k = 0; k < 2000; ++k) worst = std::fmax(worst, std::fabs(Engine::humanOffsetMs(k, 25.0, 300.0)) / (60000.0 / 300.0 / 4.0));
    CHECK(worst <= 0.4001);

    // I motorn: stegens start hamnar där förskjutningen säger
    Rig r;
    const int N = (int)(kSr * 2.0);                         // 16 steg (2 s)
    std::vector<float> one(N, 1.0f), oL, oR;
    for (int s = 0; s < kNumSteps; ++s) r.v[paramLen(s, 0)] = 0.5f;
    r.v[kParamHumanize] = 1.0f;
    r.run(one, one, oL, oR, 256);
    const double st = kSr * 0.125;
    int checkedEdges = 0; double minOff = 1e9, maxOff = -1e9;
    for (int k = 2; k < 15; ++k) {
        int edge = -1;
        for (int i = (int)((k - 0.45) * st); i < (int)((k + 0.45) * st); ++i)
            if (oL[i] > 0.5f && oL[i - 1] <= 0.5f) { edge = i; break; }
        CHECK(edge > 0);
        if (edge <= 0) continue;
        const double actualMs = (edge - k * st) / kSr * 1000.0 - 1.39;       // 2 ms jämning: 50 % efter 1,39 ms
        const double wantMs = Engine::humanOffsetMs(k, 25.0, 120.0);
        CHECK(near_(actualMs, wantMs, 0.5));
        minOff = std::fmin(minOff, actualMs); maxOff = std::fmax(maxOff, actualMs);
        ++checkedEdges;
    }
    CHECK(checkedEdges == 13 && maxOff - minOff > 5.0);      // stegen förskjuts verkligen, åt båda håll
    // samma ljud oavsett blockstorlek, och två körningar ger samma resultat
    std::vector<float> aL, aR, bL, bR;
    r.e.reset();
    r.run(one, one, aL, aR, 1024);
    double dmax = 0; for (int i = 0; i < N; ++i) dmax = std::fmax(dmax, std::fabs(aL[i] - oL[i]));
    CHECK(dmax < 1e-6);
    r.e.reset();
    r.run(one, one, bL, bR, 1024);
    CHECK(aL == bL);
    // startar mitt i låten: samma förskjutning som när man spelar dit
    std::vector<float> cL, cR;
    const int half = N / 2;
    std::vector<float> one2(N - half, 1.0f);
    r.e.reset();
    r.run(one2, one2, cL, cR, 256, true, 2.0);               // ppq 2,0 = 1 s = steg 8 = sampel 44100
    double dm = 0; for (int i = 600; i < N - half - 600; ++i) dm = std::fmax(dm, std::fabs(cL[i] - oL[half + i]));
    CHECK(dm < 0.01);
    // 0 ms = ingen förändring mot motorn utan HUMANIZE; alla steg öppna ger oförändrat ljud
    Rig z; z.allOpen(); z.v[kParamHumanize] = 1.0f;
    std::vector<float> zL, zR; z.run(one, one, zL, zR);
    bool allOne = true; for (int i = 2000; i < N; ++i) allOne = allOne && std::fabs(zL[i] - 1.0f) < 1e-3f;
    CHECK(allOne);
    CHECK(r.e.displayPos() >= 0 && r.e.displayPos() < 32);
    // snabbt tempo och splittade steg tål det utan krasch
    Rig q; q.v[kParamHumanize] = 1.0f; for (int s = 0; s < kNumSteps; ++s) q.v[kParamSplitBase + s] = 1.0f;
    std::vector<float> xL, xR; q.run(one, one, xL, xR, 77, true, 3.3, 299.0);
    bool fin = true; for (float x : xL) fin = fin && std::isfinite(x) && std::fabs(x) < 4.0f;
    CHECK(fin);
}

static void testRandomize()
{
    // namn, mappar, numrering
    CHECK(randomPresetName(1) == "Random 0001");
    CHECK(randomPresetName(9999) == "Random 9999");
    CHECK(randomFolderName(1) == "Random" && randomFolderName(2) == "Random 2");
    CHECK(randomNumberFromName("Random 0042") == 42 && randomNumberFromName("Random 42") == 0 && randomNumberFromName("Wide") == 0);
    CHECK(randomNumberFromName(randomPresetName(1234)) == 1234);

    float keep[kNumParams];
    int resOnShort = 0, filterBad = 0, tooQuiet = 0, outOfRange = 0, touched = 0, step0Rest = 0, notFinite = 0;
    int differ = 0, splitTotal = 0, resTotal = 0;
    float prev[kNumParams];
    for (uint64_t seed = 1; seed <= 3000; ++seed) {
        for (int i = 0; i < kNumParams; ++i) keep[i] = 0.123f + 0.001f * (i % 7);
        float v[kNumParams];
        for (int i = 0; i < kNumParams; ++i) v[i] = keep[i];
        randomPattern(seed, v);
        if (v[kParamMix] != keep[kParamMix] || v[kParamOutput] != keep[kParamOutput] || v[kParamHumanize] != keep[kParamHumanize] || v[kParamSampleHold] != keep[kParamSampleHold]) ++touched;
        int audible = 0, splits = 0;
        for (int i = 0; i < kNumParams; ++i) if (!std::isfinite(v[i])) ++notFinite;
        for (int s = 0; s < kNumSteps; ++s) {
            const bool sp = v[kParamSplitBase + s] >= 0.5f;
            if (sp) ++splits;
            for (int h = 0; h < (sp ? 2 : 1); ++h) {
                const float len = v[paramLen(s, h)], hp = v[paramHP(s, h)], lp = v[paramLP(s, h)];
                const float res = v[paramRes(s, h)], pan = v[paramPan(s, h)];
                for (float x : { len, hp, lp, res, pan }) if (x < 0.0f || x > 1.0f) ++outOfRange;
                if (lp - hp < 0.185f) ++filterBad;
                if (!sp && len >= 0.2f) ++audible;
                if (slotFromValue(res) >= 3 && len < 0.6f) ++resOnShort;
                if (slotFromValue(res) > 0) ++resTotal;
                if (s == 0 && h == 0 && len < 0.2f) ++step0Rest;
            }
        }
        splitTotal += splits;
        if (audible < 8 - splits) ++tooQuiet;   // delade steg räknas separat; minst 8 steg totalt hörs
        if (splits > 5) ++outOfRange;
        if (seed > 1 && std::memcmp(prev, v, sizeof v) != 0) ++differ;
        std::memcpy(prev, v, sizeof prev);
        // samma frö ger samma mönster
        float w[kNumParams];
        for (int i = 0; i < kNumParams; ++i) w[i] = keep[i];
        randomPattern(seed, w);
        CHECK(std::memcmp(v, w, sizeof v) == 0);
    }
    CHECK(touched == 0);
    CHECK(resOnShort == 0);
    CHECK(filterBad == 0);
    CHECK(tooQuiet == 0);
    CHECK(outOfRange == 0);
    CHECK(step0Rest == 0);
    CHECK(notFinite == 0);
    CHECK(differ > 2900);          // nästan alla frön ger olika mönster
    CHECK(splitTotal > 3000);      // delade steg förekommer
    CHECK(resTotal > 3000);        // resonans förekommer

    // Mönstren går att köra och blir inte tysta eller trasiga
    int silent = 0, bad = 0;
    for (uint64_t seed = 1; seed <= 60; ++seed) {
        Engine e;
        e.prepare(44100.0);
        float v[kNumParams];
        for (int i = 0; i < kNumParams; ++i) v[i] = defaultParam(i);
        randomPattern(seed * 7919, v);
        e.setParams(v);
        std::vector<float> in(512), l(512), r(512);
        double peak = 0.0;
        for (int blk = 0; blk < 200; ++blk) {
            for (int i = 0; i < 512; ++i) in[(size_t)i] = 0.5f * std::sin(0.05f * (float)(blk * 512 + i));
            e.setTransport(true, 120.0, true, true, blk * 512.0 / 44100.0 * 2.0);
            e.process(in.data(), in.data(), l.data(), r.data(), 512, false);
            for (int i = 0; i < 512; ++i) {
                if (!std::isfinite(l[(size_t)i]) || !std::isfinite(r[(size_t)i]) || std::fabs(l[(size_t)i]) > 4.0f) ++bad;
                peak = std::max(peak, (double)std::fabs(l[(size_t)i]));
            }
        }
        if (peak < 0.01) ++silent;
    }
    CHECK(bad == 0);
    CHECK(silent == 0);
}

static void testSampleHold()
{
    // holdValue: samma steg ger samma värde, olika steg olika, alltid -1..1
    int bad = 0, same = 0;
    for (long long k = -50; k < 500; ++k)
        for (int h = 0; h < 2; ++h)
            for (int w = 0; w < 2; ++w) {
                const double u = Engine::holdValue(k, h, w);
                if (!(u >= -1.0 && u <= 1.0)) ++bad;
                if (u != Engine::holdValue(k, h, w)) ++bad;
                if (u == Engine::holdValue(k + 1, h, w)) ++same;
            }
    CHECK(bad == 0 && same == 0);

    // Av (0 %) = oförändrat; på = cutoff ändras per steg men hålls konstant inom steget
    auto run = [](float sh, int blockSize, std::vector<float>& out) {
        Engine e; e.prepare(44100.0);
        float v[kNumParams];
        for (int i = 0; i < kNumParams; ++i) v[i] = defaultParam(i);
        for (int s = 0; s < kNumSteps; ++s) v[paramLen(s, 0)] = 1.0f;
        v[kParamSampleHold] = sh;
        e.setParams(v);
        const int total = 44100 * 4;
        std::vector<float> in((size_t)blockSize), l((size_t)blockSize), r((size_t)blockSize);
        out.clear();
        uint32_t rng = 12345;
        for (int pos = 0; pos < total; pos += blockSize) {
            for (int i = 0; i < blockSize; ++i) { rng = rng * 1664525u + 1013904223u; in[(size_t)i] = ((rng >> 8) / 8388608.0f - 1.0f) * 0.5f; }
            e.setTransport(true, 120.0, true, true, pos / 44100.0 * 2.0);
            e.process(in.data(), in.data(), l.data(), r.data(), blockSize, false);
            out.insert(out.end(), l.begin(), l.end());
        }
    };
    std::vector<float> off, on, on2, onBig;
    run(0.0f, 256, off); run(1.0f, 256, on); run(1.0f, 256, on2); run(1.0f, 1024, onBig);
    CHECK(on == on2);                                  // samma inställning, samma ljud
    double diff = 0.0; for (size_t i = 0; i < off.size(); ++i) diff += std::fabs(off[i] - on[i]);
    CHECK(diff > 10.0);                                // S/H hörs
    double bd = 0.0; size_t n = std::min(on.size(), onBig.size());
    for (size_t i = 0; i < n; ++i) bd = std::max(bd, (double)std::fabs(on[i] - onBig[i]));
    CHECK(bd < 1e-4);                                  // blockstorleken spelar ingen roll
    // ljudets nivå per 16-del varierar från steg till steg (slumpad cutoff)
    const int per = 44100 / 8;   // 120 bpm: 16-del = 0,125 s
    double lo = 1e9, hi = 0.0;
    for (int st = 4; st < 28; ++st) {
        double e2 = 0.0;
        for (int i = st * per + per / 2; i < (st + 1) * per; ++i) e2 += on[(size_t)i] * on[(size_t)i];
        lo = std::min(lo, e2); hi = std::max(hi, e2);
    }
    CHECK(hi > lo * 1.5);
    for (float x : on) if (!std::isfinite(x)) { CHECK(false); break; }
}

static void testSpeed()
{
    Rig r; r.allOpen();
    for (int s = 0; s < kNumSteps; ++s) { r.v[paramLP(s, 0)] = 0.4f + 0.02f * s; r.v[paramHP(s, 0)] = 0.1f; r.v[paramRes(s, 0)] = valueFromSlot(6); r.v[paramPan(s, 0)] = valueFromSlot(2); }
    const int N = (int)(kSr * 4.0);
    std::vector<float> in(N, 0.3f), oL, oR;
    const auto t0 = std::chrono::steady_clock::now();
    r.run(in, in, oL, oR, 256);
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("processing time: %.3f s for 4 s of audio (%.1f %% of real time)\n", secs, secs / 4.0 * 100.0);
    CHECK(secs < 2.0);
}

int main()
{
    testParams();
    testGate();
    testFilters();
    testResonance();
    testSplit();
    testPan();
    testMixOutput();
    testBuffers();
    testPresetsAndState();
    testGui();
    testHumanize();
    testRandomize();
    testSampleHold();
    testSpeed();
    std::printf("%d checks, %d failed\n", g_checks, g_fails);
    return g_fails ? 1 : 0;
}
