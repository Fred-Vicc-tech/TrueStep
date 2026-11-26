// HostTests.cpp - låtsas vara en VST2-värd (som Live) och testar pluginets
// VST2-del: att det anmäler sig som stereoeffekt, behandlar ljud, läser tempo,
// sparar/läser tillstånd och tål felaktiga data. Fönstret testas inte här.
#include "vst2.h"
#include "TrueStepDSP.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback host);

using namespace truestep;

static int failures = 0;
static int automateCalls = 0;

static void check(bool ok, const char* name)
{
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++failures;
}

static VstTimeInfo g_time;
static bool g_timeValid = false;

static VstIntPtr hostCallback(AEffect*, VstInt32 opcode, VstInt32, VstIntPtr, void*, float)
{
    if (opcode == audioMasterVersion) return 2400;
    if (opcode == audioMasterGetTime) return g_timeValid ? (VstIntPtr)&g_time : 0;
    if (opcode == audioMasterAutomate) ++automateCalls;
    return 0;
}

static VstIntPtr oldHost(AEffect*, VstInt32, VstInt32, VstIntPtr, void*, float) { return 0; }

// Kör ett block. Returnerar största skillnad mellan ut och in.
static double processBlock(AEffect* e, std::vector<float>& l, std::vector<float>& r, std::vector<float>& ol, std::vector<float>& orr)
{
    float* ins[2] = { l.data(), r.data() };
    float* outs[2] = { ol.data(), orr.data() };
    e->processReplacing(e, ins, outs, (VstInt32)l.size());
    double d = 0.0;
    for (size_t i = 0; i < l.size(); ++i) d = std::fmax(d, std::fmax(std::fabs(ol[i] - l[i]), std::fabs(orr[i] - r[i])));
    return d;
}

static double peakOf(const std::vector<float>& v)
{
    double p = 0.0;
    for (float x : v) p = std::fmax(p, std::fabs(x));
    return p;
}

