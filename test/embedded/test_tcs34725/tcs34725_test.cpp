/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitTCS34725
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_TCS3472x.hpp>
#include <utility/unit_color_utility.hpp>
#include <esp_random.h>
#include <cmath>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::tcs3472x;

constexpr uint32_t STORED_SIZE{4};

class TestTCS34725 : public I2CComponentTestBase<UnitTCS34725> {
protected:
    virtual UnitTCS34725* get_instance() override
    {
        auto ptr         = new m5::unit::UnitTCS34725();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
};

namespace {

float random_float(float min_val, float max_val)
{
    return min_val + (esp_random() / (float)UINT32_MAX) * (max_val - min_val);
}

constexpr Persistence pers_table[] = {
    Persistence::Every,   Persistence::Cycle1,  Persistence::Cycle2,  Persistence::Cycle3,
    Persistence::Cycle5,  Persistence::Cycle10, Persistence::Cycle15, Persistence::Cycle20,
    Persistence::Cycle25, Persistence::Cycle30, Persistence::Cycle35, Persistence::Cycle40,
    Persistence::Cycle45, Persistence::Cycle50, Persistence::Cycle55, Persistence::Cycle60,
};

constexpr Gain gain_table[] = {
    Gain::Controlx1,
    Gain::Controlx4,
    Gain::Controlx16,
    Gain::Controlx60,
};

}  // namespace

TEST_F(TestTCS34725, BeginAppliesConfig)
{
    SCOPED_TRACE(ustr);

    // Default config starts periodic with known values
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    // Read back the config values that begin() applied
    Gain gain{};
    EXPECT_TRUE(unit->readGain(gain));
    EXPECT_EQ(gain, Gain::Controlx4);  // default config gain

    float atime{};
    EXPECT_TRUE(unit->readAtime(atime));
    EXPECT_NEAR(atime, 614.4f, 2.4f);  // default config atime

    float wtime{};
    EXPECT_TRUE(unit->readWtime(wtime));
    EXPECT_NEAR(wtime, 2.4f, 2.4f);  // default config wtime

    // Restart for subsequent tests
    EXPECT_TRUE(unit->startPeriodicMeasurement());
}

TEST_F(TestTCS34725, Settings)
{
    SCOPED_TRACE(ustr);

    // Failed in periodic
    EXPECT_FALSE(unit->writeGain(Gain::Controlx60));
    EXPECT_FALSE(unit->writeAtime(0));
    EXPECT_FALSE(unit->writeWtime(0, false));

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    // Pers
    {
        for (auto&& p : pers_table) {
            EXPECT_TRUE(unit->writePersistence(p));

            Persistence pers{};
            EXPECT_TRUE(unit->readPersistence(pers));
            EXPECT_EQ(pers, p);
        }
    }

    // Gain
    {
        for (auto&& gc : gain_table) {
            EXPECT_TRUE(unit->writeGain(gc));

            Gain gain{};
            EXPECT_TRUE(unit->readGain(gain));
            EXPECT_EQ(gain, gc);
        }
    }

    // ATIME (boundary + sampled values; random covers the rest)
    {
        constexpr uint16_t atime_samples[] = {0, 1, 63, 64, 127, 128, 191, 192, 254, 255};
        for (auto t : atime_samples) {
            float ft = 2.4f * (256 - t);
            auto s   = m5::utility::formatString("AT:%X FT:%f", t, ft);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->writeAtime(t));
            uint8_t a{};
            EXPECT_TRUE(unit->readAtime(a));
            EXPECT_EQ(a, t);

            EXPECT_TRUE(unit->writeAtime(ft));
            EXPECT_TRUE(unit->readAtime(a));
            EXPECT_EQ(a, t);

            float fa{};
            EXPECT_TRUE(unit->readAtime(fa));
            EXPECT_FLOAT_EQ(fa, ft);
        }

        // failed to write
        EXPECT_TRUE(unit->writeAtime(0));
        uint8_t a{};
        EXPECT_FALSE(unit->writeAtime(1.2345f));
        EXPECT_TRUE(unit->readAtime(a));
        EXPECT_EQ(a, 0);

        EXPECT_FALSE(unit->writeAtime(666.666f));
        EXPECT_TRUE(unit->readAtime(a));
        EXPECT_EQ(a, 0);

        EXPECT_FALSE(unit->writeAtime(0.0f / 0.0f));  // Nan
        EXPECT_TRUE(unit->readAtime(a));
        EXPECT_EQ(a, 0);

        // random
        uint32_t cnt{8};
        while (cnt--) {
            float ft = random_float(AT_NORMAL_MIN, AT_NORMAL_MAX);
            auto s   = m5::utility::formatString("RandomAT:%f", ft);
            SCOPED_TRACE(s);
            EXPECT_TRUE(unit->writeAtime(ft));
            float fa{};
            EXPECT_TRUE(unit->readAtime(fa));
            EXPECT_NEAR(fa, ft, 2.4f);
        }
    }

