// RenderGui.cpp - ritar TrueSteps fönster (fram- och baksida) till PNG-filer med
// exakt samma kod som pluginet. Körs i bygget; bilderna laddas upp separat.
#include "TrueStepCore.h"
#include <cstdio>

int main()
{
    truestep::Core core;                       // standardinställningar
    core.loadFactoryPreset(11, false);         // Hi-Hat Stutter: visar delade steg, PAN och RES
    const bool a = truestep::editorRenderPng(&core, false, L"TrueStep_front.png");
    const bool b = truestep::editorRenderPng(&core, true, L"TrueStep_back.png");
    std::printf("front %s, back %s\n", a ? "ok" : "FAILED", b ? "ok" : "FAILED");
    return (a && b) ? 0 : 1;
}
