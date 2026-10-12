/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Tests for the TCS3472x data types and the color utilities (no hardware).
  Placed directly under test/, so they run on the host (test_native) and on the boards (test_UnitColor_*)
*/
#include <gtest/gtest.h>
#include <unit/unit_TCS3472x_data.hpp>
#include <utility/unit_color_utility.hpp>
#include <utility/color_correction.hpp>
#include <algorithm>
#include <tuple>

using namespace m5::unit::tcs3472x;

namespace {

Data make_data(uint16_t c, uint16_t r, uint16_t g, uint16_t b)
{
    Data d{};
    d.raw[0] = c & 0xFF;
    d.raw[1] = c >> 8;
    d.raw[2] = r & 0xFF;
    d.raw[3] = r >> 8;
    d.raw[4] = g & 0xFF;
    d.raw[5] = g >> 8;
    d.raw[6] = b & 0xFF;
    d.raw[7] = b >> 8;
    return d;
}

}  // namespace

TEST(DataStruct, RawAndConversions)
{
    {
        Data d{};

        EXPECT_EQ(d.R16(), 0);
        EXPECT_EQ(d.G16(), 0);
        EXPECT_EQ(d.B16(), 0);
        EXPECT_EQ(d.C16(), 0);
        EXPECT_EQ(d.IR(), 0);
        EXPECT_EQ(d.RnoIR16(), 0);
        EXPECT_EQ(d.GnoIR16(), 0);
        EXPECT_EQ(d.BnoIR16(), 0);
        EXPECT_EQ(d.CnoIR16(), 0);

        EXPECT_EQ(d.R8(), 0);
        EXPECT_EQ(d.G8(), 0);
        EXPECT_EQ(d.B8(), 0);
        EXPECT_EQ(d.RnoIR8(), 0);
        EXPECT_EQ(d.GnoIR8(), 0);
        EXPECT_EQ(d.BnoIR8(), 0);

        EXPECT_EQ(d.RGB565(), 0);
        EXPECT_EQ(d.RGB888(), 0U);
        EXPECT_EQ(d.RGBnoIR565(), 0);
        EXPECT_EQ(d.RGBnoIR888(), 0U);

        // M5_LOGW("RGBC16(%x,%x,%x,%x) RGB8(%x,%x,%x)", d.R16(), d.G16(), d.B16(), d.C16(), d.R8(), d.G8(), d.B8());
        // M5_LOGW("noIR IR:%d RGBC16(%x,%x,%x,%x) RGB8(%x,%x,%x)", d.IR(), d.RnoIR16(), d.GnoIR16(), d.BnoIR16(),
        //         d.CnoIR16(), d.RnoIR8(), d.GnoIR8(), d.BnoIR8());
    }

    {
        Data d{};
        std::fill(d.raw.begin(), d.raw.end(), 0xFF);

        EXPECT_EQ(d.R16(), 0xFFFF);
        EXPECT_EQ(d.G16(), 0xFFFF);
        EXPECT_EQ(d.B16(), 0xFFFF);
        EXPECT_EQ(d.C16(), 0xFFFF);
        EXPECT_EQ(d.IR(), ((0xFFFF * 3) - 0xFFFF) / 2);
        EXPECT_EQ(d.RnoIR16(), 0);
        EXPECT_EQ(d.GnoIR16(), 0);
        EXPECT_EQ(d.BnoIR16(), 0);
        EXPECT_EQ(d.CnoIR16(), 0);

        EXPECT_EQ(d.R8(), 255);
        EXPECT_EQ(d.G8(), 255);
        EXPECT_EQ(d.B8(), 255);
        EXPECT_EQ(d.RnoIR8(), 0);
        EXPECT_EQ(d.GnoIR8(), 0);
        EXPECT_EQ(d.BnoIR8(), 0);

        EXPECT_EQ(d.RGB565(), 0xFFFF);
        EXPECT_EQ(d.RGB888(), 0x00FFFFFFU);
        EXPECT_EQ(d.RGBnoIR565(), 0);
        EXPECT_EQ(d.RGBnoIR888(), 0U);

        // M5_LOGW("RGBC16(%x,%x,%x,%x) RGB8(%x,%x,%x)", d.R16(), d.G16(), d.B16(), d.C16(), d.R8(), d.G8(), d.B8());
        // M5_LOGW("noIR IR:%d RGBC16(%x,%x,%x,%x) RGB8(%x,%x,%x)", d.IR(), d.RnoIR16(), d.GnoIR16(), d.BnoIR16(),
        //         d.CnoIR16(), d.RnoIR8(), d.GnoIR8(), d.BnoIR8());
    }

    {
        Data d{};
        d.raw[2] = 0x34;
        d.raw[3] = 0x12;  // R 4660
        d.raw[4] = 0x78;
        d.raw[5] = 0x56;  // G 22136
        d.raw[6] = 0xbc;
        d.raw[7] = 0x9a;  // B 39612
        d.raw[0] = 0xaf;
        d.raw[1] = 0xbe;  // C 48815

        EXPECT_EQ(d.R16(), 0x1234);
        EXPECT_EQ(d.G16(), 0x5678);
        EXPECT_EQ(d.B16(), 0x9abc);
        EXPECT_EQ(d.C16(), 0xbeaf);
        EXPECT_EQ(d.IR(), ((0x1234 + 0x5678 + 0x9abc) - 0xbeaf) / 2);  // 8796
        EXPECT_EQ(d.RnoIR16(), 0);
        EXPECT_EQ(d.GnoIR16(), 22136 - 8796);
        EXPECT_EQ(d.BnoIR16(), 39612 - 8796);
        EXPECT_EQ(d.CnoIR16(), 48815 - 8796);

        EXPECT_EQ(d.R8(), (uint8_t)((0x1234 / (float)0xbeaf) * 255));
        EXPECT_EQ(d.G8(), (uint8_t)((0x5678 / (float)0xbeaf) * 255));
        EXPECT_EQ(d.B8(), (uint8_t)((0x9abc / (float)0xbeaf) * 255));
        EXPECT_EQ(d.RnoIR8(), 0);
        EXPECT_EQ(d.GnoIR8(), (uint8_t)((d.G16() - d.IR()) / (float)(0xbeaf - d.IR()) * 255));
        EXPECT_EQ(d.BnoIR8(), (uint8_t)((d.B16() - d.IR()) / (float)(0xbeaf - d.IR()) * 255));

        // M5_LOGW("RGBC16(%x,%x,%x,%x) RGB8(%x,%x,%x)", d.R16(), d.G16(), d.B16(), d.C16(), d.R8(), d.G8(), d.B8());
        // M5_LOGW("noIR IR:%d RGBC16(%x,%x,%x,%x) RGB8(%x,%x,%x)", d.IR(), d.RnoIR16(), d.GnoIR16(), d.BnoIR16(),
        //         d.CnoIR16(), d.RnoIR8(), d.GnoIR8(), d.BnoIR8());
    }
}

