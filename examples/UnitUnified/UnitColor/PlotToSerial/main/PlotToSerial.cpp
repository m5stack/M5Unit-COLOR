/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitColor
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedCOLOR.h>
#include <M5Utility.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

using namespace m5::unit::tcs3472x;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitColor unit;

float atime{}, max_lux{};
Gain gc{};
uint16_t saturation{};

// ************************************************************************
// Self calibration
// NOTICE:
// Set the appropriate value for your measurement environment.
// Values vary widely depending on ambient light.
// Also, when using this correction, the value of Clear is ignored.
// ************************************************************************
#if 0
Calibration calib{
    199, 11534,  // R black-white raw range
    221, 22680,  // G black-white raw range
    251, 26840,  // B black-white raw range
};
#else
Calibration calib{
    0x0075, 0x0AFE,  // R black-white raw range
    0x00A1, 0x15A6,  // G black-white raw range
    0x00AF, 0x194D   // B black-white raw range
};
#endif

// Gamma table
constexpr float gamma_value{2.5f};  // Set the appropriate value for your measurement environment
#if defined(M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CAN_CONSTEXPR_MAKE_GAMMA_TABLE)
constexpr auto gammaTable = make_gamma_table(gamma_value);
#else
std::array<uint8_t, 256> gammaTable{};
#endif

#if 0
// Pure hue: removes the white component (the smallest channel) and scales the largest remaining channel to 255,
// which gives the most saturated color of the same hue (e.g. a dull blue becomes a vivid blue)
uint16_t correction_for_blue(const Data& d)
{
    const int32_t r  = d.RnoIR8();
    const int32_t g  = d.GnoIR8();
    const int32_t b  = d.BnoIR8();
    const int32_t lo = std::min(std::min(r, g), b);
    const int32_t hi = std::max(std::max(r, g), b);
    if (hi == lo) {
        return Data::color565(r, g, b);  // Gray or white has no hue
    }
    const float scale = 255.0f / (hi - lo);
    return Data::color565(static_cast<uint8_t>((r - lo) * scale), static_cast<uint8_t>((g - lo) * scale),
                          static_cast<uint8_t>((b - lo) * scale));
}
#endif

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // Board-aware connection: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, unit) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }
    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    if (!unit.readGain(gc) || !unit.readAtime(atime)) {
        M5_LOGE("Failed to read");
        m5::unit::wiring::failStop();
    }
    saturation = calculateSaturation(atime);
    max_lux    = calculateMaxLux(atime, gc);
    M5_LOGI("ATIME:%f GAIN:%u SAT:%u MLUX:%f ", atime, m5::stl::to_underlying(gc), saturation, max_lux);

#if !defined(M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CAN_CONSTEXPR_MAKE_GAMMA_TABLE)
    gammaTable = make_gamma_table(gamma_value);
#endif

    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.fillScreen(TFT_BLACK);
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        const auto& oldest = unit.oldest();
        uint16_t colors[4]{};

        colors[0] = unit.RGB565();
        colors[1] = oldest.RGBnoIR565();
        colors[2] = Data::color565(calib.R8(oldest), calib.G8(oldest), calib.B8(oldest));
        colors[3] =
            Data::color565(gammaTable[calib.R8(oldest)], gammaTable[calib.G8(oldest)], gammaTable[calib.B8(oldest)]);

        if (!lcd.isEPD()) {
            bool narrow = lcd.width() < 200;
            lcd.startWrite();
            // Information
            // 1st : RGB
            // 2nd : RGB without IR
            // 3rd : Calibrated RGB
            // 4th : Gamma correction of calibrated values
            // 5th : RAW RGBC (wide display only)
            if (narrow) {
                // Short format for small screens (e.g. AtomS3 128x128)
                lcd.setCursor(0, 0 + 16 * 0);
                lcd.printf("RGB %3u,%3u,%3u", unit.R8(), unit.G8(), unit.B8());
                lcd.setCursor(0, 0 + 16 * 1);
                lcd.printf("noI %3u,%3u,%3u", oldest.RnoIR8(), oldest.GnoIR8(), oldest.BnoIR8());
                lcd.setCursor(0, 0 + 16 * 2);
                lcd.printf("Cal %3u,%3u,%3u", calib.R8(oldest), calib.G8(oldest), calib.B8(oldest));
                lcd.setCursor(0, 0 + 16 * 3);
                lcd.printf("Gam %3u,%3u,%3u", gammaTable[calib.R8(oldest)], gammaTable[calib.G8(oldest)],
                           gammaTable[calib.B8(oldest)]);
            } else {
                lcd.setCursor(16, 8 + 16 * 0);
                lcd.printf("    RGB(%3u,%3u,%3u)", unit.R8(), unit.G8(), unit.B8());
                lcd.setCursor(16, 8 + 16 * 1);
                lcd.printf("RGBnoIR(%3u,%3u,%3u)", oldest.RnoIR8(), oldest.GnoIR8(), oldest.BnoIR8());
                lcd.setCursor(16, 8 + 16 * 2);
                lcd.printf("RGBCalb(%3u,%3u,%3u)", calib.R8(oldest), calib.G8(oldest), calib.B8(oldest));
                lcd.setCursor(16, 8 + 16 * 3);
                lcd.printf("CalbGam(%3u,%3u,%3u)", gammaTable[calib.R8(oldest)], gammaTable[calib.G8(oldest)],
                           gammaTable[calib.B8(oldest)]);
                lcd.setCursor(16, 8 + 16 * 4);
                lcd.printf("RAW:(%04X,%04X,%04X) %04X", oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());
            }
            lcd.endWrite();

            // Color bar (1:RGB / 2:RGB without IR / 3:Calibrated / 4:Gamma)
            int32_t y = narrow ? 16 * 4 : 8 + 16 * 5;
            int32_t h = (lcd.height() - y) / 4;
            if (h > 16) {
                h = 16;
            }
            for (uint32_t i = 0; i < 4; ++i) {
                lcd.drawRect(0, y + h * i, lcd.width(), h - 1, TFT_WHITE);
                lcd.fillRect(1, y + h * i + 1, lcd.width() - 2, h - 3, colors[i]);
            }
        }

        // Serial
        auto lux    = calculateLux(oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16(), atime, gc);
        auto ct     = calculateColorTemperature(oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());
        auto cratio = calculateCRATIO(oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());
        if (oldest.C16() >= saturation) {
            M5_LOGW("Detect saturation");
            lux = ct = cratio = 0;
        }
        M5.Log.printf("RGB(%3u,%3u,%3u) RGBC:%04X,%04X,%04X,%04X Sat?:%u IR:%d Lux:%.2f CTemp:%.2f CRATIO:%.2f\n",
                      unit.R8(), unit.G8(), unit.B8(), oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16(),
                      (oldest.C16() >= saturation), static_cast<int>(oldest.IR()), lux, ct, cratio);
    }

    // Single shot
    if (M5.BtnA.wasClicked()) {
        Data d{};
        if (unit.stopPeriodicMeasurement() && unit.measureSingleshot(d)) {
            M5.Log.printf("\tSingle: RGB(%3u,%3u,%3u) RGBC:%04X,%04X,%04X,%04X\n", d.R8(), d.G8(), d.B8(), d.R16(),
                          d.G16(), d.B16(), d.C16());
            unit.startPeriodicMeasurement();
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
