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

    uint8_t atime{};
    EXPECT_TRUE(unit->readAtime(atime));
    EXPECT_EQ(atime, ms_to_atime(614.4f));  // default config atime (raw 0x00)

    uint8_t wtime{};
    bool wlong{};
    EXPECT_TRUE(unit->readWtime(wtime, wlong));
    EXPECT_EQ(wtime, 0xFF);  // default config wtime 2.4 ms
    EXPECT_FALSE(wlong);

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
                uint8_t ew{};
                bool ewl{};
                std::tie(ew, ewl) = ms_to_wtime(ft);
                EXPECT_EQ(w, ew);
                EXPECT_EQ(wlong, ewl);

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

    // Out of range ATIME is rejected
    EXPECT_FALSE(unit->measureSingleshot(d, Gain::Controlx1, 1.2f));
    EXPECT_FALSE(unit->measureSingleshot(d, Gain::Controlx1, 700.f));
    EXPECT_FALSE(unit->measureSingleshot(d, Gain::Controlx1, 0.0f / 0.0f));  // NaN

    constexpr float at[4] = {2.4f, 234.56f, 456.78f, 614.4f};
    uint32_t idx{};
    for (auto&& gc : gain_table) {
        auto s = m5::utility::formatString("GC:%x", m5::stl::to_underlying(gc));
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

    // The fixture starts periodic measurement, so AVALID (bit 0) is set after one integration cycle
    EXPECT_TRUE(unit->inPeriodic());
    const auto start_at = m5::utility::millis();
    while (!m5::utility::hasElapsed(start_at, 2000)) {
        unit->update();
        if (unit->updated()) {
            break;
        }
        m5::utility::delay(1);
    }
    EXPECT_TRUE(unit->updated());
    EXPECT_TRUE(unit->readStatus(status));
    EXPECT_TRUE(status & 0x01);  // AVALID
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
// Test with a non-default config
// ============================================================

class TestTCS34725Config : public I2CComponentTestBase<UnitTCS34725> {
protected:
    virtual UnitTCS34725* get_instance() override
    {
        auto ptr         = new m5::unit::UnitTCS34725();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);

        auto cfg  = ptr->config();
        cfg.gain  = Gain::Controlx16;
        cfg.atime = 100.8f;
        cfg.wtime = 1000.f;  // Needs WLONG
        ptr->config(cfg);
        return ptr;
    }
};

TEST_F(TestTCS34725Config, BeginAppliesConfig)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    Gain gain{};
    EXPECT_TRUE(unit->readGain(gain));
    EXPECT_EQ(gain, Gain::Controlx16);

    uint8_t atime{};
    EXPECT_TRUE(unit->readAtime(atime));
    EXPECT_EQ(atime, ms_to_atime(100.8f));

    uint8_t wtime{}, ew{};
    bool wlong{}, ewl{};
    EXPECT_TRUE(unit->readWtime(wtime, wlong));
    std::tie(ew, ewl) = ms_to_wtime(1000.f);
    EXPECT_EQ(wtime, ew);
    EXPECT_EQ(wlong, ewl);
    EXPECT_TRUE(wlong);
}