// Data struct edge cases

TEST(DataStruct, IRCache)
{
    auto d = make_data(4000, 1000, 2000, 1500);
    // IR = (1000+2000+1500-4000)/2 = 250
    EXPECT_EQ(d.IR(), 250);
    EXPECT_EQ(d.IR(true), 250);   // cached
    EXPECT_EQ(d.IR(false), 250);  // forced recompute
}

TEST(DataStruct, RnoIR8_WhenIRGreaterThanC)
{
    // R+G+B > 3*C  =>  IR > C
    auto d = make_data(15000, 20000, 20000, 20000);
    // IR = (60000 - 15000) / 2 = 22500
    EXPECT_EQ(d.IR(), 22500);
    EXPECT_EQ(d.RnoIR16(), 0);
    EXPECT_EQ(d.GnoIR16(), 0);
    EXPECT_EQ(d.BnoIR16(), 0);
    EXPECT_EQ(d.CnoIR16(), 0);
    // All noIR8 must be 0 (regression test for bug fix)
    EXPECT_EQ(d.RnoIR8(), 0);
    EXPECT_EQ(d.GnoIR8(), 0);
    EXPECT_EQ(d.BnoIR8(), 0);
    EXPECT_EQ(d.RGBnoIR565(), 0);
    EXPECT_EQ(d.RGBnoIR888(), 0U);
}

