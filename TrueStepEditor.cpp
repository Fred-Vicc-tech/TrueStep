// TrueStepEditor.cpp - pluginfönstret (Windows, GDI+). All logik och layout finns i
// TrueStepGui.h; här finns bara det som är Windows: ritkommandona (Canvas) med
// GDI+, fönstret med mus och mushjul, och presetfilerna (SAVE/LOAD).
#include "TrueStepCore.h"
#include "TrueStepGui.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <new>
#include <windows.h>
// gdiplus.h behöver min/max, som NOMINMAX (i CMakeLists) stänger av.
#include <algorithm>
namespace Gdiplus { using std::min; using std::max; }
#include <gdiplus.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <vector>

using namespace truestep;

static HINSTANCE g_instance = nullptr;
static ULONG_PTR g_gdiplusToken = 0;
static int g_gdiplusUsers = 0;
static bool g_classRegistered = false;
static const wchar_t* kWindowClass = L"TrueStepEditorWindow";

namespace {

using namespace Gdiplus;

Color toColor(Rgba c) { return Color((BYTE)(c >> 24), (BYTE)(c >> 16), (BYTE)(c >> 8), (BYTE)c); }

std::wstring wideFromUtf8(const char* s)
{
    if (!s || !*s) return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (n <= 1) return std::wstring();
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
    w.resize((size_t)n - 1);
    return w;
}

void roundedPath(GraphicsPath& path, float x, float y, float w, float h, float r)
{
    const float d = r * 2;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();
}

// Ritkommandona (Canvas) med GDI+
struct GdiCanvas : Canvas
{
    Graphics& g;
    const FontFamily& ff;
    GdiCanvas(Graphics& graphics, const FontFamily& family) : g(graphics), ff(family) {}

