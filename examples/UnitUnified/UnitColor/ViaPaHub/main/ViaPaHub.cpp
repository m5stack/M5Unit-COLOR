/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitColor via UnitPaHub

  Board ---> PaHub ---> ch:0 UnitColor
  Prints the detected color to the serial output, and fills the screen with it when the board has a screen.
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
        lcd.fillScreen(TFT_BLACK);
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
            lcd.fillScreen(d.RGB565());
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