TEST(DataStruct, RnoIR8_WhenIRGreaterThanChannel)
{
    // IR > R but IR < C
    auto d = make_data(48815, 0x1234, 0x5678, 0x9abc);
    // IR = (4660 + 22136 + 39612 - 48815) / 2 = 8796
    EXPECT_EQ(d.IR(), 8796);
    EXPECT_EQ(d.RnoIR16(), 0);             // 4660 - 8796 < 0 => clamped
    EXPECT_EQ(d.GnoIR16(), 22136 - 8796);  // 13340
    EXPECT_EQ(d.BnoIR16(), 39612 - 8796);  // 30816
    EXPECT_EQ(d.CnoIR16(), 48815 - 8796);  // 40019
    EXPECT_EQ(d.RnoIR8(), 0);              // R is below IR
    EXPECT_GT(d.GnoIR8(), 0);              // G has signal
    EXPECT_GT(d.BnoIR8(), 0);              // B has signal
}

TEST(DataStruct, ColorConversions)
{
    // color332
    EXPECT_EQ(Data::color332(0xFF, 0xFF, 0xFF), 0xFF);
    EXPECT_EQ(Data::color332(0x00, 0x00, 0x00), 0x00);

    // color565
    EXPECT_EQ(Data::color565(0xFF, 0xFF, 0xFF), 0xFFFF);
    EXPECT_EQ(Data::color565(0x00, 0x00, 0x00), 0x0000);
    EXPECT_EQ(Data::color565(0xFF, 0x00, 0x00), 0xF800);  // Red only
    EXPECT_EQ(Data::color565(0x00, 0xFF, 0x00), 0x07E0);  // Green only
    EXPECT_EQ(Data::color565(0x00, 0x00, 0xFF), 0x001F);  // Blue only

    // color888
    EXPECT_EQ(Data::color888(0xFF, 0xFF, 0xFF), 0x00FFFFFFU);
    EXPECT_EQ(Data::color888(0x12, 0x34, 0x56), 0x00123456U);

    // swap565 — byte-swapped color565
    uint16_t c565 = Data::color565(0xFF, 0x00, 0x00);
    uint16_t s565 = Data::swap565(0xFF, 0x00, 0x00);
    EXPECT_EQ(s565, (uint16_t)((c565 >> 8) | (c565 << 8)));

    // swap888 — BGR order
    EXPECT_EQ(Data::swap888(0x12, 0x34, 0x56), 0x00563412U);
}

TEST(DataStruct, ZeroClear)
{
    // When C=0, R8/G8/B8 should return 0 (avoid division by zero)
    auto d = make_data(0, 100, 200, 300);
    EXPECT_EQ(d.R8(), 0);
    EXPECT_EQ(d.G8(), 0);
    EXPECT_EQ(d.B8(), 0);
}

// Utility functions

TEST(Utility, CalculateSaturation)
{
    // Digital saturation: (256 - ATIME) > 63
    EXPECT_EQ(calculateSaturation((uint8_t)0), 0xFFFF);    // steps=256
    EXPECT_EQ(calculateSaturation((uint8_t)192), 0xFFFF);  // steps=64

    // Analog saturation: (256 - ATIME) <= 63
    // ATIME=193 => steps=63 => 1024*63=64512, ripple: 64512 - 16128 = 48384
    EXPECT_EQ(calculateSaturation((uint8_t)193), 48384);
    // ATIME=255 => steps=1 => 1024, ripple: 1024 - 256 = 768
    EXPECT_EQ(calculateSaturation((uint8_t)255), 768);

    // Float overload
    EXPECT_EQ(calculateSaturation(614.4f), 0xFFFF);
    EXPECT_EQ(calculateSaturation(2.4f), 768);
}

