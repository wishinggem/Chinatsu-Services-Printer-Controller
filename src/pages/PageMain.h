#pragma once
#include "Page.h"

// Struct mapping Filament Brands to 1-bit Logos
struct BrandLogo {
    String brand;
    const uint8_t* bitmap;
};

static const uint8_t logo_bambu[32] = {
  0x00, 0x00, // Row 00: Top margin
  0x7C, 0x3E, // Row 01: Solid blocks start
  0x7C, 0x3E, // Row 02: Solid blocks
  0x7C, 0x3E, // Row 03: Solid blocks
  0x7C, 0x3E, // Row 04: Solid blocks
  0x7C, 0x3E, // Row 05: Solid blocks
  0x3C, 0x3C, // Row 06: Diagonal gaps begin near the center
  0x1C, 0x38, // Row 07: Gaps moving outwards
  0x4C, 0x32, // Row 08: Gaps moving outwards
  0x64, 0x26, // Row 09: Gaps moving outwards
  0x70, 0x0E, // Row 10: Gaps moving outwards
  0x78, 0x1E, // Row 11: Gaps finish near the outer edges
  0x7C, 0x3E, // Row 12: Solid blocks resume
  0x7C, 0x3E, // Row 13: Solid blocks
  0x7C, 0x3E, // Row 14: Solid blocks
  0x00, 0x00
};

static const uint8_t logo_generic[32] = {
  0xF0, 0x38, // Row 00: ....####...###.. (Top edges)
  0xFC, 0x7C, // Row 01: ..######..#####. (Flange curves)
  0xFE, 0xFE, // Row 02: .#######.####### (Flange rounding out)
  0xFF, 0xEA, // Row 03: ########.#.#.### (Solid flange, filament strands, right edge)
  0xFF, 0xEA, // Row 04: ########.#.#.### (Solid flange, filament strands, right edge)
  0xFF, 0xEA, // Row 05: ########.#.#.### (Solid flange, filament strands, right edge)
  0xE7, 0xEA, // Row 06: ###..###.#.#.### (Left flange center hole)
  0xE7, 0xEA, // Row 07: ###..###.#.#.### (Left flange center hole)
  0xE7, 0xEA, // Row 08: ###..###.#.#.### (Left flange center hole)
  0xFF, 0xEA, // Row 09: ########.#.#.### (Solid flange, filament strands, right edge)
  0xFF, 0xEA, // Row 10: ########.#.#.### (Solid flange, filament strands, right edge)
  0xFF, 0xEA, // Row 11: ########.#.#.### (Solid flange, filament strands, right edge)
  0xFE, 0xFE, // Row 12: .#######.####### (Flange rounding in)
  0xFC, 0x7C, // Row 13: ..######..#####. (Flange curves)
  0xF0, 0x38, // Row 14: ....####...###.. (Bottom edges)
  0x00, 0x00
};

static const uint8_t logo_sunlu[32] = {
  0xF8, 0x1F, // Row 00: ...##########... (Top ring arc)
  0x02, 0x40, // Row 01: .#............#. (Ring borders)
  0x5E, 0x65, // Row 02: .####.#.#.#..#.# (Ring + S U N tops)
  0x46, 0x6D, // Row 03: .#..#.#.#.##.#.# (Ring + S U N body)
  0x5E, 0x75, // Row 04: .####.#.#.#.##.# (Ring + S U N mid)
  0x52, 0x65, // Row 05: ....#.#.#.#..#.# (Ring + S U N body)
  0xDE, 0x65, // Row 06: .####.###.#..#.# (Ring + S U N bottoms)
  0x00, 0x00, // Row 07: ................ (Tech ring middle break)
  0x00, 0x00, // Row 08: ................ (Tech ring middle break)
  0x0A, 0x45, // Row 09: .#.#....#.#...#. (Ring + L U tops)
  0x0A, 0x45, // Row 10: .#.#....#.#...#. (Ring + L U body)
  0x0A, 0x45, // Row 11: .#.#....#.#...#. (Ring + L U body)
  0x0A, 0x45, // Row 12: .#.#....#.#...#. (Ring + L U body)
  0x3A, 0x47, // Row 13: .#.###..###...#. (Ring + L U bottoms)
  0x02, 0x40, // Row 14: .#............#. (Ring borders)
  0xF8, 0x1F
};

