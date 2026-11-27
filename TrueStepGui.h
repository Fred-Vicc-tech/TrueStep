// TrueStepGui.h - TrueSteps gränssnitt: layout, ritning, mus och manual (baksidan).
// Ritas via "Canvas" (en liten uppsättning ritkommandon), så att all logik är
// plattformsoberoende och kan testas på alla datorer. Windows-delen
// (TrueStepEditor.cpp) ritar Canvas med GDI+ och sköter fönstret.
// Stilen följer TruePan och TrueSub: mörk metall, cyan detaljer, orange som
// andra färg, rattar med kuggkant och mörka rundade rutor.
#pragma once

#include "Manual.h"
#include "TrueStepDSP.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace truestep {

typedef uint32_t Rgba;
constexpr Rgba argb(unsigned a, unsigned r, unsigned g, unsigned b) { return (a << 24) | (r << 16) | (g << 8) | b; }
const Rgba kNone = 0;   // ingen fyllning / ingen kant

enum { kStyleRegular = 0, kStyleBold = 1, kStyleItalic = 2, kStyleBoldItalic = 3 };
enum { kAlignNear = 0, kAlignCenter = 1, kAlignFar = 2 };
enum { kCurArrow = 0, kCurHand = 1, kCurSizeWE = 2, kCurSizeNS = 3, kCurSizeAll = 4 };

static const char* const kVersionText = "1.0";

// Ritkommandon. Koordinater i bildpunkter, text i UTF-8.
struct Canvas
{
    virtual ~Canvas() {}
    virtual void fillRect(float x, float y, float w, float h, Rgba c) = 0;
    virtual void gradientV(float x, float y, float w, float h, Rgba top, Rgba bottom) = 0;
    virtual void line(float x0, float y0, float x1, float y1, Rgba c, float width) = 0;
    virtual void roundRect(float x, float y, float w, float h, float r, Rgba fill, Rgba edge, float edgeW) = 0;
    virtual void ellipse(float cx, float cy, float rx, float ry, Rgba fill, Rgba edge, float edgeW) = 0;
    virtual void arc(float cx, float cy, float r, float startDeg, float sweepDeg, Rgba c, float width) = 0;
    virtual void polygon(const float* xy, int n, Rgba fill) = 0;
    // Text: x,y = övre vänstra hörnet av raden, w = radbredd (för justering och radbrytning)
    virtual void text(float x, float y, float w, const char* s, float size, int style, Rgba c, int align) = 0;
    virtual float textHeight(const char* s, float w, float size, int style) = 0;   // höjd med radbrytning
    virtual float textWidth(const char* s, float size, int style) = 0;             // bredd på en rad
    virtual void clip(float x, float y, float w, float h) = 0;
    virtual void unclip() = 0;
};

// Gränssnittets koppling till pluginet
struct GuiHost
{
    virtual ~GuiHost() {}
    virtual float get(int index) = 0;
    virtual void set(int index, float value) = 0;       // ändring från fönstret (meddelar värden)
    virtual void beginEdit(int /*index*/) {}
    virtual void endEdit(int /*index*/) {}
    virtual const char* presetName() { return ""; }
    virtual int language() { return 0; }                 // 0 = ENG, 1 = SWE (manualen)
    virtual void setLanguage(int) {}
    virtual bool infoOn() { return true; }               // infotext-rutan högst upp
    virtual void setInfoOn(bool) {}
    virtual int bankNumber() { return 1; }               // vald Random-bank (1, 2, 3 ...)
    virtual bool bankActive() { return false; }          // PRESET-pilarna bläddrar i banken
    virtual int playPos() { return -1; }                // steg * 2 + halva, -1 = står still
    virtual void takePeaks(float& l, float& r) { l = 0.0f; r = 0.0f; }   // högsta nivå sedan sist
};

namespace theme {
// Färger som i TruePan/TrueSub: mörk metall, cyan detaljer, orange som andra färg.
const Rgba bgTop      = argb(255, 46, 49, 54);
const Rgba bgBottom   = argb(255, 22, 24, 27);
const Rgba panel      = argb(255, 14, 15, 17);
const Rgba panelEdge  = argb(255, 70, 74, 80);
const Rgba cyan       = argb(255, 0, 220, 255);
const Rgba orange     = argb(255, 255, 160, 40);
const Rgba text       = argb(255, 225, 230, 235);
const Rgba textDim    = argb(255, 140, 146, 152);
const Rgba grid       = argb(255, 45, 49, 54);
const Rgba box        = argb(255, 8, 9, 10);
const Rgba btnOff     = argb(255, 30, 33, 37);
const Rgba btnOn      = argb(255, 10, 60, 80);
const Rgba cellA      = argb(255, 20, 22, 25);
const Rgba cellB      = argb(255, 27, 30, 34);
const Rgba playhead   = argb(70, 255, 160, 40);
const Rgba barFill    = argb(255, 8, 97, 112);
const Rgba barEdgeL   = argb(255, 140, 146, 152);   // stapelns vänsterkant
const Rgba barHandle  = argb(255, 190, 245, 255);
const Rgba barFillOn  = argb(255, 120, 74, 8);
const Rgba barHandleOn = argb(255, 255, 226, 170);
const Rgba selBg      = argb(255, 8, 9, 10);
const Rgba panOn      = argb(255, 255, 160, 40);
const Rgba panOff     = argb(255, 110, 73, 26);
const Rgba resOn      = argb(255, 0, 220, 255);
const Rgba resOff     = argb(255, 8, 97, 112);
const Rgba red        = argb(255, 255, 60, 60);
const Rgba redOff     = argb(255, 70, 74, 80);
} // namespace theme

// ---------------------------------------------------------------------------
// Ritdelar: panel, ruta, knapp, ratt
// ---------------------------------------------------------------------------
inline void drawPanel(Canvas& c, float x, float y, float w, float h)
{
    c.roundRect(x, y, w, h, 12, theme::panel, theme::panelEdge, 1.0f);
}

inline void drawBox(Canvas& c, float x, float y, float w, float h)
{
    c.roundRect(x, y, w, h, 6, theme::box, theme::panelEdge, 1.0f);
}