TEST(Utility, CalculateCPL)
{
    // CPL = atime_ms * gain / DGF
    float cpl = calculateCPL(614.4f, Gain::Controlx1);
    EXPECT_NEAR(cpl, 614.4f / 310.0f, 0.001f);

    cpl = calculateCPL(2.4f, Gain::Controlx60);
    EXPECT_NEAR(cpl, 2.4f * 60.0f / 310.0f, 0.001f);

    // DGF=0 => NaN
    cpl = calculateCPL(100.0f, Gain::Controlx1, 0.0f);
    EXPECT_TRUE(std::isnan(cpl));
}

TEST(Utility, CalculateMaxLux)
{
    float cpl     = calculateCPL(614.4f, Gain::Controlx4);
    float max_lux = calculateMaxLux(614.4f, Gain::Controlx4);
    EXPECT_NEAR(max_lux, 65535.0f / (3.0f * cpl), 0.01f);
}

TEST(Utility, CalculateLux)
{
    // Known values: R=1000, G=2000, B=1500, C=4000, atime=614.4, gain=4x
    float expected_cpl = 614.4f * 4.0f / 310.0f;
    float expected_g2  = 0.136f * 750 + 1.0f * 1750 + (-0.444f) * 1250;
    float expected_lux = expected_g2 / expected_cpl;
    float lux          = calculateLux(1000, 2000, 1500, 4000, 614.4f, Gain::Controlx4);
    EXPECT_NEAR(lux, expected_lux, 0.1f);
    EXPECT_GT(lux, 0.0f);

    // All zeros => lux 0
    EXPECT_EQ(calculateLux(0, 0, 0, 0, 614.4f, Gain::Controlx1), 0.0f);

    // Negative lux clamps to 0
    float lux_neg = calculateLux(100, 100, 60000, 200, 614.4f, Gain::Controlx1);
    EXPECT_GE(lux_neg, 0.0f);
}

TEST(Utility, CalculateColorTemperature)
{
    // CT = CT_Coef * B'/R' + CT_Offset
    // R=1000, G=2000, B=1500, C=4000 => IR=250, R'=750, B'=1250
    float expected_ct = 3810.0f * 1250.0f / 750.0f + 1391.0f;
    float ct          = calculateColorTemperature(1000, 2000, 1500, 4000);
    EXPECT_NEAR(ct, expected_ct, 0.1f);

    // R' <= 0 => NaN
    // R=100, G=1000, B=1000, C=500 => IR=800, R'=-700
    float ct_nan = calculateColorTemperature(100, 1000, 1000, 500);
    EXPECT_TRUE(std::isnan(ct_nan));
}

TEST(Utility, CalculateCRATIO)
{
    // CRATIO = IR / C, clamped [0, 1]
    float cr = calculateCRATIO(1000, 2000, 1500, 4000);
    EXPECT_NEAR(cr, 250.0f / 4000.0f, 0.001f);

    // C=0 => NaN
    EXPECT_TRUE(std::isnan(calculateCRATIO(100, 200, 300, 0)));

    // Clamped to 1.0 when IR >> C
    EXPECT_FLOAT_EQ(calculateCRATIO(60000, 60000, 60000, 1), 1.0f);

    // Clamped to 0.0 when IR < 0
    EXPECT_FLOAT_EQ(calculateCRATIO(0, 0, 0, 65535), 0.0f);
}

