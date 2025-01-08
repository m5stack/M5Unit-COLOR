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
auto gammaTable = make_gamma_table(2.5f);

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

    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.startWrite();
    lcd.clear(TFT_BLACK);

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

    M5_LOGE("GT:%zu", gammaTable.size());
    for (uint32_t i = 0; i < 256; ++i) {
        float x = i;
        x /= 255;
        x = std::pow(x, 2.5f);
        x *= 255;
        uint8_t xx = x;

        float y = i;
        y /= 255;
        y = c_pow(y, 2.5f);
        y *= 255;
        uint8_t yy = y;

        M5_LOGW("[%3d] %u / %u /%u <%d>", i, xx, gammaTable[i], yy, (int)xx - (int)gammaTable[i]);
    }
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

    Units.update();
    if (unit.updated()) {
        const auto& oldest = unit.oldest();
        auto rgb565        = unit.RGB565();
        auto rgb_no_ir565  = oldest.RGBnoIR565();
        auto rgb_gamma565  = Data::color565(gammaTable[oldest.R8()], gammaTable[oldest.G8()], gammaTable[oldest.B8()]);
        auto rgb_no_ir_gamma565 =
            Data::color565(gammaTable[oldest.RnoIR8()], gammaTable[oldest.GnoIR8()], gammaTable[oldest.BnoIR8()]);

        lcd.setCursor(16, 8 + 16 * 0);
        lcd.printf("    RGB(%3u,%3u,%3u)", unit.R8(), unit.G8(), unit.B8());
        lcd.setCursor(16, 8 + 16 * 1);
        lcd.printf("RGBnoIR(%3u,%3u,%3u)", oldest.RnoIR8(), oldest.GnoIR8(), oldest.BnoIR8());
        lcd.setCursor(16, 8 + 16 * 2);
        lcd.printf("RGB Gam(%3u,%3u,%3u)", gammaTable[oldest.R8()], gammaTable[oldest.G8()], gammaTable[oldest.B8()]);
        lcd.setCursor(16, 8 + 16 * 3);
        lcd.printf("RGBGam2(%3u,%3u,%3u)", gammaTable[oldest.RnoIR8()], gammaTable[oldest.GnoIR8()],
                   gammaTable[oldest.BnoIR8()]);
        lcd.setCursor(16, 8 + 16 * 4);
        lcd.printf("RAW:(%04X,%04X,%04X) %04X", oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());

        int32_t y = 8 + 16 * 5;
        int32_t h = (lcd.height() - y) >> 2;
        if (h > 16) {
            h = 16;
        }
        lcd.fillRect(0, y + h * 0, lcd.width(), h - 1, rgb565);
        lcd.fillRect(0, y + h * 1, lcd.width(), h - 1, rgb_no_ir565);
        lcd.fillRect(0, y + h * 2, lcd.width(), h - 1, rgb_gamma565);
        lcd.fillRect(0, y + h * 3, lcd.width(), h - 1, rgb_no_ir_gamma565);

        auto lux    = calculateLux(oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16(), atime, gc);
        auto ct     = calculateColorTemperature(oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());
        auto cratio = calculateCRATIO(oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16());

        if (oldest.C16() >= saturation) {
            M5_LOGW("Detect saturation");
            lux = ct = cratio = 0;
        }
        M5_LOGI("RGB(%3u,%3u,%3u) RGBC:%u,%u,%u,%u DS:%u IR:%d Lux:%.2f CT:%.2f CR:%.2f", unit.R8(), unit.G8(),
                unit.B8(), oldest.R16(), oldest.G16(), oldest.B16(), oldest.C16(), (oldest.C16() >= saturation),
                oldest.IR(), lux, ct, cratio);
    }

    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
    }
}
