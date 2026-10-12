/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitColor via UnitPaHub

  Board ---> PaHub ---> ch:0 UnitColor
  Prints the detected color to the serial output. When the board has a screen, the upper part is filled with the
  color and the lower part shows the channel, the nearest color name and the values.
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedCOLOR.h>
#include <M5UnitUnifiedHUB.h>                 // UnitPaHub
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitPaHub hub;
m5::unit::UnitColor unit;

constexpr uint8_t UNIT_CHANNEL{0};  // PaHub channel the unit is connected to

//! True when the board has a real screen (Atom / NanoC6 / NanoH2 / NessoN1 have none)
bool has_lcd{};

//! Two text lines at the bottom, drawn off-screen and pushed at once to avoid flicker
M5Canvas canvas(&lcd);
uint16_t latest_rgb565{};

void setup_screen()
{
    lcd.fillScreen(TFT_BLACK);
    // 6x8 font scaled so that 22 characters ("RGB 255 255 255 C:FFFF") fit the width
    const int32_t text_size = std::max<int32_t>(1, lcd.width() / (22 * 6));
    canvas.setPsram(false);
    canvas.setColorDepth(1);  // Two colors: background and text
    canvas.createSprite(lcd.width(), 8 * text_size * 2 + text_size * 4);
    canvas.setPaletteColor(0, TFT_BLACK);
    canvas.setPaletteColor(1, TFT_WHITE);
    canvas.setTextSize(text_size);
    canvas.setTextColor(1);
}

void draw_color(const m5::unit::tcs3472x::Data& d)
{
    const uint8_t r{d.R8()}, g{d.G8()}, b{d.B8()};
    const auto name = m5::unit::tcs3472x::nearestColorName(r, g, b);

    lcd.startWrite();
    // Repaint the color area only when the color changes
    const uint16_t rgb565{d.RGB565()};
    if (rgb565 != latest_rgb565) {
        latest_rgb565 = rgb565;
        lcd.fillRect(0, 0, lcd.width(), lcd.height() - canvas.height(), rgb565);
    }
    const int32_t margin = canvas.getTextSizeX() * 2;
    canvas.fillSprite(0);
    canvas.setCursor(margin, margin);
    canvas.printf("PaHub ch:%u %s\n", UNIT_CHANNEL, m5::unit::tcs3472x::colorNameToString(name));
    canvas.setCursor(margin, canvas.getCursorY());
    canvas.printf("RGB %3u %3u %3u C:%04X", r, g, b, d.C16());
    canvas.pushSprite(0, lcd.height() - canvas.height());
    lcd.endWrite();
}
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    if (!hub.add(unit, UNIT_CHANNEL)) {  // PaHub ch:0 -> UnitColor
        M5_LOGE("Failed to add children");
        m5::unit::wiring::failStop();
    }
    // Board-aware I2C for the PaHub: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire.
    // The unit is reached through the hub (added above), so only the hub is added here.
    if (!m5::unit::wiring::addI2C(Units, hub) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    // A board without a screen reports a 1x1 dummy display
    has_lcd = (lcd.width() > 8 && lcd.height() > 8);
    if (has_lcd) {
        setup_screen();
    }
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        const auto& d = unit.oldest();
        M5.Log.printf("RGB(%3u,%3u,%3u) RGBC:%04X,%04X,%04X,%04X\n", d.R8(), d.G8(), d.B8(), d.R16(), d.G16(), d.B16(),
                      d.C16());
        if (has_lcd && !lcd.isEPD()) {
            draw_color(d);
        }
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs: run loop() back-to-back, but every ~2 s yield a 5 ms slice so the IDLE task
// runs and feeds the task watchdog (default 5 s).
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