TEST(Utility, AtimeConversion)
{
    EXPECT_FLOAT_EQ(atime_to_ms(0), 614.4f);
    EXPECT_FLOAT_EQ(atime_to_ms(255), 2.4f);
    EXPECT_EQ(ms_to_atime(614.4f), 0);
    EXPECT_EQ(ms_to_atime(2.4f), 255);

    // Round-trip
    for (uint16_t a = 0; a < 256; ++a) {
        float ms = atime_to_ms(a);
        EXPECT_EQ(ms_to_atime(ms), a) << "atime=" << a;
    }
}

TEST(Utility, WtimeConversion)
{
    EXPECT_FLOAT_EQ(wtime_to_ms(0, false), 614.4f);
    EXPECT_FLOAT_EQ(wtime_to_ms(0, true), 614.4f * 12);
    EXPECT_FLOAT_EQ(wtime_to_ms(255, false), 2.4f);
    EXPECT_FLOAT_EQ(wtime_to_ms(255, true), 2.4f * 12);

    // ms_to_wtime
    {
        auto result = ms_to_wtime(2.4f);
        EXPECT_EQ(std::get<0>(result), 255);
        EXPECT_FALSE(std::get<1>(result));
    }
    {
        auto result = ms_to_wtime(7372.8f);
        EXPECT_EQ(std::get<0>(result), 0);
        EXPECT_TRUE(std::get<1>(result));
    }
    // Above normal range must use WLONG
    {
        auto result = ms_to_wtime(1000.0f);
        EXPECT_TRUE(std::get<1>(result));
    }
}

TEST(Utility, CalibrationLinear)
{
    EXPECT_EQ(Calibration::linear(500, 200, 800), 128);  // midpoint
    EXPECT_EQ(Calibration::linear(200, 200, 800), 0);    // at low
    EXPECT_EQ(Calibration::linear(800, 200, 800), 255);  // at high
    EXPECT_EQ(Calibration::linear(100, 200, 800), 0);    // below low => clamped
    EXPECT_EQ(Calibration::linear(900, 200, 800), 255);  // above high => clamped
}

TEST(Utility, CalibrationRGB)
{
    Calibration calib{100, 1100, 200, 1200, 300, 1300};
    // IR=0 when R+G+B = C => noIR16 equals raw channel
    auto d = make_data(2100, 600, 700, 800);
    EXPECT_EQ(d.IR(), 0);
    EXPECT_EQ(d.RnoIR16(), 600);
    EXPECT_EQ(d.GnoIR16(), 700);
    EXPECT_EQ(d.BnoIR16(), 800);

    // linear(600, 100, 1100) = round((500/1000)*255) = round(127.5) = 128
    EXPECT_EQ(calib.R8(d), 128);
    // linear(700, 200, 1200) = round((500/1000)*255) = 128
    EXPECT_EQ(calib.G8(d), 128);
    // linear(800, 300, 1300) = round((500/1000)*255) = 128
    EXPECT_EQ(calib.B8(d), 128);
}

TEST(Utility, GammaTable)
{
    // gamma=1.0 should be identity
    auto table_1 = make_gamma_table(1.0f);
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(table_1[i], i) << "index=" << i;
    }

    // gamma=2.2 basic checks
    auto table_22 = make_gamma_table(2.2f);
    EXPECT_EQ(table_22[0], 0);
    EXPECT_EQ(table_22[255], 255);
    // Midpoint should be below 128 (gamma > 1 darkens midtones)
    EXPECT_LT(table_22[128], 128);

    // gamma=2.5 — more aggressive curve
    auto table_25 = make_gamma_table(2.5f);
    EXPECT_EQ(table_25[0], 0);
    EXPECT_EQ(table_25[255], 255);
    EXPECT_LT(table_25[128], table_22[128]);

    // gamma=1/2.2 (sRGB-like encoding for a display) brightens midtones
    auto table_enc = make_gamma_table(1.0f / 2.2f);
    EXPECT_EQ(table_enc[0], 0);
    EXPECT_EQ(table_enc[255], 255);
    EXPECT_GT(table_enc[128], 128);
    // Rounded, not truncated: (3/255)^(1/2.2) * 255 = 33.85
    EXPECT_EQ(table_enc[3], 34);
}

