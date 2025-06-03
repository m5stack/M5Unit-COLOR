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
uint16_t correction_for_blue(const Data& d)
{
    auto r = d.RnoIR8();
    auto g = d.GnoIR8();
    auto b = d.BnoIR8();

    float remove, normalize;
    if ((b < g) && (b < r)) {
        remove    = b;
        normalize = max(r - b, g - b);
    } else if ((g < b) && (g < r)) {
        remove    = g;
        normalize = max(r - g, b - g);
    } else {
        remove    = r;
        normalize = max(b - r, g - r);
    }
    // get rid of minority report
    float rednorm   = r - remove;
    float greennorm = g - remove;
    float bluenorm  = b - remove;
    // now normalize for the highest number
    rednorm /= normalize;
    greennorm /= normalize;
    bluenorm /= normalize;

    return Data::color565(rednorm, greennorm, rednorm);
}
#endif

}  // namespace

void setup()
{
    M5.begin();
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    if (!unit.readGain(gc) || !unit.readAtime(atime)) {
        M5_LOGE("Failed to read");
        lcd.clear(TFT_BLUE);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    saturation = calculateSaturation(atime);
    max_lux    = calculateMaxLux(atime, gc);
    M5_LOGI("ATIME:%f GAIN:%u SAT:%u MLUX:%f ", atime, gc, saturation, max_lux);

#if !defined(M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CAN_CONSTEXPR_MAKE_GAMMA_TABLE)
    gammaTable = make_gamma_table(gamma_value);
#endif

    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.startWrite();
    lcd.clear(TFT_BLACK);
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

    Units.update();
    if (unit.updated()) {
        const auto& oldest = unit.oldest();
        uint16_t colors[4]{};

        colors[0] = unit.RGB565();
        colors[1] = oldest.RGBnoIR565();
        colors[2] = Data::color565(calib.R8(oldest), calib.G8(oldest), calib.B8(oldest));
        colors[3] =
            Data::color565(gammaTable[calib.R8(oldest)], gammaTable[calib.G8(oldest)], gammaTable[calib.B8(oldest)]);

        // Information
        // 1st : RGB
        // 2nd : RGB without IR
        // 3rd : Calibrated RGB
        // 4th : Gamma correction of calibrated values
        // 5th : RAW RGBC
        lcd.setCursor(16, 8 + 16 * 0);
        lcd.printf("    RGB(%3u,%3u,%3u)", unit.R8(), unit.G8(), unit.B8());
        lcd.setCursor(16, 8 + 16 * 1);
        lcd.printf("RGBnoIR(%3u,%3u,%3u)", oldest.RnoIR8(), oldest.GnoIR8(), oldest.BnoIR8());
        lcd.setCursor(16, 8 + 16 * 2);
        lcd.printf("RGBCalb(%3u,%3u,%3u)", calib.R8(oldest), calib.G8(oldest), calib.G8(oldest));
        lcd.setCursor(16, 8 + 16 * 3);
        lcd.printf("CalbGam(%3u,%3u,%3u)", gammaTable[calib.R8(oldest)], gammaTable[calib.G8(oldest)],
                   gammaTable[calib.B8(oldest)]);
        lcd.setCursor(16, 8 + 16 * 4);
        lcd.printf("RAW:(%04X,%04X,%04X) %04X", oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());

        // Color bar
        // 1st : RGB
        // 2nd : RGB without IR
        // 3rd : Calibrated RGB
        // 4th : Gamma correction of calibrated values
        int32_t y = 8 + 16 * 5;
        int32_t h = (lcd.height() - y) >> 2;
        if (h > 16) {
            h = 16;
        }
        for (uint32_t i = 0; i < 4; ++i) {
            lcd.fillRect(0, y + h * i, lcd.width(), h - 1, colors[i]);
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
                      (oldest.C16() >= saturation), oldest.IR(), lux, ct, cratio);
    }

    // Single shot
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        Data d{};
        if (unit.stopPeriodicMeasurement() && unit.measureSingleshot(d)) {
            M5.Log.printf("\tSingle: RGB(%3u,%3u,%3u) RGBC:%04X,%04X,%04X,%04X\n", d.R8(), d.G8(), d.B8(), d.R16(),
                          d.G16(), d.B16(), d.C16());
            unit.startPeriodicMeasurement();
        }
    }
}