    void fillRect(float x, float y, float w, float h, Rgba c) override
    {
        // Rena pixlar: ingen kantutjamning, ingen halvpixelförskjutning
        SmoothingMode sm = g.GetSmoothingMode();
        PixelOffsetMode po = g.GetPixelOffsetMode();
        g.SetSmoothingMode(SmoothingModeNone);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        SolidBrush b(toColor(c));
        g.FillRectangle(&b, (REAL)floorf(x + 0.5f), (REAL)floorf(y + 0.5f),
                        (REAL)floorf(w + 0.5f), (REAL)floorf(h + 0.5f));
        g.SetSmoothingMode(sm);
        g.SetPixelOffsetMode(po);
    }
    void gradientV(float x, float y, float w, float h, Rgba top, Rgba bottom) override
    {
        LinearGradientBrush b(PointF(x, y), PointF(x, y + h), toColor(top), toColor(bottom));
        g.FillRectangle(&b, x, y, w, h);
    }
    void line(float x0, float y0, float x1, float y1, Rgba c, float width) override
    {
        Pen p(toColor(c), width);
        g.DrawLine(&p, x0, y0, x1, y1);
    }
    void roundRect(float x, float y, float w, float h, float r, Rgba fill, Rgba edge, float edgeW) override
    {
        if (r < 0.5f) {
            SmoothingMode sm = g.GetSmoothingMode();
            PixelOffsetMode po = g.GetPixelOffsetMode();
            g.SetSmoothingMode(SmoothingModeNone);
            g.SetPixelOffsetMode(PixelOffsetModeHalf);
            if (fill) { SolidBrush b(toColor(fill)); g.FillRectangle(&b, x, y, w, h); }
            if (edge) { Pen p(toColor(edge), edgeW); g.DrawRectangle(&p, x, y, w, h); }
            g.SetSmoothingMode(sm);
            g.SetPixelOffsetMode(po);
            return;
        }
        GraphicsPath path;
        roundedPath(path, x, y, w, h, r);
        if (fill) { SolidBrush b(toColor(fill)); g.FillPath(&b, &path); }
        if (edge) { Pen p(toColor(edge), edgeW); g.DrawPath(&p, &path); }
    }
    void ellipse(float cx, float cy, float rx, float ry, Rgba fill, Rgba edge, float edgeW) override
    {
        if (fill) { SolidBrush b(toColor(fill)); g.FillEllipse(&b, cx - rx, cy - ry, rx * 2, ry * 2); }
        if (edge) { Pen p(toColor(edge), edgeW); g.DrawEllipse(&p, cx - rx, cy - ry, rx * 2, ry * 2); }
    }
    void arc(float cx, float cy, float r, float startDeg, float sweepDeg, Rgba c, float width) override
    {
        Pen p(toColor(c), width);
        g.DrawArc(&p, cx - r, cy - r, r * 2, r * 2, startDeg, sweepDeg);
    }
    void polygon(const float* xy, int n, Rgba fill) override
    {
        if (n < 3) return;
        std::vector<PointF> pts((size_t)n);
        for (int i = 0; i < n; ++i) pts[(size_t)i] = PointF(xy[i * 2], xy[i * 2 + 1]);
        SolidBrush b(toColor(fill));
        g.FillPolygon(&b, pts.data(), n);
    }
    void text(float x, float y, float w, const char* s, float size, int style, Rgba c, int align) override
    {
        const std::wstring ws = wideFromUtf8(s);
        if (ws.empty()) return;
        Font font(&ff, size, fontStyle(style), UnitPixel);
        SolidBrush brush(toColor(c));
        const StringFormat plain;
        // Loggan (stor text) ritas utan GDI+:s sidmarginal, som i TruePan
        StringFormat fmt(size >= 30.0f ? StringFormat::GenericTypographic() : &plain);
        fmt.SetAlignment(align == kAlignCenter ? StringAlignmentCenter : (align == kAlignFar ? StringAlignmentFar : StringAlignmentNear));
        RectF r(x, y, w, 4000.0f);
        g.DrawString(ws.c_str(), -1, &font, r, &fmt, &brush);
    }
    float textHeight(const char* s, float w, float size, int style) override
    {
        const std::wstring ws = wideFromUtf8(s);
        if (ws.empty()) return size * 1.3f;
        Font font(&ff, size, fontStyle(style), UnitPixel);
        StringFormat fmt;
        RectF layout(0, 0, w, 100000.0f), box;
        g.MeasureString(ws.c_str(), -1, &font, layout, &fmt, &box);
        return box.Height;
    }
    float textWidth(const char* s, float size, int style) override
    {
        const std::wstring ws = wideFromUtf8(s);
        if (ws.empty()) return 0.0f;
        Font font(&ff, size, fontStyle(style), UnitPixel);
        StringFormat fmt(StringFormat::GenericTypographic());
        fmt.SetFormatFlags(fmt.GetFormatFlags() | StringFormatFlagsMeasureTrailingSpaces | StringFormatFlagsNoWrap);
        RectF layout(0, 0, 10000.0f, 1000.0f), box;
        g.MeasureString(ws.c_str(), -1, &font, layout, &fmt, &box);
        return box.Width;
    }
    void clip(float x, float y, float w, float h) override { g.SetClip(RectF(x, y, w, h), CombineModeReplace); }
    void unclip() override { g.ResetClip(); }

    static int fontStyle(int style)
    {
        switch (style) {
        case kStyleBold: return FontStyleBold;
        case kStyleItalic: return FontStyleItalic;
        case kStyleBoldItalic: return FontStyleBold | FontStyleItalic;
        default: return FontStyleRegular;
        }
    }
};

// Kopplar gränssnittet till kärnan
struct CoreGuiHost : GuiHost
{
    Core* core;
    explicit CoreGuiHost(Core* c) : core(c) {}
    float get(int i) override { return (i >= 0 && i < kNumParams) ? core->params[i].load() : 0.0f; }
    void set(int i, float v) override { core->setParamFromEditor(i, v); }
    void beginEdit(int i) override { core->beginEdit(i); }
    void endEdit(int i) override { core->endEdit(i); }
    const char* presetName() override { return core->presetName; }
    int playPos() override { return core->playPos.load(); }
    void takePeaks(float& l, float& r) override { l = core->peakL.exchange(0.0f); r = core->peakR.exchange(0.0f); }
};

struct Editor
{
    Core* plugin = nullptr;
    HWND hwnd = nullptr;
    CoreGuiHost host;
    StepGui gui;
    HWND prevFocus = nullptr;   // fönstret som hade fokus innan musen kom in
    bool tracking = false;