// Color corrections

TEST(ColorCorrection, PureHue)
{
    // Dull blue becomes vivid blue: min 60 removed, range 60 scaled to 255
    auto c = pureHue(60, 70, 120);
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 43);  // 10 * 255 / 60 = 42.5 -> 43
    EXPECT_EQ(c.b, 255);

    // Gray and white have no hue and are returned unchanged
    c = pureHue(128, 128, 128);
    EXPECT_EQ(c.r, 128);
    EXPECT_EQ(c.g, 128);
    EXPECT_EQ(c.b, 128);

    // Already pure colors are kept
    c = pureHue(255, 0, 0);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(ColorCorrection, NormalizeBrightness)
{
    // Largest channel becomes 255, ratios kept
    auto c = normalizeBrightness(50, 100, 25);
    EXPECT_EQ(c.r, 128);  // 50 * 255 / 100 = 127.5 -> 128
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 64);  // 25 * 255 / 100 = 63.75 -> 64

    // Black is returned unchanged
    c = normalizeBrightness(0, 0, 0);
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(ColorCorrection, Grayscale)
{
    auto c = grayscale(255, 255, 255);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);

    c = grayscale(0, 0, 0);
    EXPECT_EQ(c.r, 0);

    // BT.709: green is the brightest, blue the darkest
    EXPECT_EQ(grayscale(255, 0, 0).r, 54);
    EXPECT_EQ(grayscale(0, 255, 0).r, 182);
    EXPECT_EQ(grayscale(0, 0, 255).r, 19);
}

TEST(ColorCorrection, NearestColorName)
{
    EXPECT_EQ(nearestColorName(0, 0, 0), ColorName::Black);
    EXPECT_EQ(nearestColorName(30, 10, 10), ColorName::Black);
    EXPECT_EQ(nearestColorName(250, 245, 240), ColorName::White);
    EXPECT_EQ(nearestColorName(120, 120, 125), ColorName::Gray);
    EXPECT_EQ(nearestColorName(255, 0, 0), ColorName::Red);
    EXPECT_EQ(nearestColorName(255, 0, 20), ColorName::Red);  // Hue wraps around
    EXPECT_EQ(nearestColorName(255, 140, 0), ColorName::Orange);
    EXPECT_EQ(nearestColorName(255, 255, 0), ColorName::Yellow);
    EXPECT_EQ(nearestColorName(0, 255, 0), ColorName::Green);
    EXPECT_EQ(nearestColorName(0, 255, 255), ColorName::Cyan);
    EXPECT_EQ(nearestColorName(0, 0, 255), ColorName::Blue);
    EXPECT_EQ(nearestColorName(140, 0, 255), ColorName::Purple);
    EXPECT_EQ(nearestColorName(255, 0, 255), ColorName::Magenta);
    EXPECT_EQ(nearestColorName(120, 60, 20), ColorName::Brown);  // Dark orange
}

TEST(ColorCorrection, ColorNameTable)
{
    EXPECT_STREQ(colorNameToString(ColorName::Black), "Black");
    EXPECT_STREQ(colorNameToString(ColorName::Brown), "Brown");
    EXPECT_EQ(colorNameToRGB(ColorName::Red).to888(), 0xFF0000U);
    EXPECT_EQ(colorNameToRGB(ColorName::White).to565(), 0xFFFF);

    // The representative color of each name is classified as that name
    for (uint8_t i = 0; i <= static_cast<uint8_t>(ColorName::Brown); ++i) {
        const auto name = static_cast<ColorName>(i);
        const auto rgb  = colorNameToRGB(name);
        EXPECT_EQ(nearestColorName(rgb.r, rgb.g, rgb.b), name) << colorNameToString(name);
    }
}
