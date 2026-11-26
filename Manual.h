// Manual.h - texten på TrueSteps baksida (klicka på loggan).
// Engelska, som resten av gränssnittet. Varje rad är ett stycke:
//   kManHead = rubrik, kManBody = brödtext, kManNote = källa/notis (mindre).
// Texten är UTF-8 (\xC2\xB7 = mittpunkt, \xC2\xB0 = gradtecken).
#pragma once

enum { kManHead = 0, kManBody = 1, kManNote = 2 };

struct ManualPara
{
    int kind;
    const char* text;
};

static const ManualPara kManual[] = {
    { kManHead, "WHAT TrueStep DOES" },
    { kManBody, "TrueStep is a rhythmic step filter and gate. It follows the tempo and song position of your host and divides every bar "
                "into 16 steps (16th notes). Each step has its own gate length, highpass and lowpass filter, resonance and stereo "
                "position, and any step can be split into two 32nd notes that have settings of their own. Drag the edges of the bars in "
                "the STEP GRID to shape the sound of every step." },
    { kManBody, "When the host is not playing, the sound passes through unchanged (OUTPUT still applies). Every setting can be automated "
                "and is saved with your project." },

    { kManHead, "THE STEP GRID" },
    { kManBody, "Each column is one step of the bar, from 1 to 16. The bar in a column shows what is let through: its width is the gate "
                "length, its top edge is the lowpass cutoff and its bottom edge is the highpass cutoff. Everything between the edges "
                "passes, the rest is filtered away. The vertical scale is 20 Hz to 20 kHz (logarithmic) with lines at 100 Hz, 1 kHz and "
                "10 kHz. The step that is playing right now is highlighted in orange." },
    { kManBody, "Drag the right edge for LENGTH, the top edge for LOWPASS, the bottom edge for HIGHPASS, and the middle of the bar to move "
                "both filters together. Click above a bar to move the lowpass edge there, below it to move the highpass edge, or to the "
                "right of it to set the length. Double-click a column to reset it." },

    { kManHead, "GATE LENGTH" },
    { kManBody, "LENGTH is how much of the step the gate is open: 0 % is closed, 100 % is open for the whole step. The gate opens and "
                "closes in about 2 ms, so it does not click. Several steps at 100 % in a row give a continuous sound." },

    { kManHead, "LOWPASS AND HIGHPASS" },
    { kManBody, "Both filters are 24 dB per octave (4th order Butterworth). With the lowpass edge at the top and the highpass edge at the "
                "bottom the filters are switched off and the sound is not filtered at all. The lowpass edge cannot go below the highpass "
                "edge. Changes between steps glide in about 5 ms." },

    { kManHead, "RESONANCE (RES)" },
    { kManBody, "Nine levels, 0 to 8. Resonance emphasizes the frequencies around the lowpass cutoff, like the squelch of a classic "
                "acid bass synthesizer. Level 0 is flat, level 8 gives a boost of about 15 dB at the cutoff. It is heard when the lowpass "
                "edge is moved down from the top." },

    { kManHead, "1/32 SPLIT" },
    { kManBody, "The 1/32 button under a step splits it into two halves (two 32nd notes). Each half gets its own bar, filter, resonance and "
                "pan. When you split a step, the second half starts as a copy of the first. Press the button again to join the halves; "
                "the settings of the second half are kept in case you split the step again." },

    { kManHead, "PAN" },
    { kManBody, "Nine positions per step: 50L, 36L, 22L, 10L, C, 10R, 22R, 36R and 50R. C leaves the stereo image as it is." },
    { kManBody, "TrueStep uses true stereo constant power panning. The position p (-1 to +1 = step / 50) is applied to the left and the right "
                "input channel separately. For p <= 0 the left channel is placed at -1 and the right channel at 1 + 2p; for p > 0 the right "
                "channel is placed at +1 and the left channel at -1 + 2p. Each channel then goes to master left and right with the gains "
                "cos((pan + 1) x pi / 4) and sin((pan + 1) x pi / 4). The whole stereo image is moved instead of just balanced." },
    { kManNote, "Note: at 50L and 50R both input channels end up on one side and add together, so a mono sound is up to 6 dB louder there. "
                "Use OUTPUT to compensate if you need to. The dry part of DRY/WET is not panned." },

    { kManHead, "DRY/WET AND OUTPUT" },
    { kManBody, "DRY/WET mixes the original sound (0 %) with the processed sound (100 %). OUTPUT sets the level after the effect, from -24 to "
                "+12 dB (0 dB = unchanged). OVER 0 dB lights up red if the output goes over 0 dBFS. The dot in the top right corner "
                "lights up when sound is passing." },
    { kManBody, "Knobs: drag up/down, hold Shift for fine adjustment, use the mouse wheel for small steps and double-click to return to the "
                "default value." },

    { kManHead, "HUMANIZE" },
    { kManBody, "The small knob next to DRY/WET moves the start of every step a little earlier or later than the grid, up to 25 ms, "
                "for a looser, more human feel. 0 ms is off. The shifts are not random jumps: they follow pink (1/f) noise, so "
                "they drift smoothly from step to step, the way the timing of a real musician does (Hennig et al., 2011, "
                "\"The nature and perception of fluctuations in human musical rhythms\"). Most steps stay close to the grid "
                "and only a few move far from it. The pattern is fixed to the position in the song, so playback and export "
                "sound the same every time. HUMANIZE adds no delay. It moves the gate and the filters in time, not the sound." },

    { kManHead, "S/H (SAMPLE AND HOLD)" },
    { kManBody, "The classic random filter of the 1980s. The S/H knob (next to HUMANIZE) gives every step its own random cutoff, "
                "which is held for the whole step and changes at the next one. 0 % is off. Your own bars stay the starting point: "
                "S/H moves the lowpass up and down around them (and the highpass too, if you use it), so a rhythm you built keeps "
                "its shape. In a split step each 1/32 gets its own value. The values are tied to the position in the song, so playback "
                "and export sound the same every time, but they never repeat from bar to bar." },

    { kManHead, "INFO BOX" },
    { kManBody, "When you rest the mouse pointer on a knob, a button, a bar or a heading, a short explanation appears in the box at the top "
                "of the window. It follows the language you chose for the manual (ENG | SWE). If you get tired of it, switch it off "
                "with INFO ON | OFF on this page, next to ENG | SWE." },

    { kManHead, "RANDOMIZE" },
    { kManBody, "The RANDOMIZE button under HUMANIZE invents a whole new pattern: step lengths, filters, RES, PAN and 1/32 splits. "
                "DRY/WET, OUTPUT and HUMANIZE are not changed. The patterns are made to play well: most steps are heard, the filter "
                "always lets sound through, and RES is only used on longer steps, where it can be heard. Every pattern is saved "
                "automatically as Random 0001, Random 0002 and so on in Documents\\TrueStep Presets\\Random, so you can get a good one "
                "back with LOAD, or with the square bank button next to RANDOMIZE: press it to show Random bank 1 in the PRESET box, "
                "then step through the patterns with the arrows. Press again for the next bank; after the last bank the normal presets return. After 9999 patterns a new folder (Random 2, Random 3 ...) is started." },

    { kManHead, "PRESETS" },
    { kManBody, "TrueStep comes with 12 factory presets, which also appear in the preset list of your host. The arrows step through the "
                "factory presets and then your own. SAVE stores your own preset (.tsp) in the folder Documents\\TrueStep Presets, which is "
                "created automatically. LOAD opens a preset file." },

    { kManHead, "SYNC AND TEMPO" },
    { kManBody, "TrueStep needs tempo and position from the host, so the steps lock to the song, to loops and to jumps in the timeline. "
                "One bar of 4/4 is 16 steps. Pattern length is always one bar; use the host's tempo to change the speed." },

    { kManHead, "TIPS" },
    { kManBody, "Try 50 % length on all steps for a classic trance gate. Move the lowpass edge down and add resonance on every second "
                "step for a rhythmic wah. Split the last step of a bar for a quick roll. Put TruePan after TrueStep for even more movement "
                "in the stereo field." },

    { kManNote, "The VST2 version of TrueStep is built with its own interface (no third-party SDK). The VST3 version uses the VST3 SDK from Steinberg (MIT licence). TrueStep runs on Windows 7 and later." },
};