    explicit Editor(Core* c) : plugin(c), host(c), gui(&host) {}
};

void paintTo(Editor* ed, Graphics& out, int width, int height)
{
    Bitmap buffer(width, height, PixelFormat32bppPARGB);
    {
        Graphics g(&buffer);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        FontFamily segoe(L"Segoe UI");   // finns i Windows 7
        FontFamily arial(L"Arial");      // reserv
        const FontFamily& ff = segoe.IsAvailable() ? segoe : arial;
        GdiCanvas canvas(g, ff);
        ed->gui.draw(canvas, (uint32_t)GetTickCount());
    }
    out.SetInterpolationMode(InterpolationModeNearestNeighbor);
    out.SetPixelOffsetMode(PixelOffsetModeHalf);
    out.DrawImage(&buffer, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel);
}

void paint(Editor* ed, HDC hdc, int width, int height)
{
    if (width <= 0 || height <= 0) return;
    Graphics screen(hdc);
    paintTo(ed, screen, width, height);
}

// --- Presetmapp: Dokument\TrueStep Presets (skapas automatiskt) ------------
std::wstring presetFolder()
{
    wchar_t docs[MAX_PATH];
    if (SHGetFolderPathW(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, 0, docs) != S_OK) return std::wstring();
    std::wstring dir = std::wstring(docs) + L"\\TrueStep Presets";
    CreateDirectoryW(dir.c_str(), nullptr);   // gör inget om mappen redan finns
    return dir;
}

bool fileExists(const std::wstring& path)
{
    const DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring widen(const std::string& s)
{
    std::wstring w;
    for (char c : s) w += (wchar_t)(unsigned char)c;
    return w;
}

// Filnamnet utan mapp och filändelse, som säkert presetnamn.
std::string presetNameFromPath(const wchar_t* path)
{
    const wchar_t* base = path;
    for (const wchar_t* c = path; *c; ++c) if (*c == L'\\' || *c == L'/') base = c + 1;
    std::string out;
    for (const wchar_t* c = base; *c; ++c) {
        if (*c == L'.' && wcschr(c + 1, L'.') == nullptr) break;   // sista punkten = filändelse
        out += (*c >= 32 && *c < 127) ? (char)*c : '_';
    }
    return cleanPresetName(out);
}

std::vector<std::wstring> listUserPresets(const std::wstring& dir)
{
    std::vector<std::wstring> files;
    if (dir.empty()) return files;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*.tsp").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return files;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) files.push_back(dir + L"\\" + fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(files.begin(), files.end(), [](const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) < 0; });
    return files;
}

bool readPresetFile(const std::wstring& path, float* v)
{
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return false;
    std::string text;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0 && text.size() < 65536) text.append(buf, n);
    std::fclose(f);
    std::string name;
    return presetFromText(text, v, &name);
}

void presetsChanged(Editor* ed)
{
    ed->plugin->markEditorPreset();
    ed->plugin->hostUpdateDisplay();
    InvalidateRect(ed->hwnd, nullptr, FALSE);
}

bool loadUserPreset(Editor* ed, const std::wstring& path, bool showErrors)
{
    Core* p = ed->plugin;
    float v[kNumParams];
    if (!readPresetFile(path, v)) {
        if (showErrors) MessageBoxW(ed->hwnd, L"This file is not a TrueStep preset.", L"TrueStep", MB_OK | MB_ICONWARNING);
        return false;
    }
    p->applyValues(v, true);
    copyString(p->presetName, presetNameFromPath(path.c_str()).c_str(), sizeof p->presetName);
    p->program = -1;
    presetsChanged(ed);
    return true;
}

// Pilarna i PRESET: först fabrikspresetsen, sedan egna presets i mappen.
void stepPreset(Editor* ed, int step)
{
    Core* p = ed->plugin;
    const std::vector<std::wstring> user = listUserPresets(presetFolder());
    const int total = kNumFactoryPresets + (int)user.size();

    int pos = -1;
    if (p->program >= 0) pos = p->program;
    else {
        for (size_t i = 0; i < user.size(); ++i)
            if (presetNameFromPath(user[i].c_str()) == p->presetName) { pos = kNumFactoryPresets + (int)i; break; }
    }
    if (pos < 0) pos = step > 0 ? -1 : total;
    for (int tries = 0; tries < total; ++tries) {
        pos += step;
        if (pos < 0) pos = total - 1;
        if (pos >= total) pos = 0;
        if (pos < kNumFactoryPresets) { p->loadFactoryPreset(pos, true); presetsChanged(ed); return; }
        if (loadUserPreset(ed, user[(size_t)(pos - kNumFactoryPresets)], false)) return;   // hoppa över trasiga filer
    }
}

void savePresetFile(Editor* ed)
{
    Core* p = ed->plugin;
    const std::wstring dir = presetFolder();
    const std::string suggestion = uniquePresetName(p->presetName, [&](const std::string& n) {
        return !dir.empty() && fileExists(dir + L"\\" + widen(n) + L".tsp");
    });
    wchar_t file[MAX_PATH] = L"";
    const std::wstring sw = widen(suggestion);
    wcsncpy(file, sw.c_str(), MAX_PATH - 1);
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = ed->hwnd;
    ofn.lpstrFilter = L"TrueStep preset (*.tsp)\0*.tsp\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = dir.empty() ? nullptr : dir.c_str();
    ofn.lpstrDefExt = L"tsp";
    ofn.lpstrTitle = L"Save TrueStep preset";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&ofn)) return;

    float v[kNumParams];
    for (int i = 0; i < kNumParams; ++i) v[i] = p->params[i].load();
    const std::string name = presetNameFromPath(file);
    const std::string text = presetToText(name, v);
    FILE* f = _wfopen(file, L"wb");
    if (!f || std::fwrite(text.data(), 1, text.size(), f) != text.size()) {
        if (f) std::fclose(f);
        MessageBoxW(ed->hwnd, L"The preset could not be saved.", L"TrueStep", MB_OK | MB_ICONWARNING);
        return;
    }
    std::fclose(f);
    copyString(p->presetName, name.c_str(), sizeof p->presetName);
    p->program = -1;
    presetsChanged(ed);
}