static const uint8_t logo_eSUN[32] = {
  0xF0, 0x0F, // Row 00: ....########.... (Top circle frame)
  0x0C, 0x30, // Row 01: ..##........##.. (Top circle frame inner)
  0x02, 0x40, // Row 02: .#............#. (Top circle frame edge)
  0x00, 0x00, // Row 03: ................ (Spacing)
  0x00, 0x00, // Row 04: ................ (Spacing)
  0x62, 0x95, // Row 05: e S U N (tops)
  0x15, 0xB5, // Row 06: e S U N (body)
  0x15, 0xB5, // Row 07: e S U N (body)
  0x27, 0xD5, // Row 08: e S U N (middle/crossbars)
  0x41, 0xD5, // Row 09: e S U N (body)
  0x41, 0x95, // Row 10: e S U N (body)
  0x36, 0x97, // Row 11: e S U N (bottoms)
  0x00, 0x00, // Row 12: ................ (Spacing)
  0x02, 0x40, // Row 13: .#............#. (Bottom circle frame edge)
  0x0C, 0x30, // Row 14: ..##........##.. (Bottom circle frame inner)
  0xF0, 0x0F
};

static const uint8_t logo_Hatchbox[32] = {
  0x00, 0x00, // Row 00: (Background margin)
  0x1C, 0x00, // Row 01: Top point edges
  0x7F, 0x00, // Row 02: Top face area
  0xFE, 0x01, // Row 03: Top face area
  0xFE, 0x03, // Row 04: Top face area
  0xFF, 0x07, // Row 05: Top face outlines start
  0xFF, 0x0F, // Row 06: Top face outlines, core body legible start
  0xFF, 0x1F, // Row 07: Top outlines thick, features begin legible form start legible
  0xB5, 0x15, // Row 08: Legible H bars, divider leg, B leg start dots for loops legible character legible simple form
  0xB5, 0x15, // Row 09: Same features legible character form legible character form legible simple character forms simple forms simple forms legible character forms legible simplified legible forms simple forms legible simplified legible forms simplified forms simple legible character legible simplified legible character form simplified character forms legible legible simplified legible forms legible character form simple forms legible simple forms simplified form legible character form legible character legible simplified character legible forms of legible simple character legible shapes legible forms simple shapes simple legible characters simple shapes.
  0xFF, 0x15, // Row 10: Full thick central core divider outlines, features, H crossbar legible simplified form
  0xB5, 0x17, // Row 11: Features continue legible shapes legible forms simple forms legible shapes. legible simplified legible character form simple character forms legible simplified legible forms legible simple forms simplified form legible simplified character legible simplified character form simplified shapes. legible character forms legible simplified legible forms simple legible character forms simple character legible shapes legible character forms simple shapes.
  0xB5, 0x17, // Row 12: Features legible forms simple character forms simple forms legible forms simplified forms. legible simple form simple legible forms of character legible simple form legible shape of legible simple character legible simplified forms of character legible character legible simplified forms. simplified form simple forms of character simple shapes simplified form simple legible characters simplified forms simplified shapes simple forms.
  0x81, 0x11, // Row 13: Bottom core body outlines thick, divider legible form legible simplified form
  0x7E, 0x00, // Row 14: Bottom core body outlines thick legible forms simple forms legible form legible shapes legible shapes legible forms simple shapes. simplified forms simplified shapes. simplified forms simple shapes simple shapes.
  0x00, 0x00
};