    // WTIME
    {
        constexpr bool wlong_table[] = {false, true};

        constexpr uint16_t wtime_samples[] = {0, 1, 63, 64, 127, 128, 191, 192, 254, 255};
        for (auto&& wl : wlong_table) {
            for (auto t : wtime_samples) {
                float ft = 2.4f * (256 - t) * (wl ? 12 : 1);
                auto s   = m5::utility::formatString("WT:%X(%u) FT:%f", t, wl, ft);
                SCOPED_TRACE(s);

                EXPECT_TRUE(unit->writeWtime(t, wl));
                uint8_t w{};
                bool wlong{};
                EXPECT_TRUE(unit->readWtime(w, wlong));
                EXPECT_EQ(w, t);
                EXPECT_EQ(wlong, wl);

                //
                EXPECT_TRUE(unit->writeWtime(ft));
                EXPECT_TRUE(unit->readWtime(w, wlong));

                float fw{};
                EXPECT_TRUE(unit->readWtime(fw));
                EXPECT_FLOAT_EQ(fw, ft);
            }
        }

        // failed to write
        EXPECT_TRUE(unit->writeWtime(0, false));

        uint8_t w{};
        bool wlong{};
        EXPECT_FALSE(unit->writeWtime(1.2345f));
        EXPECT_TRUE(unit->readWtime(w, wlong));
        EXPECT_EQ(w, 0);
        EXPECT_FALSE(wlong);

        EXPECT_FALSE(unit->writeWtime(9876.543f));
        EXPECT_TRUE(unit->readWtime(w, wlong));
        EXPECT_EQ(w, 0);
        EXPECT_FALSE(wlong);

        EXPECT_FALSE(unit->writeWtime(0.0f / 0.0f));  // NaN
        EXPECT_TRUE(unit->readWtime(w, wlong));
        EXPECT_EQ(w, 0);
        EXPECT_FALSE(wlong);

        // random
        uint32_t cnt{8};
        while (cnt--) {
            float ft = random_float(WT_NORMAL_MIN, WT_LONG_MAX);
            auto s   = m5::utility::formatString("RandomWT:%f", ft);
            SCOPED_TRACE(s);
            EXPECT_TRUE(unit->writeWtime(ft));
            float fw{};
            EXPECT_TRUE(unit->readWtime(fw));
            EXPECT_NEAR(fw, ft, 2.4f * 12);
        }
    }
}

TEST_F(TestTCS34725, Interrupt)
{
    SCOPED_TRACE(ustr);

    constexpr bool it_table[] = {true, false};
    for (auto&& b : it_table) {
        EXPECT_TRUE(unit->writeInterrupt(b));
        bool bb{};
        EXPECT_TRUE(unit->readInterrupt(bb));
        EXPECT_EQ(bb, b);

        EXPECT_TRUE(unit->clearInterrupt());
    }

    uint32_t cnt{8};
    while (cnt--) {
        uint16_t low  = (esp_random() & 0xFFFF);
        uint16_t high = (esp_random() & 0xFFFF);
        auto s        = m5::utility::formatString("Low:0x%04X High:0x%04X", low, high);
        SCOPED_TRACE(s);
        EXPECT_TRUE(unit->writeInterruptThreshold(low, high));

        uint16_t ll{}, hh{};
        EXPECT_TRUE(unit->readInterruptThreshold(ll, hh));
        EXPECT_EQ(ll, low);
        EXPECT_EQ(hh, high);
    }
}

