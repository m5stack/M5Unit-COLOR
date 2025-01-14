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
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::tcs3472x;
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

constexpr uint32_t STORED_SIZE{4};

class TestTCS34725 : public ComponentTestBase<UnitTCS34725, bool> {
protected:
    virtual UnitTCS34725* get_instance() override
    {
        auto ptr         = new m5::unit::UnitTCS34725();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestTCS34725, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestTCS34725, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestTCS34725, ::testing::Values(false));

namespace {

template <class U>
elapsed_time_t test_periodic(U* unit, const uint32_t times, const uint32_t measure_duration = 0)
{
    auto tm         = unit->interval();
    auto timeout_at = m5::utility::millis() + 10 * 1000;
    do {
        unit->update();
        if (unit->updated()) {
            break;
        }
        std::this_thread::yield();
    } while (!unit->updated() && m5::utility::millis() <= timeout_at);
    // timeout
    if (!unit->updated()) {
        return 0;
    }

    //
    uint32_t measured{};
    auto start_at = unit->updatedMillis();
    timeout_at    = start_at + (times * (tm + measure_duration) * 2);

    do {
        unit->update();
        measured += unit->updated() ? 1 : 0;
        if (measured >= times) {
            break;
        }
        m5::utility::delay(1);

    } while (measured < times && m5::utility::millis() <= timeout_at);
    return (measured == times) ? m5::utility::millis() - start_at : 0;
}

auto rng = std::default_random_engine{};
std::uniform_real_distribution<> dist_atime{AT_NORMAL_MIN, AT_NORMAL_MAX};
std::uniform_real_distribution<> dist_wtime{WT_NORMAL_MIN, WT_LONG_MAX};
std::uniform_int_distribution<> dist_thres{0, 0xFFFF};

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

TEST_P(TestTCS34725, Settings)
{
    SCOPED_TRACE(ustr);

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

    // ATIME
    {
        for (uint16_t t = 0; t < 256; ++t) {
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
        uint32_t cnt{32};
        while (cnt--) {
            float ft = dist_atime(rng);
            EXPECT_TRUE(unit->writeAtime(ft));
            float fa{};
            EXPECT_TRUE(unit->readAtime(fa));
            EXPECT_NEAR(fa, ft, 2.4f);
        }
    }

    // WTIME
    {
        constexpr bool wlong_table[] = {false, true};

        for (auto&& wl : wlong_table) {
            for (uint16_t t = 0; t < 256; ++t) {
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
        uint32_t cnt{32};
        while (cnt--) {
            float ft = dist_wtime(rng);
            EXPECT_TRUE(unit->writeWtime(ft));
            float fw{};
            EXPECT_TRUE(unit->readWtime(fw));
            EXPECT_NEAR(fw, ft, 2.4f * 12);
        }
    }
}

TEST_P(TestTCS34725, Interrupt)
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

    uint32_t cnt{32};
    while (cnt--) {
        uint16_t low  = dist_thres(rng);
        uint16_t high = dist_thres(rng);
        EXPECT_TRUE(unit->writeInterruptThreshold(low, high));

        uint16_t ll{}, hh{};
        EXPECT_TRUE(unit->readInterruptThreshold(ll, hh));
        EXPECT_EQ(ll, low);
        EXPECT_EQ(hh, high);
    }
}

TEST_P(TestTCS34725, Data)
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

TEST_P(TestTCS34725, Periodic)
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

    for (auto&& gc : gain_table) {
        for (auto&& tm : tm_table) {
            auto s = m5::utility::formatString("GC:%x A:%f W:%f", gc, tm[0], tm[1]);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->startPeriodicMeasurement(gc, tm[0], tm[1]));
            EXPECT_TRUE(unit->inPeriodic());

            auto elapsed = test_periodic(unit.get(), STORED_SIZE);
            EXPECT_TRUE(unit->stopPeriodicMeasurement());
            EXPECT_FALSE(unit->inPeriodic());

            // M5_LOGW("elapsed:%ld", elapsed);
            EXPECT_GE(elapsed, (int)(std::round(tm[0] + tm[1]) * STORED_SIZE));

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
}

TEST_P(TestTCS34725, Singleshot)
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

        uint32_t cnt{8};
        while (cnt--) {
            EXPECT_TRUE(unit->measureSingleshot(d, gc, at[idx]));
            // M5_LOGW("GC:%u %.2f [%u]:(%x,%x,%x,%x) %x", gc, at[idx], cnt, d.R16(), d.G16(), d.B16(), d.C16(),
            //         d.RGB565());
        }
        ++idx;
    }
}
