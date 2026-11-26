// TrueStepCore.h - det som kopplar ihop ljudmotorn med värdprogrammet:
// parametrar (atomiska, delas mellan ljudtråd och fönster), ljudbehandling per
// block, sparat tillstånd (chunk) och fabrikspresets. Värdprogrammet kopplas in
// via de virtuella host*-funktionerna (samma upplägg som TruePan).
#pragma once

#include "TrueStepDSP.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
  #include <xmmintrin.h>
  #define TRUESTEP_HAS_SSE 1
#else
  #define TRUESTEP_HAS_SSE 0
#endif

namespace truestep {

const int kEditorWidth = 900, kEditorHeight = 640;
const uint32_t kStateVersion = 1;

inline void copyString(char* dst, const char* src, size_t maxLen)
{
    size_t i = 0;
    for (; i + 1 < maxLen && src[i]; ++i) dst[i] = src[i];
    dst[i] = 0;
}

// Sparat tillstånd ("chunk") som värdprogrammet lagrar i projektet. Har ett
// versionsnummer så att framtida versioner kan läsa gamla projekt.
#pragma pack(push, 1)
struct StateChunk
{
    char magic[4];          // "TSst"
    uint32_t version;       // 1
    uint32_t numParams;
    int32_t program;        // vald fabrikspreset (-1 = egen)
    char name[32];          // presetnamn
    float values[kNumParams];
};
#pragma pack(pop)
const size_t kChunkHeader = 48;

// Tempo och position från värdprogrammet
struct TransportInfo
{
    bool tempoValid = false, playing = false, ppqValid = false;
    double tempo = 120.0, ppq = 0.0;
};

struct Core
{
    Engine engine;
    std::atomic<float> params[kNumParams];

    // För gränssnittet
    std::atomic<int> playPos{ -1 };            // steg * 2 + halva som spelas nu, -1 = står still
    std::atomic<float> peakL{ 0.0f };          // högsta utnivå sedan fönstret läste
    std::atomic<float> peakR{ 0.0f };