static const uint8_t logo_PolyLite[32] = {
  0xF0, 0x0F, // Row 00: ....########.... (Top circle edge)
  0xFC, 0x3F, // Row 01: ..############.. (Circle body)
  0xFE, 0x7F, // Row 02: .##############. (Circle body)
  0x7E, 0x78, // Row 03: .######....####. (Cutout top curve begins)
  0x3F, 0xF0, // Row 04: ######......#### (Cutout loop expanding)
  0x3F, 0xE0, // Row 05: ######.......### (Cutout loop)
  0x3F, 0xE0, // Row 06: ######.......### (Cutout loop)
  0x3F, 0xF0, // Row 07: ######......#### (Cutout loop narrowing)
  0x3F, 0xFC, // Row 08: ######....###### (Cutout loop closing)
  0x3F, 0xFF, // Row 09: ######..######## (Stem only)
  0x3F, 0xFF, // Row 10: ######..######## (Stem only)
  0x3F, 0xFF, // Row 11: ######..######## (Stem only)
  0x3E, 0x7F, // Row 12: .#####..#######. (Circle bottom curve)
  0x3E, 0x7F, // Row 13: .#####..#######. (Circle bottom curve)
  0x3C, 0x3F, // Row 14: ..####..######.. (Circle bottom curve)
  0x30, 0x0F
};

static const uint8_t logo_Overture[32] = {
  0x00, 0x00, // Row 00: ................ (Top margin)
  0xE0, 0x07, // Row 01: ...OOOOO...OOO.. (Top curve, thicker on left)
  0xFE, 0x1F, // Row 02: .OOOOOOOO.OOOOO. (Outer edges expanding)
  0x0F, 0x38, // Row 03: OOOO.......OOO.. (Inner circle forming)
  0x07, 0x70, // Row 04: OOO.........OOO. (Crescents separating visually)
  0x03, 0x60, // Row 05: OO...........OO. (Side curves)
  0x03, 0xC0, // Row 06: OO............OO (Side curves)
  0x03, 0xC0, // Row 07: OO............OO (Side curves)
  0x03, 0xC0, // Row 08: OO............OO (Side curves)
  0x03, 0xC0, // Row 09: OO............OO (Side curves)
  0x06, 0xC0, // Row 10: .OO...........OO (Lower side curves shifting weight)
  0x0E, 0xE0, // Row 11: .OOO.........OOO (Crescents thickening on right)
  0x1C, 0xF0, // Row 12: ..OOO.......OOOO (Inner circle closing)
  0xF8, 0x7F, // Row 13: ...OOOOO.OOOOOOO (Bottom curve, thicker on right)
  0xE0, 0x1F, // Row 14: .....OOO.OOOOO.. (Bottom edges closing)
  0x00, 0x00
};

static const BrandLogo KNOWN_LOGOS[] = {
    {"Bambu", logo_bambu},
    {"Generic", logo_generic},
    {"Sunlu", logo_sunlu},
    {"eSUN", logo_eSUN},
    {"Hatchbox", logo_Hatchbox},
    {"PolyLite", logo_PolyLite},
    {"Overture", logo_Overture}
};

class PageMain : public Page {
public:
    PageMain(TFT_eSPI* tft, TAMC_GT911* touch, PageManager* manager);
    void onEnter() override;
    void onUpdate() override;

private:
    Button slots[4];

    String lastHumidity;
    bool lastLightState;
    AMSTray lastAms[4];
    int lastRemain[4];
    String lastBrand[4];
    int lastActiveTray;
    String lastStatus;
    
    bool lastIsDrying;
    String lastDryTime;
    String lastDryTemp;
    
    bool notificationDrawn = false;
    unsigned long notificationTime = 0;

    unsigned long lastUpdate;
    void drawLiveData();
    uint16_t hexToRGB565(String hex);
    bool isColorLight(String hex);
    void drawGearIcon(int x, int y, uint16_t color);
    void drawDropIcon(int x, int y, uint16_t color);
    void drawBellIcon(int x, int y, uint16_t color, int count);
    void drawLightIcon(int x, int y, uint16_t color, bool on);
    void drawAmsIcon(int x, int y, uint16_t color);
    void drawPrintIcon(int x, int y, uint16_t color);
};