struct Rect
{
    float x, y, w, h;
    bool hit(int px, int py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};

inline void drawButton(Canvas& c, const Rect& r, const char* label, bool active, Rgba activeColor, float fontSize = 0.0f)
{
    c.roundRect(r.x, r.y, r.w, r.h, 6, active ? theme::btnOn : theme::btnOff, active ? activeColor : theme::panelEdge,
                active ? 2.0f : 1.0f);
    const float size = fontSize > 0.0f ? fontSize : (r.h >= 34.0f ? 15.0f : 14.0f);
    // Pilarna "<" och ">" sitter lågt i typsnittet: lyft dem 1,5 px.
    const bool arrow = label[0] != 0 && label[1] == 0 && (label[0] == '<' || label[0] == '>');
    const float dy = arrow ? -1.5f : 0.0f;
    c.text(r.x, r.y + (r.h - size * 1.35f) / 2.0f + dy, r.w, label, size, kStyleBold, active ? activeColor : theme::textDim,
           kAlignCenter);
}

// Ratt med kuggkant (samma som i TruePan). Cyan båge för värdet (0..1).
inline void drawKnob(Canvas& c, float cx, float cy, float r, float v)
{
    const float start = 135.0f, sweep = 270.0f;
    const float arcW = 6.0f;
    const float ar = r + 9.0f;
    c.arc(cx, cy, ar, start, sweep, argb(255, 40, 43, 47), arcW);
    if (v > 0.001f) c.arc(cx, cy, ar, start, sweep * v, theme::cyan, arcW);

    const float kPiF = 3.14159265f;
    const float angle = (start + sweep * v) * kPiF / 180.0f;

    for (int i = 0; i < 4; ++i) {
        const float rr = r + 2.5f - i * 1.0f;
        c.ellipse(cx + 1.5f, cy + 3.0f, rr, rr, argb(40 + i * 25, 0, 0, 0), kNone, 0.0f);
    }

    const float kRefR = 36.0f;
    const float kDepth = (0.99f - 0.90f) * kRefR;
    const float tipRef = kRefR * (0.99f - 2.5f / 82.0f);
    const float kPitch = 2.0f * kPiF * tipRef / 20.0f;
    const float kGap = tipRef - kDepth - (0.66f * kRefR + kRefR * 4.4f / 82.0f);
    const float tip = r * (0.99f - 2.5f / 82.0f);
    const float root = tip - kDepth;
    int n = (int)std::floor(2.0f * kPiF * tip / kPitch + 0.5f);
    if (n < 8) n = 8;
    const float ringOut = root - kGap;
    const float ringIn = ringOut - 3.0f;

    c.ellipse(cx, cy, r, r, argb(255, 14, 14, 16), kNone, 0.0f);

    const int steps = n * 24;
    const int m = steps <= 64 * 24 ? steps : 64 * 24;
    static thread_local float pts[2 * 64 * 24];
    const float rot = angle - kPiF / n;
    for (int i = 0; i < m; ++i) {
        const float a = 2.0f * kPiF * i / m;
        float u = (a - rot) * n / (2.0f * kPiF);
        u -= std::floor(u);
        const float x = std::fabs(u - 0.5f) * 2.0f;
        float t = (x - 0.25f) / 0.5f;
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        t = t * t * (3.0f - 2.0f * t);
        const float rr = tip - (tip - root) * t;
        pts[2 * i] = cx + rr * std::cos(a);
        pts[2 * i + 1] = cy + rr * std::sin(a);
    }
    c.polygon(pts, m, argb(255, 115, 127, 139));

    c.ellipse(cx, cy, ringOut, ringOut, argb(255, 26, 34, 43), kNone, 0.0f);
    c.ellipse(cx, cy, ringIn, ringIn, argb(255, 109, 122, 134), kNone, 0.0f);

    float w = r * 2.2f / 82.0f * 1.6f;
    if (w < 1.8f) w = 1.8f;
    const float r0 = ringOut + r * 0.4f / 82.0f, r1 = tip - r * 3.6f / 82.0f;
    c.line(cx + std::cos(angle) * r0, cy + std::sin(angle) * r0, cx + std::cos(angle) * r1, cy + std::sin(angle) * r1,
           argb(255, 250, 250, 250), w);
}

// ---------------------------------------------------------------------------
// Själva gränssnittet
// ---------------------------------------------------------------------------
// Typografi för manualen: inga radbrytningar mellan tal och enhet ("0 dB", "20 kHz", "50 %"),
// inte mitt i "32-delar", och korta ord (1-2 bokstäver, samt första ordet i en mening upp till 3 bokstäver)
// hålls ihop med nästa ord så att de aldrig blir ensamma på en rad. Ersätter mellanslag med fast
// mellanslag (UTF-8 C2 A0). Bindestreck lämnas som de är: radbrytning görs av wrapText, som bara bryter vid vanliga
// mellanslag (aldrig efter ett bindestreck och aldrig i ett ord som "32-delar").
inline std::string typeset(const char* in)
{
    const std::string s = in;
    static const char* const units[] = { "dBFS", "dB", "kHz", "Hz", "ms", "%" };
    const char* nb = "\xC2\xA0";
    std::string out;
    const size_t n = s.size();
    auto isLetter = [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 0x80; };
    for (size_t i = 0; i < n; ++i) {
        const char c = s[i];
        if (c == ' ' && i > 0 && i + 1 < n) {
            bool bind = false;
            // tal + enhet
            if (s[i - 1] >= '0' && s[i - 1] <= '9') {
                for (const char* u : units) {
                    const size_t L = std::strlen(u);
                    if (s.compare(i + 1, L, u) == 0 && (i + 1 + L >= n || !isLetter((unsigned char)s[i + 1 + L]))) bind = true;
                }
            }
            // ord + tal hålls ihop ("Random 0001", "bank 2")
            if (isLetter((unsigned char)s[i - 1]) && s[i + 1] >= '0' && s[i + 1] <= '9') bind = true;
            // kort ord före mellanslaget
            size_t b = i;
            while (b > 0 && isLetter((unsigned char)s[b - 1])) --b;
            const size_t wlen = i - b;
            if (wlen >= 1 && wlen <= 2) bind = true;
            if (wlen == 3) {   // första ordet i en mening: "Ett", "The"
                size_t p = b;
                while (p > 0 && s[p - 1] == ' ') --p;
                if (p == 0 || s[p - 1] == '.' || s[p - 1] == ':') bind = true;
            }
            if (bind) { out += nb; continue; }
        }
        out += c;
    }
    return out;
}

// Infotexter (rutan högst upp när musen vilar över något). Max ca 110 tecken så att de ryms på två rader.
enum Tip {
    kTipNone = 0, kTipLogo, kTipMeter, kTipGridTitle, kTipStepNum, kTipGrid, kTipLen, kTipLP, kTipHP, kTipSplit,
    kTipPan, kTipRes, kTipDryWet, kTipOutput, kTipOver, kTipSH, kTipHuman, kTipRandom, kTipBank, kTipPreset,
    kTipSave, kTipLoad, kTipStepPanel, kTipBack, kTipLang, kTipInfo, kTipCount
};

inline const char* tipText(int id, int lang)
{
    static const char* const en[kTipCount] = {
        "",
        "Click the logo to see the credits and the manual.",
        "Lights up when sound is passing through TrueStep.",
        "STEP GRID: one bar in 16 steps. Drag the bars to shape the sound of each step.",
        "The number of the step in the bar. The step playing right now lights up in orange.",
        "Drag the bar to move both filters. Double-click the column to reset the step.",
        "LENGTH: how long the gate stays open in this step. Drag the right edge.",
        "LOWPASS: everything above the top edge is filtered away. Drag the top edge.",
        "HIGHPASS: everything below the bottom edge is filtered away. Drag the bottom edge.",
        "Splits the step in two 32nd notes, each with its own settings. Click again to join them.",
        "PAN: stereo position of the step, from 50L to 50R. Click a bar to choose.",
        "RES: resonance (0 to 8) at the lowpass edge. It is heard best on longer steps.",
        "DRY/WET: mixes the original sound (0 %) with the processed sound (100 %).",
        "OUTPUT: level after the effect, -24 to +12 dB. Double-click to reset to 0 dB.",
        "Lights up red if the output goes over 0 dBFS.",
        "S/H: sample and hold. A new random cutoff for every step, held until the next one.",
        "HUMANIZE: moves each step up to 25 ms, following the natural timing drift of musicians (Hennig et al., 2011).",
        "RANDOMIZE: invents a new pattern and saves it as Random 0001, 0002 ... in the Random folder.",
        "Browse your saved Random patterns, bank by bank. Press again for the next bank.",
        "PRESET: the arrows step through the factory presets and then your own. SAVE and LOAD handle files.",
        "SAVE: stores your sound as a preset file (.tsp). You choose the name and the folder.",
        "LOAD: opens a preset file (.tsp).",
        "STEP: the settings of the step you last changed.",
        "Back to the plugin.",
        "Language of the manual and of these info texts.",
        "Shows or hides this info box when you point at things."
    };
    static const char* const sv[kTipCount] = {
        "",
        "Klicka på loggan för credits och manual.",
        "Lyser när ljud passerar genom TrueStep.",
        "STEP GRID: en takt i 16 steg. Dra i staplarna för att forma ljudet i varje steg.",
        "Stegets nummer i takten. Steget som spelas just nu lyser orange.",
        "Dra i staplen för att flytta båda filtren. Dubbelklicka på kolumnen för att nollställa steget.",
        "LENGTH: hur länge gaten är öppen i steget. Dra i högerkanten.",
        "LOWPASS: allt ovanför överkanten filtreras bort. Dra i överkanten.",
        "HIGHPASS: allt under underkanten filtreras bort. Dra i underkanten.",
        "Delar steget i två 32-delar med egna inställningar. Klicka igen för att slå ihop dem.",
        "PAN: stegets stereoposition, från 50L till 50R. Klicka på en stapel för att välja.",
        "RES: resonans (0 till 8) vid lowpass-kanten. Hörs bäst på längre steg.",
        "DRY/WET: blandar originalljudet (0 %) med det bearbetade ljudet (100 %).",
        "OUTPUT: nivån efter effekten, -24 till +12 dB. Dubbelklicka för att nollställa till 0 dB.",
        "Lyser rött om utnivån går över 0 dBFS.",
        "S/H: sample and hold. En ny slumpad cutoff för varje steg, som hålls till nästa.",
        "HUMANIZE: flyttar varje steg upp till 25 ms, efter musikers naturliga tidsdrift (Hennig m.fl., 2011).",
        "RANDOMIZE: hittar på ett nytt mönster och sparar det som Random 0001, 0002 ... i Random-mappen.",
        "Bläddra bland dina sparade Random-mönster, bank för bank. Tryck igen för nästa bank.",
        "PRESET: pilarna bläddrar bland fabrikspresets och sedan dina egna. SAVE och LOAD hanterar filer.",
        "SAVE: sparar ljudet som en presetfil (.tsp). Du väljer namn och mapp.",
        "LOAD: öppnar en presetfil (.tsp).",
        "STEP: inställningarna för steget du senast ändrade.",
        "Tillbaka till pluginet.",
        "Språk för manualen och för de här infotexterna.",
        "Visar eller döljer infotexten när du pekar på något."
    };
    if (id < 0 || id >= kTipCount) return "";
    return lang == kLangSwe ? sv[id] : en[id];
}

// Egen radbrytning: bryter bara vid vanliga mellanslag (U+0020). Fasta mellanslag (U+00A0) och bindestreck
// hålls alltid ihop med ordet. Mäter hela raden med Canvas::textWidth, så inga extra marginaler läggs till.
inline std::vector<std::string> wrapText(Canvas& c, const std::string& txt, float maxW, float size, int style)
{
    std::vector<std::string> lines;
    std::string line;
    size_t pos = 0;
    while (pos <= txt.size()) {
        size_t sp = txt.find(' ', pos);
        if (sp == std::string::npos) sp = txt.size();
        const std::string word = txt.substr(pos, sp - pos);
        pos = sp + 1;
        if (word.empty() && sp >= txt.size()) break;
        if (line.empty()) { line = word; continue; }
        const std::string cand = line + " " + word;
        if (c.textWidth(cand.c_str(), size, style) <= maxW) line = cand;
        else { lines.push_back(line); line = word; }
    }
    if (!line.empty() || lines.empty()) lines.push_back(line);
    return lines;
}

class StepGui
{
public:
    enum { W = 900, H = 640 };
    enum Action { kActNone, kActPresetPrev, kActPresetNext, kActSave, kActLoad, kActRandom, kActBank };

