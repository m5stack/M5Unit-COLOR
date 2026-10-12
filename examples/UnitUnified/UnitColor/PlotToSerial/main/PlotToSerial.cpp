/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitColor

  Shows the detected color as 4 color bars, and outputs it to the serial.
  Bar 4 shows the calibrated color through a selectable correction:
    Gamma -> Pure hue -> Brightness normalization -> Color name -> Grayscale
  BtnA click: Single shot measurement
  BtnA hold : Switch the correction of bar 4
  BtnB hold : Calibrate black (cover the sensor)
  BtnC hold : Calibrate white (put a white paper over the sensor)
  (CoreS3 / Tab5: the touch buttons at the bottom of the screen. Boards without BtnB / BtnC use the values below)
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
// Set the appropriate value for your measurement environment, or hold BtnB / BtnC to record black / white.
// Values vary widely depending on ambient light, gain and integration time.
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
// The sensor values are linear in light, but the display expects gamma-encoded (sRGB) values,
// so encode them with 1/2.2 to make them look right on the screen
constexpr float gamma_value{1.0f / 2.2f};
#if defined(M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CAN_CONSTEXPR_MAKE_GAMMA_TABLE)
constexpr auto gammaTable = make_gamma_table(gamma_value);
#else
std::array<uint8_t, 256> gammaTable{};
#endif

// Correction applied to the calibrated color on bar 4 (switched by holding BtnA)
enum class Correction : uint8_t { Gamma, PureHue, Brightness, Name, Gray };
constexpr uint8_t correction_count{5};
constexpr const char* correction_label[correction_count]       = {"CalbGam", "PureHue", "Bright", "ColName", "Gray"};
constexpr const char* correction_short_label[correction_count] = {"Gam", "Hue", "Bri", "Nam", "Gry"};
Correction correction{Correction::Gamma};

// Latest measurement, used for the calibration by BtnB / BtnC
Data latest{};

// Record the latest raw values (without IR) as black or white. Rejected if black would not be below white
void calibrate(const bool white)
{
    const uint16_t r{latest.RnoIR16()}, g{latest.GnoIR16()}, b{latest.BnoIR16()};
    Calibration c{calib};
    if (white) {
        c.whiteR = r;
        c.whiteG = g;
        c.whiteB = b;
    } else {
        c.blackR = r;
        c.blackG = g;
        c.blackB = b;
    }
    if (c.whiteR <= c.blackR || c.whiteG <= c.blackG || c.whiteB <= c.blackB) {
        M5_LOGW("Calibration rejected: white must be brighter than black on all channels");
        return;
    }
    calib = c;
    M5_LOGI("Calibrated %s: R %04X-%04X G %04X-%04X B %04X-%04X", white ? "white" : "black", calib.blackR, calib.whiteR,
            calib.blackG, calib.whiteG, calib.blackB, calib.whiteB);
}

RGB8 apply_correction(const uint8_t r, const uint8_t g, const uint8_t b)
{
    switch (correction) {
        case Correction::PureHue:
            return pureHue(r, g, b);
        case Correction::Brightness:
            return normalizeBrightness(r, g, b);
        case Correction::Name:
            return colorNameToRGB(nearestColorName(r, g, b));
        case Correction::Gray:
            return grayscale(r, g, b);
        default:
            return RGB8{gammaTable[r], gammaTable[g], gammaTable[b]};
    }
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
        latest             = oldest;
        const uint8_t cr   = calib.R8(oldest);
        const uint8_t cg   = calib.G8(oldest);
        const uint8_t cb   = calib.B8(oldest);
        const RGB8 corrected{apply_correction(cr, cg, cb)};
        const auto ci = m5::stl::to_underlying(correction);
        uint16_t colors[4]{};

        colors[0] = oldest.RGB565();
        colors[1] = oldest.RGBnoIR565();
        colors[2] = Data::color565(cr, cg, cb);
        colors[3] = corrected.to565();

        if (!lcd.isEPD()) {
            bool narrow = lcd.width() < 200;
            lcd.startWrite();
            // Information
            // 1st : RGB
            // 2nd : RGB without IR
            // 3rd : Calibrated RGB
            // 4th : Calibrated values through the selected correction
            // 5th : RAW RGBC (wide display only)
            if (narrow) {
                // Short format for small screens (e.g. AtomS3 128x128)
                lcd.setCursor(0, 0 + 16 * 0);
                lcd.printf("RGB %3u,%3u,%3u", oldest.R8(), oldest.G8(), oldest.B8());
                lcd.setCursor(0, 0 + 16 * 1);
                lcd.printf("noI %3u,%3u,%3u", oldest.RnoIR8(), oldest.GnoIR8(), oldest.BnoIR8());
                lcd.setCursor(0, 0 + 16 * 2);
                lcd.printf("Cal %3u,%3u,%3u", cr, cg, cb);
                lcd.setCursor(0, 0 + 16 * 3);
                if (correction == Correction::Name) {
                    lcd.printf("%s %-11s", correction_short_label[ci], colorNameToString(nearestColorName(cr, cg, cb)));
                } else {
                    lcd.printf("%s %3u,%3u,%3u", correction_short_label[ci], corrected.r, corrected.g, corrected.b);
                }
            } else {
                lcd.setCursor(16, 8 + 16 * 0);
                lcd.printf("    RGB(%3u,%3u,%3u)", oldest.R8(), oldest.G8(), oldest.B8());
                lcd.setCursor(16, 8 + 16 * 1);
                lcd.printf("RGBnoIR(%3u,%3u,%3u)", oldest.RnoIR8(), oldest.GnoIR8(), oldest.BnoIR8());
                lcd.setCursor(16, 8 + 16 * 2);
                lcd.printf("RGBCalb(%3u,%3u,%3u)", cr, cg, cb);
                lcd.setCursor(16, 8 + 16 * 3);
                if (correction == Correction::Name) {
                    lcd.printf("%7s(%-11s)", correction_label[ci], colorNameToString(nearestColorName(cr, cg, cb)));
                } else {
                    lcd.printf("%7s(%3u,%3u,%3u)", correction_label[ci], corrected.r, corrected.g, corrected.b);
                }
                lcd.setCursor(16, 8 + 16 * 4);
                lcd.printf("RAW:(%04X,%04X,%04X) %04X", oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());
            }
            lcd.endWrite();

            // Color bar (1:RGB / 2:RGB without IR / 3:Calibrated / 4:Selected correction)
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
                      oldest.R8(), oldest.G8(), oldest.B8(), oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16(),
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

    // Switch the correction of bar 4 (a hold does not fire wasClicked)
    if (M5.BtnA.wasHold()) {
        correction = static_cast<Correction>((m5::stl::to_underlying(correction) + 1) % correction_count);
        M5_LOGI("Bar 4: %s", correction_label[m5::stl::to_underlying(correction)]);
    }

    // Calibration
    if (M5.BtnB.wasHold()) {
        calibrate(false);
    }
    if (M5.BtnC.wasHold()) {
        calibrate(true);
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
