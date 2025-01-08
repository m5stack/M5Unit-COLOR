/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_color_utility.cpp
  @brief Utilities for UnitColor
*/
#include "unit_color_utility.hpp"
#include <M5Utility.hpp>
#include <limits>

namespace {
constexpr float gain_table[4] = {1.0f, 4.0f, 16.0f, 60.0f};
}

namespace m5 {
namespace unit {
namespace tcs3472x {

float calculateLux(const int32_t rawR, const int32_t rawG, const int32_t rawB, const int32_t rawC, const float atime_ms,
                   const Gain gc, const float dgf, const float coefR, const float coefG, const float coefB)
{
    float ir  = (rawR + rawG + rawB - rawC) * 0.5f;
    float rp  = rawR - ir;
    float gp  = rawG - ir;
    float bp  = rawB - ir;
    float g2  = coefR * rp + coefG * gp + coefB * bp;
    float cpl = calculateCPL(atime_ms, gc, dgf);
    float lx  = (cpl != 0.0f) ? g2 / cpl : 0.0f;
    return lx >= 0.0f ? lx : 0.0f;
}

float calculateColorTemperature(const int32_t rawR, const int32_t rawG, const int32_t rawB, const int32_t rawC)
{
    float ir = (rawR + rawG + rawB - rawC) * 0.5f;
    float rp = rawR - ir;
    // float gp  = rawG - ir;
    float bp = rawB - ir;
    return CT_Coef * bp / rp + CT_Offset;
}

uint16_t calculateSaturation(const uint8_t raw)
{
    uint16_t sat = ((256 - raw) > 63) ? 0xFFFF /* Digiatl */ : 1024 * (256 - raw) /* Analog */;
    if (256 - raw <= 63) {
        sat -= (sat >> 2);  // Ripple saturation
    }
    return sat;
}

float calculateCRATIO(const int32_t rawR, const int32_t rawG, const int32_t rawB, const int32_t rawC)
{
    if (rawC == 0) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    float ir = (rawR + rawG + rawB - rawC) * 0.5f;
    return std::fmax(std::fmin(ir / rawC, 1.0f), 0.0f);
}

float calculateCPL(const float atime_ms, const Gain gc, const float dgf)
{
    return (dgf > 0.0f) ? atime_ms * gain_table[m5::stl::to_underlying(gc)] / dgf
                        : std::numeric_limits<float>::quiet_NaN();
}

}  // namespace tcs3472x
}  // namespace unit
}  // namespace m5