    bool showBack = false;

    explicit StepGui(GuiHost* h) : host_(h)
    {
        std::memset(editParams_, 0, sizeof editParams_);
    }

    // --- Ritning ---------------------------------------------------------
    void draw(Canvas& c, uint32_t nowMs)
    {
        // Bakgrund: mörk borstad metall
        c.gradientV(0, 0, (float)W, (float)H, theme::bgTop, theme::bgBottom);
        for (int y = 0; y < H; y += 3) c.line(0, (float)y, (float)W, (float)y, argb(10, 255, 255, 255), 1.0f);

        // Nivåmätare: högsta värdet sedan förra bilden, faller mjukt
        float pkL = 0.0f, pkR = 0.0f;
        host_->takePeaks(pkL, pkR);
        meterL_ = pkL > meterL_ ? pkL : meterL_ * 0.82f;
        meterR_ = pkR > meterR_ ? pkR : meterR_ * 0.82f;
        if (pkL > 1.0f || pkR > 1.0f) overUntil_ = nowMs + 600;
        const bool signal = meterL_ > 0.001f || meterR_ > 0.001f;
        const bool over = (int32_t)(overUntil_ - nowMs) > 0;

        // --- Rubrik ---
        {
            const float wTrue = c.textWidth("True", 38, kStyleBoldItalic);
            const float wStep = c.textWidth("Step", 38, kStyleBoldItalic);
            c.text(21, 7, 300, "True", 38, kStyleBoldItalic, theme::text, kAlignNear);
            c.text(21 + wTrue, 7, 300, "Step", 38, kStyleBoldItalic, theme::cyan, kAlignNear);
            logoRight_ = 21.0f + wTrue + wStep + 6.0f;     // loggan är klickbar (baksidan)
        }
        c.text(21, 55, 320, "Rhythmic Step Filter Gate", 15, kStyleItalic, theme::text, kAlignNear);
        {
            char v[48];
            std::snprintf(v, sizeof v, "TrueStep  v%s", kVersionText);
            c.text(555, 32, 300, v, 15, kStyleRegular, theme::text, kAlignFar);
        }
        c.ellipse(869.0f, 42.0f, 7.0f, 7.0f, signal ? theme::cyan : argb(255, 50, 54, 58), kNone, 0.0f);

        if (showBack) drawBack(c); else drawFront(c, over);
        drawTip(c, nowMs);
    }

    // --- Mus -------------------------------------------------------------
    bool overLogo(int x, int y) const { return x >= 15 && x <= logoRight_ && y >= 4 && y <= 52; }

    Action mouseDown(int x, int y, bool dbl)
    {
        tipId_ = 0; tipPending_ = false;
        endEdits();
        drag_ = kDragNone;

        if (overLogo(x, y) || (showBack && btnBack_.hit(x, y))) {   // loggan vänder pluginet
            showBack = !showBack;
            scroll_ = 0.0f;
            return kActNone;
        }
        if (showBack) { backMouseDown(x, y); return kActNone; }

        if (presetPrev_.hit(x, y)) return kActPresetPrev;
        if (presetNext_.hit(x, y)) return kActPresetNext;
        if (btnSave_.hit(x, y)) return kActSave;
        if (btnLoad_.hit(x, y)) return kActLoad;
        if (btnRandom_.hit(x, y)) return kActRandom;
        if (btnBank_.hit(x, y)) return kActBank;

        // rattar
        for (int k = 0; k < 4; ++k) {
            const KnobDef& kd = knobs_[k];
            const float dx = x - kd.cx, dy = y - kd.cy, rr = kd.r + 12.0f;
            if (dx * dx + dy * dy > rr * rr) continue;
            if (dbl) { edit(kd.param, defaultParam(kd.param)); endEdits(); return kActNone; }
            drag_ = kDragKnob;
            knob_ = k;
            startY_ = y;
            startVal_ = host_->get(kd.param);
            beginOnly(kd.param);
            return kActNone;
        }

        // 1/32-knappar
        if (y >= kSplitY && y < kSplitY + kSplitH && x >= kLeft && x < kLeft + kNumSteps * kCellW) {
            toggleSplit((int)((x - kLeft) / kCellW));
            endEdits();
            return kActNone;
        }

        // PAN / RES (9 lägen)
        {
            int s, h; bool isPan;
            if (hitSelector(x, y, s, h, isPan)) {
                setInfo(s, h);
                if (dbl) {
                    edit(isPan ? paramPan(s, h) : paramRes(s, h), isPan ? valueFromSlot(4) : 0.0f);
                    endEdits();
                    return kActNone;
                }
                aStep_ = s; aHalf_ = h; drag_ = isPan ? kDragPan : kDragRes;
                setSlotFromX(x);
                return kActNone;
            }
        }

        // staplarna
        int s, h;
        if (!hitCell(x, y, s, h)) return kActNone;
        aStep_ = s; aHalf_ = h;
        setInfo(s, h);
        if (dbl) {
            edit(paramLen(s, h), 1.0f);
            edit(paramHP(s, h), 0.0f);
            edit(paramLP(s, h), 1.0f);
            edit(paramPan(s, h), valueFromSlot(4));
            edit(paramRes(s, h), 0.0f);
            endEdits();
            return kActNone;
        }
        const Bar b = getBar(s, h);
        const float T = 6.0f;
        const bool nearRight = (x >= b.bx1 - T && x <= b.bx1 + T) && (y >= b.yTop - T && y <= b.yBot + T);
        if (nearRight) { drag_ = kDragLen; dragMove(x, y, false); return kActNone; }
        if (x >= b.bx0 && x <= b.bx1 && y >= b.yTop - T && y <= b.yBot + T) {
            bool nearTop = std::fabs(y - b.yTop) <= T, nearBot = std::fabs(y - b.yBot) <= T;
            if (nearTop && nearBot) { if (std::fabs(y - b.yTop) <= std::fabs(y - b.yBot)) nearBot = false; else nearTop = false; }
            if (nearTop) { drag_ = kDragLP; dragMove(x, y, false); return kActNone; }
            if (nearBot) { drag_ = kDragHP; dragMove(x, y, false); return kActNone; }
            drag_ = kDragBody;
            startY_ = y;
            startHP_ = host_->get(paramHP(s, h));
            startLP_ = host_->get(paramLP(s, h));
            beginOnly(paramHP(s, h)); beginOnly(paramLP(s, h));
            return kActNone;
        }
        if (y < b.yTop) { drag_ = kDragLP; dragMove(x, y, false); return kActNone; }   // ovanför: dra lowpass dit
        if (y > b.yBot) { drag_ = kDragHP; dragMove(x, y, false); return kActNone; }   // under: dra highpass dit
        drag_ = kDragLen; dragMove(x, y, false);                                        // till höger: sätt längd
        return kActNone;
    }