// --- Svenska -----------------------------------------------------------------
static const ManualPara kManualSv[] = {
    { kManHead, "VAD TrueStep GÖR" },
    { kManBody, "TrueStep är ett rytmiskt stegfilter och en gate. Det följer tempot och låtens position i värdprogrammet och delar "
                "varje takt i 16 steg (16-delar). Varje steg har egen gate-längd, highpass- och lowpass-filter, resonans och "
                "stereoposition, och varje steg kan delas i två 32-delar med egna inställningar. Dra i kanterna på staplarna i "
                "STEP GRID för att forma ljudet i varje steg." },
    { kManBody, "När värdprogrammet inte spelar passerar ljudet oförändrat (OUTPUT gäller fortfarande). Alla inställningar kan "
                "automatiseras och sparas med ditt projekt." },

    { kManHead, "STEP GRID" },
    { kManBody, "Varje kolumn är ett steg i takten, från 1 till 16. Staplen i en kolumn visar vad som släpps igenom: bredden är "
                "gate-längden, överkanten är lowpass-gränsen och underkanten är highpass-gränsen. Allt mellan kanterna passerar, "
                "resten filtreras bort. Den lodräta skalan går från 20 Hz till 20 kHz (logaritmisk) med linjer vid 100 Hz, 1 kHz och "
                "10 kHz. Steget som spelas just nu lyser orange." },
    { kManBody, "Dra högerkanten för LENGTH, överkanten för LOWPASS, underkanten för HIGHPASS och mitten av staplen för att flytta "
                "båda filtren tillsammans. Klicka ovanför en stapel för att flytta lowpass-kanten dit, under den för att flytta "
                "highpass-kanten, eller till höger om den för att ställa in längden. Dubbelklicka på en kolumn för att nollställa den." },

    { kManHead, "GATE-LÄNGD" },
    { kManBody, "LENGTH anger hur stor del av steget som gaten är öppen: 0 % är stängd, 100 % är öppen under hela steget. Gaten "
                "öppnar och stänger på cirka 2 ms, så det klickar inte. Flera steg i rad med 100 % ger ett sammanhängande ljud." },

    { kManHead, "LOWPASS OCH HIGHPASS" },
    { kManBody, "Båda filtren är 24 dB per oktav (Butterworth av fjärde ordningen). Med lowpass-kanten överst och highpass-kanten "
                "underst är filtren avstängda och ljudet filtreras inte alls. Lowpass-kanten kan inte gå under highpass-kanten. "
                "Ändringar mellan stegen glider över på cirka 5 ms." },

    { kManHead, "RESONANS (RES)" },
    { kManBody, "Nio nivåer, 0 till 8. Resonansen betonar frekvenserna kring lowpass-gränsen, som den kvittrande klangen i en klassisk "
                "acid-basssynt. Nivå 0 är plan, nivå 8 ger en förstärkning på cirka 15 dB vid gränsen. Den hörs när lowpass-kanten "
                "har flyttats ner från toppen." },

    { kManHead, "1/32-DELNING" },
    { kManBody, "Knappen 1/32 under ett steg delar steget i två halvor (två 32-delar). Varje halva får egen stapel, eget filter, egen "
                "resonans och eget pan. När du delar ett steg börjar den andra halvan som en kopia av den första. Tryck på knappen "
                "igen för att slå ihop halvorna; den andra halvans inställningar sparas om du delar steget igen." },

    { kManHead, "PAN" },
    { kManBody, "Nio lägen per steg: 50L, 36L, 22L, 10L, C, 10R, 22R, 36R och 50R. C lämnar stereobilden som den är." },
    { kManBody, "TrueStep använder äkta stereo-panorering med konstant effekt. Positionen p (-1 till +1 = läge / 50) används på vänster och "
                "höger ingångskanal var för sig. För p <= 0 placeras vänster kanal på -1 och höger kanal på 1 + 2p; för p > 0 placeras "
                "höger kanal på +1 och vänster kanal på -1 + 2p. Varje kanal går sedan till master vänster och höger med förstärkningarna "
                "cos((pan + 1) x pi / 4) och sin((pan + 1) x pi / 4). Hela stereobilden flyttas i stället för att bara balanseras." },
    { kManNote, "Obs: vid 50L och 50R hamnar båda ingångskanalerna på samma sida och läggs ihop, så ett monoljud blir upp till 6 dB "
                "starkare där. Använd OUTPUT för att kompensera vid behov. Den torra delen av DRY/WET panoreras inte." },

    { kManHead, "DRY/WET OCH OUTPUT" },
    { kManBody, "DRY/WET blandar originalljudet (0 %) med det bearbetade ljudet (100 %). OUTPUT ställer in nivån efter effekten, från "
                "-24 till +12 dB (0 dB = oförändrat). OVER 0 dB lyser rött om utnivån går över 0 dBFS. Pricken uppe till höger lyser "
                "när ljud passerar." },
    { kManBody, "Rattar: dra uppåt/nedåt, håll Shift för finjustering, använd mushjulet för små steg och dubbelklicka för att gå "
                "tillbaka till standardvärdet." },

    { kManHead, "HUMANIZE" },
    { kManBody, "Den lilla ratten bredvid DRY/WET flyttar starten av varje steg lite tidigare eller senare än rutnätet, upp till 25 ms, "
                "för en lösare, mer mänsklig känsla. 0 ms är av. Förskjutningarna är inga slumpmässiga hopp: de följer rosa (1/f) brus, "
                "så de glider mjukt från steg till steg, som tajmingen hos en riktig musiker (Hennig m.fl., 2011, "
                "\"The nature and perception of fluctuations in human musical rhythms\"). De flesta steg ligger nära rutnätet och "
                "bara några få flyttas långt från det. Mönstret är knutet till positionen i låten, så uppspelning och export låter "
                "likadant varje gång. HUMANIZE ger ingen fördröjning. Det flyttar gaten och filtren i tid, inte ljudet." },

    { kManHead, "S/H (SAMPLE AND HOLD)" },
    { kManBody, "Det klassiska slumpfiltret från 1980-talet. Ratten S/H (bredvid HUMANIZE) ger varje steg en egen slumpad cutoff, som "
                "hålls under hela steget och ändras vid nästa. 0 % är av. Dina egna staplar är fortfarande utgångspunkten: S/H flyttar "
                "lowpass upp och ner kring dem (och highpass också, om du använder den), så ett rytmmönster du byggt behåller sin form. "
                "I ett delat steg får varje 1/32 ett eget värde. Värdena är knutna till positionen i låten, så uppspelning och export "
                "låter likadant varje gång, men de upprepas aldrig från takt till takt." },

    { kManHead, "INFORUTAN" },
    { kManBody, "När du låter muspekaren vila på en ratt, en knapp, en stapel eller en rubrik visas en kort förklaring i rutan högst upp "
                "i fönstret. Den följer språket du valt för manualen (ENG | SWE). Om du tröttnar på den stänger du av den med "
                "INFO ON | OFF på den här sidan, bredvid ENG | SWE." },

    { kManHead, "RANDOMIZE" },
    { kManBody, "Knappen RANDOMIZE under HUMANIZE hittar på ett helt nytt mönster: steglängder, filter, RES, PAN och 1/32-delningar. "
                "DRY/WET, OUTPUT, HUMANIZE och S/H ändras inte. Mönstren är gjorda för att fungera: de flesta steg hörs, filtret släpper "
                "alltid igenom ljud, och RES används bara på längre steg, där den hörs. Varje mönster sparas automatiskt som Random 0001, "
                "Random 0002 och så vidare i Documents\\TrueStep Presets\\Random, så att du kan få tillbaka ett bra mönster med LOAD, "
                "eller med den fyrkantiga bank-knappen bredvid RANDOMIZE: tryck på den för att visa Random bank 1 i PRESET-rutan och "
                "bläddra sedan bland mönstren med pilarna. Tryck igen för nästa bank; efter sista banken kommer de vanliga presetsen "
                "tillbaka. Efter 9999 mönster startas en ny mapp (Random 2, Random 3 ...)." },

    { kManHead, "PRESETS" },
    { kManBody, "TrueStep levereras med 12 fabrikspresets, som också syns i värdprogrammets presetlista. Pilarna bläddrar först bland "
                "fabrikspresetsen och sedan bland dina egna. SAVE sparar din egen preset (.tsp) i mappen Documents\\TrueStep Presets, "
                "som skapas automatiskt. LOAD öppnar en presetfil." },

    { kManHead, "SYNK OCH TEMPO" },
    { kManBody, "TrueStep behöver tempo och position från värdprogrammet, så att stegen låser mot låten, mot loopar och mot hopp i "
                "tidslinjen. En takt i 4/4 är 16 steg. Mönstret är alltid en takt långt; använd värdprogrammets tempo för att ändra "
                "hastigheten." },

    { kManHead, "TIPS" },
    { kManBody, "Prova 50 % längd på alla steg för en klassisk trance-gate. Flytta lowpass-kanten ner och lägg på resonans på vartannat "
                "steg för en rytmisk wah. Dela sista steget i en takt för en snabb rull. Lägg TruePan efter TrueStep för ännu mer "
                "rörelse i stereobilden." },

    { kManNote, "VST2-versionen av TrueStep är byggd med ett eget gränssnitt (ingen SDK från tredje part). VST3-versionen använder "
                "VST3-SDK från Steinberg (MIT-licens). TrueStep fungerar på Windows 7 och senare." },
};

enum { kLangEng = 0, kLangSwe = 1 };
inline const ManualPara* manualFor(int lang, int& count)
{
    if (lang == kLangSwe) { count = (int)(sizeof(kManualSv) / sizeof(kManualSv[0])); return kManualSv; }
    count = (int)(sizeof(kManual) / sizeof(kManual[0]));
    return kManual;
}