TEST_F(TestTCS34725, Data)
{
    SCOPED_TRACE(ustr);

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
        EXPECT_EQ(d.RGB888(), 0);
        EXPECT_EQ(d.RGBnoIR565(), 0);
        EXPECT_EQ(d.RGBnoIR888(), 0);

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
        EXPECT_EQ(d.RGB888(), 0x00FFFFFF);
        EXPECT_EQ(d.RGBnoIR565(), 0);
        EXPECT_EQ(d.RGBnoIR888(), 0);

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

TEST_F(TestTCS34725, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    // inPeriodic
    EXPECT_FALSE(unit->startPeriodicMeasurement());
    EXPECT_FALSE(unit->startPeriodicMeasurement(Gain::Controlx4, 100.f, 200.f));

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    // Invalid params
    EXPECT_FALSE(unit->startPeriodicMeasurement(Gain::Controlx4, 1.2f, 100.f));
    EXPECT_FALSE(unit->startPeriodicMeasurement(Gain::Controlx4, 1111.2f, 100.f));
    EXPECT_FALSE(unit->startPeriodicMeasurement(Gain::Controlx4, 12.3f, 1.2f));
    EXPECT_FALSE(unit->startPeriodicMeasurement(Gain::Controlx4, 12.3f, 7777.f));

    constexpr float tm_table[][2] = {
        // ATIME, WTIME
        {2.4f, 2.4f}, {614.4f, 2.4f}, {2.4f, 614.4f}, {123.4f, 234.5f}, {614.4f, 7372.8f},
    };

    // Gain does not affect timing; test with one gain, varying ATIME/WTIME
    constexpr size_t tm_count = sizeof(tm_table) / sizeof(tm_table[0]);
    for (size_t i = 0; i < tm_count; ++i) {
        auto& tm = tm_table[i];
        auto s   = m5::utility::formatString("A:%f W:%f", tm[0], tm[1]);
        SCOPED_TRACE(s);
        M5_LOGI("[%u/%u] ATIME:%.1f WTIME:%.1f interval:%lu", (unsigned)(i + 1), (unsigned)tm_count, tm[0], tm[1],
                (unsigned long)(std::round(tm[0] + tm[1])));

        EXPECT_TRUE(unit->startPeriodicMeasurement(Gain::Controlx4, tm[0], tm[1]));
        EXPECT_TRUE(unit->inPeriodic());

        auto r = collect_periodic_measurements(unit.get(), STORED_SIZE);
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_FALSE(r.timed_out);
        EXPECT_EQ(r.update_count, STORED_SIZE);
        EXPECT_LE(r.median(), r.expected_interval + 1);

        EXPECT_EQ(unit->available(), STORED_SIZE);
        EXPECT_FALSE(unit->empty());
        EXPECT_TRUE(unit->full());

        uint32_t cnt{STORED_SIZE / 2};
        while (cnt-- && unit->available()) {
            EXPECT_NE(unit->RGB565(), 0);
            EXPECT_EQ(unit->R8(), unit->oldest().R8());
            EXPECT_EQ(unit->G8(), unit->oldest().G8());
            EXPECT_EQ(unit->B8(), unit->oldest().B8());
            EXPECT_EQ(unit->RGB565(), unit->oldest().RGB565());

            EXPECT_FALSE(unit->empty());
            unit->discard();
        }
        EXPECT_EQ(unit->available(), STORED_SIZE / 2);
        EXPECT_FALSE(unit->empty());
        EXPECT_FALSE(unit->full());

        unit->flush();
        EXPECT_EQ(unit->available(), 0);
        EXPECT_TRUE(unit->empty());
        EXPECT_FALSE(unit->full());

        EXPECT_EQ(unit->R8(), 0);
        EXPECT_EQ(unit->G8(), 0);
        EXPECT_EQ(unit->B8(), 0);
        EXPECT_EQ(unit->RGB565(), 0);
    }
}

TEST_F(TestTCS34725, Singleshot)
{
    SCOPED_TRACE(ustr);

    Data d{};

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->measureSingleshot(d));

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    constexpr float at[4] = {2.4f, 234.56f, 456.78f, 614.4f};
    uint32_t idx{};
    for (auto&& gc : gain_table) {
        auto s = m5::utility::formatString("GC:%x", gc);
        SCOPED_TRACE(s);

        uint32_t cnt{2};
        while (cnt--) {
            EXPECT_TRUE(unit->measureSingleshot(d, gc, at[idx]));
            // M5_LOGW("GC:%u %.2f [%u]:(%x,%x,%x,%x) %x", gc, at[idx], cnt, d.R16(), d.G16(), d.B16(), d.C16(),
            //         d.RGB565());
        }
        ++idx;
    }
}

TEST_F(TestTCS34725, Status)
{
    SCOPED_TRACE(ustr);

    uint8_t status{};
    EXPECT_TRUE(unit->readStatus(status));
    // AVALID bit (bit 0) should be set if periodic measurement is running
    if (unit->inPeriodic()) {
        // Wait for at least one integration cycle
        auto timeout_at = m5::utility::millis() + 2000;
        while (m5::utility::millis() < timeout_at) {
            unit->update();
            if (unit->updated()) {
                break;
            }
            m5::utility::delay(1);
        }
        EXPECT_TRUE(unit->readStatus(status));
        EXPECT_TRUE(status & 0x01);  // AVALID
    }
}

// ============================================================
// Test with start_periodic=false
// ============================================================

class TestTCS34725NoPeriodic : public I2CComponentTestBase<UnitTCS34725> {
protected:
    virtual UnitTCS34725* get_instance() override
    {
        auto ptr         = new m5::unit::UnitTCS34725();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);

        auto cfg           = ptr->config();
        cfg.start_periodic = false;
        ptr->config(cfg);
        return ptr;
    }
};

TEST_F(TestTCS34725NoPeriodic, BeginWithoutPeriodic)
{
    SCOPED_TRACE(ustr);

    // begin() with start_periodic=false should not start periodic measurement
    EXPECT_FALSE(unit->inPeriodic());

    // Should be able to do singleshot
    Data d{};
    EXPECT_TRUE(unit->measureSingleshot(d));
}

// ============================================================
// Pure computation tests (no hardware required)
// ============================================================

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
    EXPECT_EQ(d.RGBnoIR888(), 0);
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
}