    void mouseMove(int x, int y, bool shift)
    {
        updateHover(x, y);
        if (drag_ == kDragThumb) {
            const float viewH = kManBottom - kManTop, maxScroll = maxScrollPx();
            const float th = std::max(30.0f, viewH * viewH / std::max(contentH_, 1.0f));
            if (viewH - th > 1.0f) scroll_ = thumbScroll0_ + (y - startY_) * maxScroll / (viewH - th);
            clampScroll();
            return;
        }
        if (drag_ != kDragNone) dragMove(x, y, shift);
    }

    void mouseLeave() { tipId_ = 0; tipPending_ = false; }

    void mouseUp()
    {
        endEdits();
        drag_ = kDragNone;
    }

    // Mushjul: rattar (små steg), PAN/RES (ett läge) och manualen (rullar)
    void wheel(int x, int y, int clicks)
    {
        if (clicks == 0) return;
        if (showBack) { scroll_ -= (float)clicks * 48.0f; clampScroll(); return; }
        for (int k = 0; k < 4; ++k) {
            const KnobDef& kd = knobs_[k];
            const float dx = x - kd.cx, dy = y - kd.cy, rr = kd.r + 12.0f;
            if (dx * dx + dy * dy > rr * rr) continue;
            const float stepSize = (kd.param == kParamOutput) ? 0.5f / (float)(kOutMaxDb - kOutMinDb) : 0.01f;
            edit(kd.param, host_->get(kd.param) + clicks * stepSize);
            endEdits();
            return;
        }
        int s, h; bool isPan;
        if (hitSelector(x, y, s, h, isPan)) {
            const int p = isPan ? paramPan(s, h) : paramRes(s, h);
            const int slot = slotFromValue(host_->get(p)) + (clicks > 0 ? 1 : -1);
            edit(p, valueFromSlot(slot < 0 ? 0 : (slot > 8 ? 8 : slot)));
            endEdits();
            setInfo(s, h);
        }
    }

    bool dragging() const { return drag_ != kDragNone; }

    int cursorAt(int x, int y) const
    {
        if (overLogo(x, y)) return kCurHand;
        if (showBack) return (btnBack_.hit(x, y) || langEng_.hit(x, y) || langSwe_.hit(x, y) || infoBox_.hit(x, y)) ? kCurHand : kCurArrow;
        if (presetPrev_.hit(x, y) || presetNext_.hit(x, y) || btnSave_.hit(x, y) || btnLoad_.hit(x, y) || btnRandom_.hit(x, y) || btnBank_.hit(x, y)) return kCurHand;
        if (y >= kSplitY && y < kSplitY + kSplitH && x >= kLeft && x < kLeft + kNumSteps * kCellW) return kCurHand;
        int s, h; bool isPan;
        if (hitSelector(x, y, s, h, isPan)) return kCurHand;
        if (drag_ == kDragLen) return kCurSizeWE;
        if (drag_ == kDragLP || drag_ == kDragHP) return kCurSizeNS;
        if (!hitCell(x, y, s, h)) return kCurArrow;
        const Bar b = getBar(s, h);
        const float T = 6.0f;
        if ((x >= b.bx1 - T && x <= b.bx1 + T) && (y >= b.yTop - T && y <= b.yBot + T)) return kCurSizeWE;
        if (x >= b.bx0 && x <= b.bx1 && (std::fabs(y - b.yTop) <= T || std::fabs(y - b.yBot) <= T)) return kCurSizeNS;
        if (x >= b.bx0 && x <= b.bx1 && y > b.yTop && y < b.yBot) return kCurSizeAll;
        return kCurArrow;
    }

private:
    enum Drag { kDragNone, kDragLen, kDragLP, kDragHP, kDragBody, kDragPan, kDragRes, kDragKnob, kDragThumb };

    struct KnobDef { int param; float cx, cy, r; };
    struct Bar { float cx0, cx1, bx0, bx1, yTop, yBot; };

    // Layout
    // Rutorna på nedre raden: DRY/WET (bred), STEP (smal), PRESET, OUTPUT. Mellanrum 12 px.
    static float colX(int i) { static const float x[4] = { 20.0f, 330.0f, 512.0f, 730.0f }; return x[i & 3]; }
    static float colW(int i) { static const float w[4] = { 298.0f, 170.0f, 206.0f, 150.0f }; return w[i & 3]; }
    static constexpr float kMainY = 83.0f, kMainH = 322.0f;
    static constexpr float kLeft = 78.0f, kCellW = 48.0f, kGridTop = 134.0f, kGridH = 170.0f;
    static constexpr float kNumY = 112.0f;
    static constexpr float kSplitY = 312.0f, kSplitH = 20.0f;
    static constexpr float kPanY = 337.0f, kPanH = 22.0f;
    static constexpr float kResY = 365.0f, kResH = 22.0f;
    static constexpr float kRow2Y = 417.0f, kRow2H = 190.0f;
    static constexpr float kManX = 40.0f, kManW = 798.0f, kManTop = 224.0f, kManBottom = 600.0f;
    static constexpr float kThumbX = 852.0f, kThumbW = 6.0f;

    GuiHost* host_;
    KnobDef knobs_[4] = { { kParamMix, colX(0) + 79.0f, kRow2Y + 99.0f, 44.0f },
                          { kParamOutput, colX(3) + 75.0f, kRow2Y + 99.0f, 44.0f },
                          { kParamHumanize, colX(0) + 243.0f, kRow2Y + 75.0f, 20.0f },
                          { kParamSampleHold, colX(0) + 173.0f, kRow2Y + 75.0f, 20.0f } };
    Rect presetPrev_ = { colX(2) + 14.0f, kRow2Y + 44.0f, 30.0f, 30.0f };
    Rect presetNext_ = { colX(2) + 162.0f, kRow2Y + 44.0f, 30.0f, 30.0f };
    Rect presetBox_ = { colX(2) + 48.0f, kRow2Y + 44.0f, 110.0f, 30.0f };
    Rect btnSave_ = { colX(2) + 14.0f, kRow2Y + 90.0f, 84.0f, 34.0f };
    Rect btnLoad_ = { colX(2) + 106.0f, kRow2Y + 90.0f, 84.0f, 34.0f };
    Rect btnRandom_ = { colX(0) + 144.0f, kRow2Y + 135.0f, 87.0f, 34.0f };
    Rect btnBank_ = { colX(0) + 239.0f, kRow2Y + 135.0f, 33.0f, 34.0f };
    std::vector<std::vector<std::string>> wrapEn_, wrapSv_;   // manualen radbruten (se wrapText)
    std::vector<std::string> tipLines_;
    int tipKey_ = -1;
    Rect langEng_ = { 0, 0, 0, 0 }, langSwe_ = { 0, 0, 0, 0 }, infoBox_ = { 0, 0, 0, 0 };
    int tipId_ = 0;
    bool tipPending_ = false;
    uint32_t tipSince_ = 0;   // sätts vid ritning av baksidan
    Rect btnBack_ = { 760.0f, 114.0f, 100.0f, 34.0f };

    Drag drag_ = kDragNone;
    int aStep_ = 0, aHalf_ = 0, knob_ = 0, startY_ = 0;
    float startHP_ = 0.0f, startLP_ = 1.0f, startVal_ = 0.0f;
    int infoStep_ = 0, infoHalf_ = 0;
    float scroll_ = 0.0f, contentH_ = 0.0f, thumbScroll0_ = 0.0f;
    float meterL_ = 0.0f, meterR_ = 0.0f;
    uint32_t overUntil_ = 0;
    float logoRight_ = 200.0f;
    int editParams_[8];
    int numEdit_ = 0;