int main()
{
    check(VSTPluginMain(nullptr) == nullptr && VSTPluginMain(oldHost) == nullptr, "ingen/for gammal vard: pluginet skapas inte");

    AEffect* e = VSTPluginMain(hostCallback);
    check(e != nullptr && e->magic == kEffectMagic, "plugin skapas");
    check((e->flags & effFlagsCanReplacing) && (e->flags & effFlagsProgramChunks),
          "anmaler sig som effekt (inte instrument), processReplacing, sparar chunk");
    check(e->numInputs == 2 && e->numOutputs == 2, "2 in, 2 ut (stereo)");
    check(e->dispatcher(e, effGetPlugCategory, 0, 0, nullptr, 0) == kPlugCategEffect, "kategori effekt");
    check(e->uniqueID == (('T' << 24) | ('r' << 16) | ('S' << 8) | 't'), "eget unikt ID (TrSt)");
    char name[64] = {};
    e->dispatcher(e, effGetEffectName, 0, 0, name, 0);
    check(std::strcmp(name, "TrueStep") == 0, "namnet TrueStep");

    e->dispatcher(e, effOpen, 0, 0, nullptr, 0);
    e->dispatcher(e, effSetSampleRate, 0, 0, nullptr, 44100.0f);
    e->dispatcher(e, effSetBlockSize, 0, 512, nullptr, 0);
    e->dispatcher(e, effMainsChanged, 0, 1, nullptr, 0);

    // Utan tempo från värden (står still): ljudet passerar oförändrat
    std::vector<float> l(512), r(512), ol(512), orr(512);
    for (int i = 0; i < 512; ++i) { l[i] = (float)std::sin(i * 0.05) * 0.5f; r[i] = (float)std::cos(i * 0.031) * 0.4f; }
    double d = 0.0;
    for (int b = 0; b < 20; ++b) d = std::fmax(d, processBlock(e, l, r, ol, orr));
    check(d < 1e-5, "ingen transport: ljudet gar igenom oforandrat (OUTPUT 0 dB)");

    // Samma buffert in och ut
    {
        std::vector<float> bl = l, br = r;
        float* io[2] = { bl.data(), br.data() };
        e->processReplacing(e, io, io, 512);
        double dd = 0.0;
        for (int i = 0; i < 512; ++i) dd = std::fmax(dd, std::fmax(std::fabs(bl[i] - l[i]), std::fabs(br[i] - r[i])));
        check(dd < 1e-5, "in- och utbuffert samma minne fungerar");
    }

    // Saknade ingångar
    {
        float* outs[2] = { ol.data(), orr.data() };
        e->processReplacing(e, nullptr, outs, 512);
        bool silent = true;
        for (int i = 0; i < 512; ++i) silent = silent && ol[i] == 0.0f && orr[i] == 0.0f;
        check(silent, "inga ingangar: tyst, ingen krasch");
    }

    // Parametrar
    char text[64] = {};
    check(e->numParams == kNumParams && kNumParams == 180, "180 parametrar");
    bool namesOk = true;
    for (int i = 0; i < kNumParams; ++i) {
        e->dispatcher(e, effGetParamName, i, 0, text, 0);
        namesOk = namesOk && std::strcmp(text, paramName(i)) == 0 && text[0] != 0;
    }
    check(namesOk, "alla parametrar har namn");
    e->dispatcher(e, effGetParamName, 0, 0, text, 0);
    const bool mixName = std::strcmp(text, paramName(kParamMix)) == 0;
    e->dispatcher(e, effGetParamName, paramLen(0, 0), 0, text, 0);
    check(mixName && std::strcmp(text, "S01 Len") == 0, "stabila namn: Mix och S01 Len");
    check(e->dispatcher(e, effCanBeAutomated, 0, 0, nullptr, 0) == 1 && e->dispatcher(e, effCanBeAutomated, 999, 0, nullptr, 0) == 0,
          "parametrarna kan automatiseras");

    e->dispatcher(e, effGetParamDisplay, paramPan(0, 0), 0, text, 0);
    check(std::strcmp(text, "C") == 0, "PAN standard visar C");
    e->setParameter(e, paramPan(0, 0), valueFromSlot(1));
    e->dispatcher(e, effGetParamDisplay, paramPan(0, 0), 0, text, 0);
    check(std::strcmp(text, "36L") == 0, "PAN-lage 1 visar 36L");
    e->setParameter(e, paramPan(0, 0), NAN);
    check(e->getParameter(e, paramPan(0, 0)) == valueFromSlot(1), "NaN fran varden ignoreras");
    e->setParameter(e, paramPan(0, 0), 3.0f);
    check(e->getParameter(e, paramPan(0, 0)) == valueFromSlot(1), "varde utanfor 0..1 ignoreras");
    e->setParameter(e, paramPan(0, 0), valueFromSlot(4));
    e->dispatcher(e, effGetParamLabel, kParamOutput, 0, text, 0);
    check(std::strcmp(text, "dB") == 0, "OUTPUT har enheten dB");

    // Tempo och position från värden: steg 1 stängt (längd 0), steg 2 öppet
    {
        std::memset(&g_time, 0, sizeof g_time);
        g_time.tempo = 120.0;
        g_time.flags = kVstTempoValid | kVstPpqPosValid | kVstTransportPlaying;
        g_timeValid = true;
        AEffect* t = VSTPluginMain(hostCallback);
        t->dispatcher(t, effSetSampleRate, 0, 0, nullptr, 44100.0f);
        t->dispatcher(t, effMainsChanged, 0, 1, nullptr, 0);
        t->setParameter(t, paramLen(0, 0), 0.0f);
        std::vector<float> one(512, 0.5f), a(512), b(512);
        // Steg 1 = ppq 0..0,25; steg 2 = 0,25..0,5
        g_time.ppqPos = 0.10;
        for (int k = 0; k < 30; ++k) processBlock(t, one, one, a, b);
        const double closed = peakOf(a);
        g_time.ppqPos = 0.30;
        for (int k = 0; k < 30; ++k) processBlock(t, one, one, a, b);
        const double open = peakOf(a);
        check(closed < 0.02 && open > 0.3, "stegen foljer Lives position (ppq): steg 1 stangt, steg 2 oppet");
        // ppq i nästa takt (16 steg = 4 kvartsnoter): samma steg 1
        g_time.ppqPos = 4.10;
        for (int k = 0; k < 30; ++k) processBlock(t, one, one, a, b);
        check(peakOf(a) < 0.02, "monstret upprepas varje takt");
        // Transporten stoppad: passerar oförändrat
        g_time.flags = kVstTempoValid | kVstPpqPosValid;
        double dd = 0.0;
        for (int k = 0; k < 30; ++k) dd = processBlock(t, one, one, a, b);
        check(dd < 1e-5, "Live stoppad: ljudet passerar");
        t->dispatcher(t, effClose, 0, 0, nullptr, 0);
        g_timeValid = false;
    }
    check(e->dispatcher(e, effCanDo, 0, 0, (void*)"receiveVstTimeInfo", 0) == 1 &&
          e->dispatcher(e, effCanDo, 0, 0, (void*)"receiveVstMidiEvent", 0) == 0, "canDo: tidsinfo ja, MIDI nej");

    // Spara och läsa tillstånd (som när Live sparar och öppnar ett projekt)
    e->setParameter(e, kParamMix, 0.8f);
    e->setParameter(e, paramLP(5, 1), 0.4f);
    e->setParameter(e, kParamOutput, outputNormFromDb(-6.0));
    void* data = nullptr;
    VstIntPtr size = e->dispatcher(e, effGetChunk, 0, 0, &data, 0);
    check(size > 0 && data != nullptr && (size_t)size == 48 + kNumParams * 4, "tillstand kan sparas (rimlig storlek)");
    std::vector<unsigned char> saved((unsigned char*)data, (unsigned char*)data + size);

    AEffect* e2 = VSTPluginMain(hostCallback);
    check(e2->dispatcher(e2, effSetChunk, 0, (VstIntPtr)saved.size(), saved.data(), 0) == 1, "tillstand kan lasas");
    check(e2->getParameter(e2, paramPan(0, 0)) == valueFromSlot(4) && e2->getParameter(e2, kParamMix) == 0.8f &&
          e2->getParameter(e2, paramLP(5, 1)) == 0.4f && e2->getParameter(e2, kParamOutput) == outputNormFromDb(-6.0),
          "alla parametrar aterstalls efter oppning");

    // Kortare chunk: resten får standardvärden, namnet behålls
    {
        std::vector<unsigned char> shortChunk(48 + 2 * 4, 0);
        std::memcpy(shortChunk.data(), "TSst", 4);
        const uint32_t ver = 1, cnt = 2; const int32_t prog = -1;
        std::memcpy(shortChunk.data() + 4, &ver, 4); std::memcpy(shortChunk.data() + 8, &cnt, 4); std::memcpy(shortChunk.data() + 12, &prog, 4);
        std::memcpy(shortChunk.data() + 16, "Mine", 4);
        const float v[2] = { 0.1f, 0.9f };
        std::memcpy(shortChunk.data() + 48, v, sizeof v);
        e2->dispatcher(e2, effSetChunk, 0, (VstIntPtr)shortChunk.size(), shortChunk.data(), 0);
        e2->dispatcher(e2, effGetProgramName, 0, 0, text, 0);
        check(e2->getParameter(e2, 0) == 0.1f && e2->getParameter(e2, 1) == 0.9f
              && e2->getParameter(e2, kParamOutput) == defaultParam(kParamOutput) && std::strcmp(text, "Mine") == 0,
              "chunk med farre parametrar: resten far standardvarden, namnet behalls");
    }

    unsigned char junk[7] = { 1, 2, 3, 4, 5, 6, 7 };
    check(e2->dispatcher(e2, effSetChunk, 0, sizeof junk, junk, 0) == 0, "trasig data avvisas utan krasch");
    {
        std::vector<unsigned char> bad(52, 0);
        std::memcpy(bad.data(), "TSst", 4);
        const uint32_t ver = 1, cnt = 500;
        std::memcpy(bad.data() + 4, &ver, 4); std::memcpy(bad.data() + 8, &cnt, 4);
        std::memset(bad.data() + 16, 0xFF, 32);     // namn utan nolla, konstiga tecken
        const float nanv = NAN;
        std::memcpy(bad.data() + 48, &nanv, 4);
        check(e2->dispatcher(e2, effSetChunk, 0, (VstIntPtr)bad.size(), bad.data(), 0) == 1 &&
              std::isfinite(e2->getParameter(e2, 0)), "felaktigt antal parametrar, NaN och trasigt namn hanteras");
    }

    // Fabrikspresets som program
    {
        AEffect* q = VSTPluginMain(hostCallback);
        check(q->numPrograms == kNumFactoryPresets && kNumFactoryPresets == 12, "12 fabrikspresets syns som program i Live");
        char nm[64] = {};
        bool ok = true;
        for (int i = 0; i < q->numPrograms; ++i)
            ok = ok && q->dispatcher(q, effGetProgramNameIndexed, i, 0, nm, 0) == 1 && std::strcmp(nm, factoryPresetName(i)) == 0;
        check(ok, "alla fabrikspresets har namn");
        q->dispatcher(q, effSetProgram, 0, 2, nullptr, 0);
        q->dispatcher(q, effGetProgramName, 0, 0, nm, 0);
        float v[kNumParams];
        factoryPresetValues(2, v);
        bool same = true;
        for (int i = 0; i < kNumParams; ++i) same = same && q->getParameter(q, i) == v[i];
        check(std::strcmp(nm, factoryPresetName(2)) == 0 && same && q->dispatcher(q, effGetProgram, 0, 0, nullptr, 0) == 2,
              "program 2 laddas med alla varden");
        q->dispatcher(q, effSetProgram, 0, 99, nullptr, 0);
        check(q->dispatcher(q, effGetProgram, 0, 0, nullptr, 0) == 2, "ogiltigt programnummer ignoreras");
        q->dispatcher(q, effClose, 0, 0, nullptr, 0);
    }

    e->dispatcher(e, effClose, 0, 0, nullptr, 0);
    e2->dispatcher(e2, effClose, 0, 0, nullptr, 0);

    std::printf("\n%s: %d fel\n", failures ? "MISSLYCKADES" : "ALLA VARDTESTER OK", failures);
    return failures ? 1 : 0;
}