void loadPresetFile(Editor* ed)
{
    const std::wstring dir = presetFolder();
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = ed->hwnd;
    ofn.lpstrFilter = L"TrueStep preset (*.tsp)\0*.tsp\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = dir.empty() ? nullptr : dir.c_str();
    ofn.lpstrTitle = L"Load TrueStep preset";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return;
    loadUserPreset(ed, file, true);
}

void runAction(Editor* ed, StepGui::Action a)
{
    switch (a) {
    case StepGui::kActPresetPrev: stepPreset(ed, -1); break;
    case StepGui::kActPresetNext: stepPreset(ed, +1); break;
    case StepGui::kActSave: savePresetFile(ed); break;
    case StepGui::kActLoad: loadPresetFile(ed); break;
    default: break;
    }
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Editor* ed = reinterpret_cast<Editor*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ed = static_cast<Editor*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)ed);
        ed->hwnd = hwnd;
        SetTimer(hwnd, 1, 33, nullptr);   // ~30 bilder/s
        return 0;
    }
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (ed) {
            RECT rc; GetClientRect(hwnd, &rc);
            paint(ed, hdc, rc.right - rc.left, rc.bottom - rc.top);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SETCURSOR:
        if (ed && LOWORD(lParam) == HTCLIENT) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
            LPCSTR cur = IDC_ARROW;
            switch (ed->gui.cursorAt(pt.x, pt.y)) {
            case kCurHand: cur = IDC_HAND; break;
            case kCurSizeWE: cur = IDC_SIZEWE; break;
            case kCurSizeNS: cur = IDC_SIZENS; break;
            case kCurSizeAll: cur = IDC_SIZEALL; break;
            default: break;
            }
            SetCursor(LoadCursor(nullptr, cur));
            return TRUE;
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        if (!ed) return 0;
        const int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
        const StepGui::Action a = ed->gui.mouseDown(x, y, msg == WM_LBUTTONDBLCLK);
        if (ed->gui.dragging()) SetCapture(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        if (a != StepGui::kActNone) runAction(ed, a);
        return 0;
    }
    case WM_MOUSELEAVE:
        // Musen lämnade fönstret: ge tillbaka tangentbordsfokus till värdprogrammet
        if (ed) {
            ed->tracking = false;
            if (GetFocus() == hwnd && ed->prevFocus && IsWindow(ed->prevFocus)) SetFocus(ed->prevFocus);
            ed->prevFocus = nullptr;
        }
        return 0;
    case WM_MOUSEMOVE:
        // Windows 7 skickar mushjulet till fönstret som har fokus: ta fokus medan musen
        // är över TrueStep, så att hjulet fungerar (manualen och rattarna).
        if (ed && !ed->tracking) {
            HWND f = GetFocus();
            if (f != hwnd) { ed->prevFocus = f; SetFocus(hwnd); }
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof tme;
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            if (TrackMouseEvent(&tme)) ed->tracking = true;
        }
        if (ed) {
            ed->gui.mouseMove((short)LOWORD(lParam), (short)HIWORD(lParam), (wParam & MK_SHIFT) != 0);
            if (ed->gui.dragging()) InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONUP:
        if (ed) {
            ed->gui.mouseUp();
            if (GetCapture() == hwnd) ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (ed && lParam == 0) ed->gui.mouseUp();   // musen släpptes utanför oss
        return 0;
    case WM_MOUSEWHEEL:
        if (ed) {
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            const int clicks = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            if (clicks != 0) ed->gui.wheel(pt.x, pt.y, clicks);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

namespace truestep {

void editorOpen(Core* p, void* parent)
{
    if (p->editor || !parent) return;

    if (!g_instance) {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&windowProc), &g_instance);
    }
    if (g_gdiplusUsers++ == 0) {
        GdiplusStartupInput input;
        GdiplusStartup(&g_gdiplusToken, &input, nullptr);
    }
    if (!g_classRegistered) {
        WNDCLASSW wc = {};
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = windowProc;
        wc.hInstance = g_instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = kWindowClass;
        if (RegisterClassW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) g_classRegistered = true;
    }

    Editor* ed = new (std::nothrow) Editor(p);
    if (!ed) return;
    p->editor = ed;
    CreateWindowExW(0, kWindowClass, L"TrueStep", WS_CHILD | WS_VISIBLE,
                    0, 0, kEditorWidth, kEditorHeight,
                    static_cast<HWND>(parent), nullptr, g_instance, ed);
}

void editorClose(Core* p)
{
    Editor* ed = static_cast<Editor*>(p->editor);
    if (!ed) return;
    if (ed->hwnd) DestroyWindow(ed->hwnd);
    delete ed;
    p->editor = nullptr;
    if (--g_gdiplusUsers == 0) {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

// Ritar fönstret (framsida eller baksida) till en PNG-fil utan att öppna något
// fönster. Används av RenderGui (bilder av gränssnittet i bygget).
bool editorRenderPng(Core* core, bool backSide, const wchar_t* path)
{
    GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &input, nullptr) != Ok) return false;
    bool ok = false;
    {
        Editor ed(core);
        ed.gui.showBack = backSide;
        Bitmap bmp(kEditorWidth, kEditorHeight, PixelFormat32bppPARGB);
        {
            Graphics g(&bmp);
            paintTo(&ed, g, kEditorWidth, kEditorHeight);
            paintTo(&ed, g, kEditorWidth, kEditorHeight);   // andra bilden: loggans mått är då kända
        }
        UINT num = 0, size = 0;
        GetImageEncodersSize(&num, &size);
        if (size > 0) {
            std::vector<unsigned char> buf(size);
            ImageCodecInfo* codecs = reinterpret_cast<ImageCodecInfo*>(buf.data());
            GetImageEncoders(num, size, codecs);
            for (UINT i = 0; i < num; ++i) {
                if (wcscmp(codecs[i].MimeType, L"image/png") == 0) {
                    ok = bmp.Save(path, &codecs[i].Clsid, nullptr) == Ok;
                    break;
                }
            }
        }
    }
    GdiplusShutdown(token);
    return ok;
}

} // namespace truestep