    // --- Parameteränd­ringar (grupperas för automation i värden) ---
    void beginOnly(int p)
    {
        for (int i = 0; i < numEdit_; ++i) if (editParams_[i] == p) return;
        if (numEdit_ < 8) { editParams_[numEdit_++] = p; host_->beginEdit(p); }
    }
    void edit(int p, float v)
    {
        v = !(v >= 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
        beginOnly(p);
        host_->set(p, v);
    }
    void endEdits()
    {
        for (int i = 0; i < numEdit_; ++i) host_->endEdit(editParams_[i]);
        numEdit_ = 0;
    }

    bool isSplit(int s) const { return isSplitValue(host_->get(kParamSplitBase + s)); }
    void setInfo(int s, int h) { infoStep_ = s; infoHalf_ = h; }

    // --- Geometri ---
    void cellX(int s, int half, float& x0, float& x1) const
    {
        x0 = kLeft + s * kCellW;
        x1 = x0 + kCellW;
        if (isSplit(s)) { const float hw = kCellW / 2; x0 += half * hw; x1 = x0 + hw; }
    }
    static float yFromVal(float v) { return kGridTop + (1.0f - v) * kGridH; }
    static float valFromY(float y)
    {
        const float v = 1.0f - (y - kGridTop) / kGridH;
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }
    Bar getBar(int s, int h) const
    {
        Bar b;
        cellX(s, h, b.cx0, b.cx1);
        const float len = host_->get(paramLen(s, h));
        const float hp = host_->get(paramHP(s, h));
        float lp = host_->get(paramLP(s, h));
        if (lp < hp) lp = hp;
        b.bx0 = b.cx0 + 1.0f;                                              // hela pixlar (ingen kantutjämning)
        b.bx1 = b.bx0 + std::floor(len * ((b.cx1 - b.cx0) - 1.0f) + 0.5f);
        b.yTop = std::floor(yFromVal(lp) + 0.5f);
        b.yBot = std::floor(yFromVal(hp) + 0.5f);
        return b;
    }
    // --- Infotext: vad pekar musen på? ---
    int tipAt(int x, int y) const
    {
        if (overLogo(x, y)) return kTipLogo;
        if (showBack) {
            if (btnBack_.hit(x, y)) return kTipBack;
            if (langEng_.hit(x, y) || langSwe_.hit(x, y)) return kTipLang;
            if (infoBox_.hit(x, y)) return kTipInfo;
            const float dx = x - 869.0f, dy = y - 42.0f;
            return (dx * dx + dy * dy <= 100.0f) ? kTipMeter : kTipNone;
        }
        { const float dx = x - 869.0f, dy = y - 42.0f; if (dx * dx + dy * dy <= 100.0f) return kTipMeter; }
        for (int k = 0; k < 4; ++k) {
            const KnobDef& kd = knobs_[k];
            const float dx = x - kd.cx, dy = y - kd.cy, rr = kd.r + 6.0f;
            if (dx * dx + dy * dy > rr * rr) continue;
            return kd.param == kParamMix ? kTipDryWet : kd.param == kParamOutput ? kTipOutput
                 : kd.param == kParamHumanize ? kTipHuman : kTipSH;
        }
        if (btnRandom_.hit(x, y)) return kTipRandom;
        if (btnBank_.hit(x, y)) return kTipBank;
        if (presetPrev_.hit(x, y) || presetNext_.hit(x, y) || presetBox_.hit(x, y)) return kTipPreset;
        if (btnSave_.hit(x, y)) return kTipSave;
        if (btnLoad_.hit(x, y)) return kTipLoad;
        const Rect over = { colX(3) + 30.0f, kRow2Y + 167.0f, 96.0f, 18.0f };
        if (over.hit(x, y)) return kTipOver;
        const Rect tDry = { colX(0) + 12.0f, kRow2Y + 6.0f, 110.0f, 26.0f }, tOut = { colX(3) + 12.0f, kRow2Y + 6.0f, 90.0f, 26.0f },
                   tPre = { colX(2) + 12.0f, kRow2Y + 6.0f, 100.0f, 26.0f }, pStep = { colX(1), kRow2Y, colW(1), kRow2H },
                   tGrid = { 36.0f, 94.0f, 100.0f, 22.0f }, rowNum = { kLeft, 110.0f, kNumSteps * kCellW, 20.0f };
        if (tDry.hit(x, y)) return kTipDryWet;
        if (tOut.hit(x, y)) return kTipOutput;
        if (tPre.hit(x, y)) return kTipPreset;
        if (pStep.hit(x, y)) return kTipStepPanel;
        if (tGrid.hit(x, y)) return kTipGridTitle;
        if (rowNum.hit(x, y)) return kTipStepNum;
        if (x >= 20 && x < kLeft + kNumSteps * kCellW) {
            if (y >= kSplitY && y < kSplitY + kSplitH) return kTipSplit;
            if (y >= kPanY && y <= kPanY + kPanH) return kTipPan;
            if (y >= kResY && y <= kResY + kResH) return kTipRes;
        }
        int s, h;
        if (hitCell(x, y, s, h)) {
            const Bar b = getBar(s, h);
            const float T = 6.0f;
            if ((x >= b.bx1 - T && x <= b.bx1 + T) && (y >= b.yTop - T && y <= b.yBot + T)) return kTipLen;
            if (x >= b.bx0 && x <= b.bx1 && std::fabs(y - b.yTop) <= T) return kTipLP;
            if (x >= b.bx0 && x <= b.bx1 && std::fabs(y - b.yBot) <= T) return kTipHP;
            return kTipGrid;
        }
        return kTipNone;
    }

    void updateHover(int x, int y)
    {
        const int t = tipAt(x, y);
        if (t != tipId_) { tipId_ = t; tipPending_ = true; }
    }

    // Ritar infotext-rutan (mellan loggan och versionsnumret) när musen vilat en stund
    void drawTip(Canvas& c, uint32_t nowMs)
    {
        if (tipPending_) { tipSince_ = nowMs; tipPending_ = false; }
        if (!host_->infoOn() || tipId_ == kTipNone || drag_ != kDragNone) return;
        if ((int32_t)(nowMs - tipSince_) < 300) return;
        const char* raw = tipText(tipId_, host_->language());
        if (!raw[0]) return;
        const float bx = 270.0f, by = 21.0f, bw = 384.0f, bh = 50.0f;
        c.roundRect(bx, by, bw, bh, 12.0f, theme::box, theme::panelEdge, 1.0f);
        const float tw = bw - 22.0f;
        const int key = tipId_ * 2 + host_->language();
        if (key != tipKey_) { tipLines_ = wrapText(c, typeset(raw), tw * 0.97f, 14.0f, kStyleRegular); tipKey_ = key; }
        // Fasta rader som på referensbilden: första raden alltid på samma plats, andra raden 18 px under
        for (size_t k = 0; k < tipLines_.size() && k < 2; ++k)
            c.text(279.5f, 26.7f + 18.0f * (float)k, tw, tipLines_[k].c_str(), 14.0f, kStyleRegular,
                   argb(255, 150, 153, 157), kAlignNear);
    }

    bool hitCell(int x, int y, int& s, int& h) const
    {
        if (x < kLeft || x >= kLeft + kNumSteps * kCellW) return false;
        if (y < kGridTop || y > kGridTop + kGridH) return false;
        s = (int)((x - kLeft) / kCellW);
        h = 0;
        if (isSplit(s) && ((x - kLeft) - s * kCellW) >= kCellW / 2) h = 1;
        return true;
    }
    void selX(int s, int half, float& x0, float& x1) const
    {
        const float cx = kLeft + s * kCellW;
        if (isSplit(s)) { const float hw = kCellW / 2; x0 = cx + half * hw + 2.0f; x1 = cx + (half + 1) * hw - 2.0f; }
        else { x0 = cx + 3.0f; x1 = cx + kCellW - 2.0f; }
    }
    bool hitSelector(int x, int y, int& s, int& h, bool& isPan) const
    {
        if (x < kLeft || x >= kLeft + kNumSteps * kCellW) return false;
        if (y >= kPanY && y < kPanY + kPanH) isPan = true;
        else if (y >= kResY && y < kResY + kResH) isPan = false;
        else return false;
        s = (int)((x - kLeft) / kCellW);
        h = 0;
        if (isSplit(s) && ((x - kLeft) - s * kCellW) >= kCellW / 2) h = 1;
        return true;
    }

    void toggleSplit(int s)
    {
        const bool on = !isSplit(s);
        if (on) {   // andra halvan börjar som en kopia av första
            edit(paramLen(s, 1), host_->get(paramLen(s, 0)));
            edit(paramHP(s, 1), host_->get(paramHP(s, 0)));
            edit(paramLP(s, 1), host_->get(paramLP(s, 0)));
            edit(paramPan(s, 1), host_->get(paramPan(s, 0)));
            edit(paramRes(s, 1), host_->get(paramRes(s, 0)));
        }
        edit(kParamSplitBase + s, on ? 1.0f : 0.0f);
        setInfo(s, 0);
    }

    void setSlotFromX(int x)
    {
        float x0, x1;
        selX(aStep_, aHalf_, x0, x1);
        int i = (int)std::floor((x - x0) * (float)kNumSlots / (x1 - x0));
        if (i < 0) i = 0;
        if (i > kNumSlots - 1) i = kNumSlots - 1;
        edit(drag_ == kDragPan ? paramPan(aStep_, aHalf_) : paramRes(aStep_, aHalf_), valueFromSlot(i));
    }

    void dragMove(int x, int y, bool shift)
    {
        const float kMinGap = 0.02f;
        switch (drag_) {
        case kDragLen: {
            const Bar b = getBar(aStep_, aHalf_);
            const float innerW = (b.cx1 - b.cx0) - 1.0f;
            float v = (x - (b.cx0 + 1.0f)) / innerW;
            v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            if (v < 0.03f) v = 0.0f;
            if (v > 0.97f) v = 1.0f;
            edit(paramLen(aStep_, aHalf_), v);
            break; }
        case kDragLP: {
            float v = valFromY((float)y);
            const float lo = host_->get(paramHP(aStep_, aHalf_)) + kMinGap;
            if (v < lo) v = lo;
            if (v > 0.985f) v = 1.0f;
            edit(paramLP(aStep_, aHalf_), v);
            break; }
        case kDragHP: {
            float v = valFromY((float)y);
            const float hi = host_->get(paramLP(aStep_, aHalf_)) - kMinGap;
            if (v > hi) v = hi;
            if (v < 0.015f) v = 0.0f;
            edit(paramHP(aStep_, aHalf_), v);
            break; }
        case kDragBody: {
            float d = (float)(startY_ - y) / kGridH;
            if (d < -startHP_) d = -startHP_;
            if (d > 1.0f - startLP_) d = 1.0f - startLP_;
            edit(paramHP(aStep_, aHalf_), startHP_ + d);
            edit(paramLP(aStep_, aHalf_), startLP_ + d);
            break; }
        case kDragPan:
        case kDragRes:
            setSlotFromX(x);
            return;
        case kDragKnob: {
            const float per = shift ? 1000.0f : 250.0f;   // Shift = finjustering
            edit(knobs_[knob_].param, startVal_ + (float)(startY_ - y) / per);
            return; }
        default:
            return;
        }
    }

    // --- Framsidan ---
    void drawFront(Canvas& c, bool over)
    {
        const int playing = host_->playPos();
        const int ps = playing >= 0 ? (playing >> 1) : -1, ph = playing >= 0 ? (playing & 1) : 0;

        // === Stegrutnätet ===
        drawPanel(c, 20.0f, kMainY, 860.0f, kMainH);
        c.text(36, kMainY + 10, 200, "STEP GRID", 16, kStyleBold, theme::text, kAlignNear);
        c.text(300, kMainY + 13, 564, "Right edge = LENGTH     Top edge = LOWPASS     Bottom edge = HIGHPASS", 12, kStyleRegular,
               theme::textDim, kAlignFar);

        for (int s = 0; s < kNumSteps; ++s) {
            const float x0 = kLeft + s * kCellW;
            char num[16];
            std::snprintf(num, sizeof num, "%d", (s + 1) & 31);
            c.text(x0, kNumY, kCellW, num, 12, (s % 4 == 0 || s == ps) ? kStyleBold : kStyleRegular,
                   s == ps ? theme::orange : ((s % 4 == 0) ? theme::text : theme::textDim), kAlignCenter);
            c.fillRect(x0, kGridTop, kCellW, kGridH, ((s / 4) & 1) ? theme::cellB : theme::cellA);
        }
        if (ps >= 0) {
            float x0, x1;
            cellX(ps, isSplit(ps) ? ph : 0, x0, x1);
            c.fillRect(x0, kGridTop, x1 - x0, kGridH, theme::playhead);
        }

        // frekvenslinjer (100 Hz, 1 kHz, 10 kHz)
        static const float kLineVal[3] = { 0.2331f, 0.5663f, 0.8997f };
        static const char* const kLineTxt[3] = { "100", "1k", "10k" };
        for (int i = 0; i < 3; ++i) {
            const float y = yFromVal(kLineVal[i]);
            c.line(kLeft, y, kLeft + kNumSteps * kCellW, y, theme::grid, 1.0f);
            c.text(kLeft - 52.0f, y - 8.0f, 46.0f, kLineTxt[i], 12, kStyleRegular, theme::textDim, kAlignFar);
        }
        for (int s = 0; s <= kNumSteps; ++s) {
            const float x = kLeft + s * kCellW;
            c.line(x, kGridTop, x, kGridTop + kGridH, theme::grid, 1.0f);
        }

        // staplar
        for (int s = 0; s < kNumSteps; ++s) {
            const bool sp = isSplit(s);
            for (int h = 0; h < (sp ? 2 : 1); ++h) {
                const Bar b = getBar(s, h);
                const bool now = (s == ps && (sp ? h == ph : true));
                const Rgba fill = now ? theme::barFillOn : theme::barFill;
                const Rgba handle = now ? theme::barHandleOn : theme::barHandle;
                const float bw = b.bx1 - b.bx0, bh = b.yBot - b.yTop;
                if (bw >= 3.0f && bh >= 5.0f) {
                    // Staplar i hela pixlar: 2 px ljus kant upptill, nedtill och till höger, 1 px grå kant till vänster
                    c.fillRect(b.bx0, b.yTop, bw, bh, fill);
                    c.fillRect(b.bx0, b.yTop + 2.0f, 1.0f, bh - 4.0f, now ? theme::orange : theme::barEdgeL);
                    c.fillRect(b.bx0, b.yTop, bw, 2.0f, handle);
                    c.fillRect(b.bx0, b.yBot - 2.0f, bw, 2.0f, handle);
                    c.fillRect(b.bx1 - 2.0f, b.yTop, 2.0f, bh, handle);
                } else if (bw > 0.0f) {
                    c.fillRect(b.bx0, b.yTop, bw, bh < 2.0f ? 2.0f : bh, handle);
                } else {
                    c.fillRect(b.bx0, b.yTop, 2.0f, bh < 2.0f ? 2.0f : bh, handle);   // längd 0: bara ett streck
                }
            }
            if (sp) {
                const float xm = kLeft + s * kCellW + kCellW / 2;
                c.line(xm, kGridTop, xm, kGridTop + kGridH, theme::orange, 1.0f);
            }
        }

        // 1/32-knappar
        c.text(20, kSplitY + 3, 52, "1/32", 12, kStyleBold, theme::textDim, kAlignFar);
        for (int s = 0; s < kNumSteps; ++s) {
            const Rect r = { kLeft + s * kCellW + 3.0f, kSplitY, kCellW - 6.0f, kSplitH };
            drawButton(c, r, "1/32", isSplit(s), theme::orange, 11.0f);
        }

        // PAN och RES (9 lägen per steg)
        c.text(20, kPanY + 4, 52, "PAN", 12, kStyleBold, theme::textDim, kAlignFar);
        c.text(20, kResY + 3, 52, "RES", 12, kStyleBold, theme::textDim, kAlignFar);
        for (int s = 0; s < kNumSteps; ++s) {
            for (int h = 0; h < (isSplit(s) ? 2 : 1); ++h) {
                float sx0, sx1;
                selX(s, h, sx0, sx1);
                drawSelector(c, sx0, sx1, kPanY, kPanY + kPanH, slotFromValue(host_->get(paramPan(s, h))), true);
                drawSelector(c, sx0, sx1, kResY, kResY + kResH, slotFromValue(host_->get(paramRes(s, h))), false);
            }
        }

        // === DRY/WET (med HUMANIZE) och OUTPUT ===
        for (int k = 0; k < 2; ++k) {
            const KnobDef& kd = knobs_[k];
            const float x0 = colX(k == 0 ? 0 : 3);
            drawPanel(c, x0, kRow2Y, colW(k == 0 ? 0 : 3), kRow2H);
            c.text(x0 + 16, kRow2Y + 10, 180, k == 0 ? "DRY/WET" : "OUTPUT", 16, kStyleBold, theme::text, kAlignNear);
            drawKnob(c, kd.cx, kd.cy, kd.r, host_->get(kd.param));
            char txt[32];
            if (k == 0) std::snprintf(txt, sizeof txt, "%d %%", (int)(host_->get(kParamMix) * 100.0f + 0.5f));
            else std::snprintf(txt, sizeof txt, "%+.1f dB", outputDbFromNorm(host_->get(kParamOutput)));
            if (k == 1 && std::fabs(outputDbFromNorm(host_->get(kParamOutput))) < 0.05) std::snprintf(txt, sizeof txt, "0.0 dB");
            c.text(kd.cx - 70.0f, kd.cy + 43.0f, 140.0f, txt, 22, kStyleBold, theme::cyan, kAlignCenter);
            if (k == 1) c.text(x0 - 14, kRow2Y + 169, 104, "OVER 0 dB", 11, kStyleBold, over ? theme::red : theme::redOff, kAlignFar);
        }
        for (int k = 2; k < 4; ++k) {
            const KnobDef& hk = knobs_[k];
            const float hv = host_->get(hk.param);
            c.text(hk.cx - 40.0f, hk.cy - 48.0f, 80.0f, k == 2 ? "HUMANIZE" : "S/H", 12, kStyleBold, theme::text, kAlignCenter);
            drawKnob(c, hk.cx, hk.cy, hk.r, hv);
            char ht[24];
            if (k == 2) std::snprintf(ht, sizeof ht, "%.1f ms", (double)hv * kHumanizeMaxMs);
            else std::snprintf(ht, sizeof ht, "%d %%", (int)(hv * 100.0f + 0.5f));
            c.text(hk.cx - 40.0f, hk.cy + 29.0f, 80.0f, ht, 13, kStyleBold, theme::cyan, kAlignCenter);
        }
        {
            drawButton(c, btnRandom_, "RANDOMIZE", false, theme::cyan, 12.0f);
            char bn[16];
            std::snprintf(bn, sizeof bn, "%d", host_->bankNumber());
            drawButton(c, btnBank_, bn, host_->bankActive(), theme::cyan);
        }

        // === PRESET ===
        {
            const float x0 = colX(2);
            drawPanel(c, x0, kRow2Y, colW(2), kRow2H);
            c.text(x0 + 16, kRow2Y + 10, 180, "PRESET", 16, kStyleBold, theme::text, kAlignNear);
            drawButton(c, presetPrev_, "<", false, theme::cyan);
            drawButton(c, presetNext_, ">", false, theme::cyan);
            drawBox(c, presetBox_.x, presetBox_.y, presetBox_.w, presetBox_.h);
            c.clip(presetBox_.x + 3, presetBox_.y, presetBox_.w - 6, presetBox_.h);
            c.text(presetBox_.x + 3, presetBox_.y + 7, presetBox_.w - 6, host_->presetName(), 12, kStyleBold, theme::cyan, kAlignCenter);
            c.unclip();
            drawButton(c, btnSave_, "SAVE", false, theme::cyan);
            drawButton(c, btnLoad_, "LOAD", false, theme::cyan);
            c.text(x0, kRow2Y + 138, colW(2), "Your presets are stored in", 11, kStyleRegular, theme::textDim, kAlignCenter);
            c.text(x0, kRow2Y + 154, colW(2), "Documents\\TrueStep Presets", 11, kStyleRegular, theme::textDim, kAlignCenter);
        }

        // === STEP (värden för valt steg) ===
        {
            const float x0 = colX(1);
            const float pw = colW(1);
            drawPanel(c, x0, kRow2Y, pw, kRow2H);
            c.text(x0 + 16, kRow2Y + 10, 100, "STEP", 16, kStyleBold, theme::text, kAlignNear);
            const int s = infoStep_, h = infoHalf_ < (isSplit(s) ? 2 : 1) ? infoHalf_ : 0;
            char id[16];
            std::snprintf(id, sizeof id, "%d%s", s + 1, isSplit(s) ? (h ? "b" : "a") : "");
            c.text(x0 + pw - 16.0f - 100.0f, kRow2Y + 6, 100, id, 22, kStyleBold, theme::orange, kAlignFar);
            char hp[24], lp[24], pn[16], v[24];
            hzText(host_->get(paramHP(s, h)), hp, sizeof hp);
            hzText(host_->get(paramLP(s, h)), lp, sizeof lp);
            panText(slotFromValue(host_->get(paramPan(s, h))), pn, sizeof pn);
            const char* const labels[5] = { "LENGTH", "HIGHPASS", "LOWPASS", "PAN", "RESONANCE" };
            char vals[5][24];
            std::snprintf(vals[0], 24, "%d %%", (int)(host_->get(paramLen(s, h)) * 100.0f + 0.5f));
            std::snprintf(vals[1], 24, "%s", hp);
            std::snprintf(vals[2], 24, "%s", lp);
            std::snprintf(vals[3], 24, "%s", pn);
            std::snprintf(v, sizeof v, "%d", slotFromValue(host_->get(paramRes(s, h))));
            std::snprintf(vals[4], 24, "%s", v);
            for (int i = 0; i < 5; ++i) {
                const float y = kRow2Y + 50.0f + i * 26.0f;
                c.text(x0 + 16, y + 2, 100, labels[i], 12, kStyleBold, theme::textDim, kAlignNear);
                c.text(x0 + 16, y, pw - 32.0f, vals[i], 14, kStyleBold, theme::text, kAlignFar);
            }
        }

        c.text(20, 614, 860, "Drag the bars  |  Shift = fine  |  mouse wheel = small steps  |  double-click = reset  |  click the logo for the manual",
               12, kStyleRegular, theme::textDim, kAlignNear);
    }

    // 50 % blandning mot panelens färg: ger "halv" översta pixelrad så att trappan upplevs som en rak linje
    static Rgba halfTone(Rgba c)
    {
        const Rgba p = theme::panel;
        auto mix = [](Rgba a, Rgba b, int sh) { return (((a >> sh) & 255u) + ((b >> sh) & 255u) + 1u) / 2u; };
        return argb(255, (int)mix(c, p, 16), (int)mix(c, p, 8), (int)mix(c, p, 0));
    }

    // PAN: V-form (22,19,16,13,10,13,16,19,22 px). RES: trappa (3,5,7,9,11.5,14,16.5,19,22 px; stapel 5 och 7 har en halv översta rad).
    void drawSelector(Canvas& c, float x0, float x1, float y0, float y1, int slot, bool isPan)
    {
        static const float kResHeights[kNumSlots] = { 3.0f, 5.0f, 7.0f, 9.0f, 11.5f, 14.0f, 16.5f, 19.0f, 22.0f };
        const float w = x1 - x0, scale = (y1 - y0) / 22.0f;
        for (int i = 0; i < kNumSlots; ++i) {
            float bx0, bw;
            if (w >= 40.0f) { bx0 = std::floor(x0) + i * 5.0f; bw = 3.0f; }          // 3 px breda staplar, 5 px delning
            else {
                bx0 = x0 + std::floor(i * w / kNumSlots);
                const float bx1 = x0 + std::floor((i + 1) * w / kNumSlots);
                bw = bx1 - bx0 > 1.0f ? bx1 - bx0 - (w >= 30.0f ? 1.0f : 0.0f) : 1.0f;
                if (bw < 1.0f) bw = 1.0f;
            }
            const float h = (isPan ? (10.0f + 3.0f * (float)std::abs(i - 4)) : kResHeights[i]) * scale;
            const float full = std::floor(h);
            const bool sel = (i == slot);
            const Rgba col = isPan ? (sel ? theme::panOn : theme::panOff) : (sel ? theme::resOn : theme::resOff);
            c.fillRect(bx0, y1 - full, bw, full, col);
            if (h - full > 0.01f) c.fillRect(bx0, y1 - full - 1.0f, bw, 1.0f, halfTone(col));
        }
    }

    // --- Baksidan: vem som byggt TrueStep och en rullbar manual ---
    float maxScrollPx() const { return std::max(0.0f, contentH_ - (kManBottom - kManTop)); }
    void clampScroll()
    {
        const float m = maxScrollPx();
        if (scroll_ > m) scroll_ = m;
        if (scroll_ < 0.0f) scroll_ = 0.0f;
    }

    void backMouseDown(int x, int y)
    {
        if (infoBox_.hit(x, y)) { host_->setInfoOn(!host_->infoOn()); return; }
        if (langEng_.hit(x, y) && host_->language() != kLangEng) { host_->setLanguage(kLangEng); contentH_ = 0.0f; scroll_ = 0.0f; return; }
        if (langSwe_.hit(x, y) && host_->language() != kLangSwe) { host_->setLanguage(kLangSwe); contentH_ = 0.0f; scroll_ = 0.0f; return; }
        if (x >= kThumbX - 6 && x <= kThumbX + kThumbW + 6 && y >= kManTop && y <= kManBottom && maxScrollPx() > 0.0f) {
            const float viewH = kManBottom - kManTop, maxScroll = maxScrollPx();
            const float th = std::max(30.0f, viewH * viewH / contentH_);
            const float ty = kManTop + (viewH - th) * (scroll_ / maxScroll);
            if (y < ty) { scroll_ -= viewH * 0.9f; clampScroll(); }
            else if (y > ty + th) { scroll_ += viewH * 0.9f; clampScroll(); }
            else { drag_ = kDragThumb; startY_ = y; thumbScroll0_ = scroll_; }
        }
    }

    static void manualFont(int kind, float& size, int& style, Rgba& color)
    {
        if (kind == kManHead) { size = 14.0f; style = kStyleBold; color = theme::cyan; }
        else if (kind == kManNote) { size = 12.0f; style = kStyleItalic; color = theme::text; }
        else { size = 13.0f; style = kStyleRegular; color = theme::text; }
    }

    // Ritar (eller bara mäter, om draw = false) manualen; returnerar total höjd
    float layoutManual(Canvas& c, float y0, bool draw)
    {
        float y = y0;
        int count = 0;
        const ManualPara* man = manualFor(host_->language(), count);
        std::vector<std::vector<std::string>>& cache = host_->language() == kLangSwe ? wrapSv_ : wrapEn_;
        if ((int)cache.size() != count) {   // radbrytning görs en gång per språk
            cache.clear();
            for (int i = 0; i < count; ++i) {
                float size; int style; Rgba color;
                manualFont(man[i].kind, size, style, color);
                if (man[i].kind == kManHead) cache.push_back(std::vector<std::string>(1, std::string(man[i].text)));
                else cache.push_back(wrapText(c, typeset(man[i].text), kManW * 0.96f, size, style));
            }
        }
        for (int i = 0; i < count; ++i) {
            const ManualPara& mp = man[i];
            const std::vector<std::string>& lines = cache[(size_t)i];
            float size; int style; Rgba color;
            manualFont(mp.kind, size, style, color);
            if (mp.kind == kManHead && i > 0) y += 14.0f;
            const float lh = c.textHeight("Ag", kManW, size, style);
            const float h = lh * (float)lines.size();
            if (draw && y + h > kManTop - 20.0f && y < kManBottom + 20.0f)
                for (size_t k = 0; k < lines.size(); ++k) {
                    const float ly = y + lh * (float)k;
                    if (ly + lh > kManTop - 4.0f && ly < kManBottom + 4.0f)
                        c.text(kManX, ly, kManW, lines[k].c_str(), size, style, color, kAlignNear);
                }
            y += h + (mp.kind == kManHead ? 3.0f : 7.0f);
        }
        return y - y0;
    }

    void drawBack(Canvas& c)
    {
        // Credits
        drawPanel(c, 20.0f, 83.0f, 860.0f, 92.0f);
        {
            char t[64];
            std::snprintf(t, sizeof t, "TrueStep v%s", kVersionText);
            c.text(38, 92, 400, t, 22, kStyleBold, theme::text, kAlignNear);
        }
        const bool sv = host_->language() == kLangSwe;   // bara manualtexten byter språk; resten av fönstret är engelskt
        c.text(38, 123, 700, "Rhythmic Step Filter Gate  \xC2\xB7  VST2 / VST3  \xC2\xB7  Windows 7 and later", 13, kStyleItalic,
               theme::text, kAlignNear);
        {
            // Fredrik Frigren | Copyright (c) 2F Productions 2026 | info@2f.se
            // Copyright-tecknet ritas själv (ring + C) så att det blir tydligt i alla storlekar.
            const char* partA = "Fredrik Frigren  |  Copyright ";
            const char* partB = " 2F Productions 2026  |  info@2f.se";
            float x = 40.0f;
            const float y = 148.0f;
            c.text(x, y, 600, partA, 15, kStyleBold, theme::cyan, kAlignNear);
            x += c.textWidth(partA, 15, kStyleBold);
            const float d = 15.0f, cyc = y + 9.5f;
            c.ellipse(x + 1.0f + d * 0.5f, cyc, d * 0.5f, d * 0.5f, kNone, theme::cyan, 1.6f);
            c.text(x + 1.0f, cyc - 7.5f, d, "C", 10, kStyleBold, theme::cyan, kAlignCenter);
            x += d + 2.0f;
            c.text(x, y, 600, partB, 15, kStyleBold, theme::cyan, kAlignNear);
        }
        drawButton(c, btnBack_, "BACK", false, theme::cyan);

        // Manual
        drawPanel(c, 20.0f, 185.0f, 860.0f, 435.0f);
        c.text(38, 192, 200, "MANUAL", 14, kStyleBold, theme::textDim, kAlignNear);
        {
            // ENG | SWE efter ordet MANUAL; vald språk i cyan, det andra dämpat
            const float wMan = c.textWidth("MANUAL", 14, kStyleBold);
            const float wEng = c.textWidth("ENG", 12, kStyleBold), wSwe = c.textWidth("SWE", 12, kStyleBold);
            const float wBar = c.textWidth("|", 12, kStyleBold);
            float lx = 38.0f + wMan + 16.0f;
            const float ly = 193.0f;
            c.text(lx, ly, 60, "ENG", 12, kStyleBold, sv ? theme::textDim : theme::cyan, kAlignNear);
            langEng_ = { lx - 3.0f, ly - 2.0f, wEng + 6.0f, 20.0f };
            lx += wEng + 8.0f;
            c.text(lx, ly, 20, "|", 12, kStyleBold, theme::textDim, kAlignNear);
            lx += wBar + 8.0f;
            c.text(lx, ly, 60, "SWE", 12, kStyleBold, sv ? theme::cyan : theme::textDim, kAlignNear);
            langSwe_ = { lx - 3.0f, ly - 2.0f, wSwe + 6.0f, 20.0f };
            // INFO BOX / INFORUTA + liten kvadrat som tänds (blå) och släcks (svart) med ett klick
            lx += wSwe + 8.0f;
            c.text(lx, ly, 20, "|", 12, kStyleBold, theme::textDim, kAlignNear);
            lx += wBar + 8.0f;
            const char* infoLabel = sv ? "INFORUTA" : "INFO BOX";
            c.text(lx, ly, 120, infoLabel, 12, kStyleBold, theme::textDim, kAlignNear);
            const float wLab = c.textWidth(infoLabel, 12, kStyleBold);
            const float sx = std::floor(lx + wLab + 6.0f), sy = ly + 3.0f;      // kvadraten: 11 x 11 px, kant 1 px
            const Rgba edge = argb(255, 122, 127, 132);
            c.fillRect(sx, sy, 11.0f, 1.0f, edge);
            c.fillRect(sx, sy + 10.0f, 11.0f, 1.0f, edge);
            c.fillRect(sx, sy + 1.0f, 1.0f, 9.0f, edge);
            c.fillRect(sx + 10.0f, sy + 1.0f, 1.0f, 9.0f, edge);
            if (host_->infoOn()) c.fillRect(sx + 3.0f, sy + 3.0f, 5.0f, 5.0f, argb(255, 2, 190, 220));
            infoBox_ = { lx - 3.0f, ly - 2.0f, (sx + 11.0f) - lx + 6.0f, 20.0f };   // etiketten och kvadraten är klickbara
        }
        c.text(380, 195, 478, "scroll with the mouse wheel  \xC2\xB7  click the logo or BACK to return", 11, kStyleRegular,
               theme::textDim, kAlignFar);
        if (contentH_ <= 0.0f) contentH_ = layoutManual(c, 0.0f, false);
        clampScroll();
        c.clip(kManX - 4.0f, kManTop, kManW + 12.0f, kManBottom - kManTop);
        layoutManual(c, kManTop - scroll_, true);
        c.unclip();

        // Rullningslist
        const float viewH = kManBottom - kManTop;
        c.fillRect(kThumbX, kManTop, kThumbW, viewH, argb(255, 30, 33, 37));
        const float maxScroll = maxScrollPx();
        if (maxScroll > 0.0f) {
            const float th = std::max(30.0f, viewH * viewH / contentH_);
            const float ty = kManTop + (viewH - th) * (scroll_ / maxScroll);
            c.fillRect(kThumbX, ty, kThumbW, th, drag_ == kDragThumb ? theme::cyan : argb(255, 90, 96, 104));
        }
    }
};

} // namespace truestep