    StateChunk chunk;
    int program = 0;
    char presetName[32] = "Default Gate";
    // Tid (ms) då fönstret senast bytte/sparade preset. Vissa värdar (Live) ber om program 0 strax
    // efter att pluginet meddelat att presetnamnet ändrats; det ska inte skriva över ljudet.
    std::atomic<int64_t> editorPresetStamp{ -100000 };
    static int64_t nowMs()
    {
        return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    // Likadant när ett sparat projekt öppnas: Live ber om program 0 efter effSetChunk. Då ska det
    // laddade ljudet (en egen preset, program -1) behållas, inte bytas mot Default Gate.
    std::atomic<int64_t> stateLoadStamp{ -100000000 };
    void markEditorPreset() { editorPresetStamp.store(nowMs()); }
    void markStateLoaded() { stateLoadStamp.store(nowMs()); }
    bool stateLoadRecent() const { return nowMs() - stateLoadStamp.load() < 15000; }
    bool editorPresetRecent() const { return nowMs() - editorPresetStamp.load() < 3000; }
    void* editor = nullptr;                    // fönstret (TrueStepEditor.cpp), om öppet

    Core() { for (int i = 0; i < kNumParams; ++i) params[i].store(defaultParam(i)); }
    virtual ~Core() {}

    // --- Kopplingar till värdprogrammet ---
    virtual void hostAutomate(int /*index*/, float /*value*/) {}   // parameter ändrad i fönstret
    virtual void hostBeginEdit(int /*index*/) {}
    virtual void hostEndEdit(int /*index*/) {}
    virtual void hostUpdateDisplay() {}                            // presetnamn m.m. ändrat

    void setParam(int index, float value)
    {
        if (index < 0 || index >= kNumParams) return;
        if (!(value >= 0.0f && value <= 1.0f)) return;   // NaN / utanför område
        params[index].store(value);
    }

    // Används när gränssnittet ändrar en parameter: meddelar även värden så att
    // ändringen kan spelas in som automation.
    void setParamFromEditor(int index, float value)
    {
        setParam(index, value);
        if (index >= 0 && index < kNumParams) hostAutomate(index, params[index].load());
    }
    void beginEdit(int index) { hostBeginEdit(index); }
    void endEdit(int index) { hostEndEdit(index); }

    void applyValues(const float* v, bool fromEditor)
    {
        for (int i = 0; i < kNumParams; ++i) {
            if (fromEditor) { beginEdit(i); setParamFromEditor(i, v[i]); endEdit(i); }
            else setParam(i, v[i]);
        }
    }

    void loadFactoryPreset(int index, bool fromEditor)
    {
        if (index < 0 || index >= kNumFactoryPresets) return;
        float v[kNumParams];
        factoryPresetValues(index, v);
        applyValues(v, fromEditor);
        program = index;
        copyString(presetName, factoryPresetName(index), sizeof presetName);
    }

    // --- Ljudbehandling (ljudtråden): ingen allokering, inga lås ---
    static void storeMax(std::atomic<float>& a, float v)
    {
        if (v > a.load()) a.store(v);
    }

    void processAudio(const float* inL, const float* inR, float* outL, float* outR, int n, bool accumulate,
                      const TransportInfo& t)
    {
        if (!outL || n <= 0) return;
#if TRUESTEP_HAS_SSE
        // Stäng av denormala tal (kan annars göra processorn mycket långsam).
        const unsigned oldCsr = _mm_getcsr();
        _mm_setcsr(oldCsr | 0x8040);   // FTZ + DAZ
#endif
        float v[kNumParams];
        for (int i = 0; i < kNumParams; ++i) v[i] = params[i].load();
        engine.setParams(v);
        engine.setTransport(t.tempoValid, t.tempo, t.playing, t.ppqValid, t.ppq);
        engine.process(inL, inR, outL, outR, n, accumulate);

        playPos.store(engine.displayPos());
        storeMax(peakL, engine.peakL());
        storeMax(peakR, engine.peakR());
#if TRUESTEP_HAS_SSE
        _mm_setcsr(oldCsr);
#endif
    }

    // --- Sparat tillstånd ---
    const StateChunk& saveState()
    {
        std::memset(&chunk, 0, sizeof chunk);
        std::memcpy(chunk.magic, "TSst", 4);
        chunk.version = kStateVersion;
        chunk.numParams = kNumParams;
        chunk.program = program;
        copyString(chunk.name, presetName, sizeof chunk.name);
        for (int i = 0; i < kNumParams; ++i) chunk.values[i] = params[i].load();
        return chunk;
    }

    bool loadState(const void* ptr, size_t size)
    {
        if (!ptr || size < kChunkHeader) return false;
        const unsigned char* data = static_cast<const unsigned char*>(ptr);
        if (std::memcmp(data, "TSst", 4) != 0) return false;
        uint32_t version = 0, count = 0;
        std::memcpy(&version, data + 4, 4);
        std::memcpy(&count, data + 8, 4);
        if (version < 1) return false;
        int32_t prog = 0;
        std::memcpy(&prog, data + 12, 4);
        program = (prog >= -1 && prog < kNumFactoryPresets) ? prog : -1;
        char nm[33];
        std::memcpy(nm, data + 16, 32);
        nm[32] = 0;
        copyString(presetName, cleanPresetName(nm).c_str(), sizeof presetName);
        // Börja från standardvärden: parametrar som saknas får standardvärden.
        for (int i = 0; i < kNumParams; ++i) setParam(i, defaultParam(i));
        const size_t available = (size - kChunkHeader) / sizeof(float);
        if (count > available) count = (uint32_t)available;
        for (uint32_t i = 0; i < count && i < (uint32_t)kNumParams; ++i) {
            float v;
            std::memcpy(&v, data + kChunkHeader + i * sizeof(float), sizeof(float));
            setParam((int)i, v);   // ogiltiga värden ignoreras
        }
        return true;
    }
};

// Fönstret (Windows). Öppnas i ett föräldrafönster från värdprogrammet.
void editorOpen(Core* core, void* parentWindow);
void editorClose(Core* core);
// Ritar fönstret till en PNG-fil (för bilder av gränssnittet)
bool editorRenderPng(Core* core, bool backSide, const wchar_t* path);

} // namespace truestep
