/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_color_utility.hpp
  @brief Utilities for UnitColor
*/
#ifndef M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_HPP
#define M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_HPP

#include "../unit/unit_TCS3472x.hpp"
#include <cmath>
#include <tuple>
#include <cassert>

namespace m5 {
namespace unit {
namespace tcs3472x {

///@name Coefficients for Lux and CT Equations
///@{
constexpr float GA{1.0f};
// constexpr float GA{1.08f}; // Placing the parts behind clear glass requires using GA = 1.08
constexpr float DF{310.f};
constexpr float DGF{GA * DF};
constexpr float R_Coef{0.136f};
constexpr float G_Coef{1.0f};
constexpr float B_Coef{-0.444f};
constexpr float CT_Coef{3810.f};
constexpr float CT_Offset{1391.f};
///@}

//@name For A/WTIME
///@{
constexpr float AT_NORMAL_FACTOR = 2.4f;
constexpr float AT_NORMAL_MIN    = AT_NORMAL_FACTOR;        // 2.4ms
constexpr float AT_NORMAL_MAX    = 256 * AT_NORMAL_FACTOR;  // 614.4ms
//
constexpr float WT_NORMAL_FACTOR = AT_NORMAL_FACTOR;
constexpr float WT_NORMAL_MIN    = WT_NORMAL_FACTOR;        // 2.4ms
constexpr float WT_NORMAL_MAX    = 256 * WT_NORMAL_FACTOR;  // 614.4ms
constexpr float WT_LONG_FACTOR   = 2.4f * 12.f;
constexpr float WT_LONG_MIN      = WT_LONG_FACTOR;        // 28.8ms
constexpr float WT_LONG_MAX      = 256 * WT_LONG_FACTOR;  // 7372.8ms
///@}

/*!
  @struct Calibration
  @brief Raw value to determine black and white
 */
struct Calibration {
    Calibration(const uint16_t br, const uint16_t wr, const uint16_t bg, const uint16_t wg, const uint16_t bb,
                const uint16_t wb)
        : blackR{br}, whiteR{wr}, blackG{bg}, whiteG{wg}, blackB{bb}, whiteB{wb}
    {
        assert(wr > br && "White must be greater than black");
        assert(wg > bg && "White must be greater than black");
        assert(wb > bb && "White must be greater than black");
    }

    uint16_t blackR{}, whiteR{};
    uint16_t blackG{}, whiteG{};
    uint16_t blackB{}, whiteB{};

    //! @breif Get the read value with calibration
    inline uint8_t R8(const Data& d) const
    {
        return linear(d.RnoIR16(), blackR, whiteR);
    }
    //! @breif Get the read value with calibration
    inline uint8_t G8(const Data& d) const
    {
        return linear(d.GnoIR16(), blackG, whiteG);
    }
    //! @breif Get the read value with calibration
    inline uint8_t B8(const Data& d) const
    {
        return linear(d.BnoIR16(), blackB, whiteB);
    }

    //! @brief Linear interpolation in a specified range
    inline static uint8_t linear(const uint16_t raw, const uint16_t low, const uint16_t high)
    {
        return std::fmax(std::fmin(std::round((float)(raw - low) / (float)(high - low) * 255.f), 255.f), 0.0f);
    }
};

//! @brief ATIME to ms
inline float atime_to_ms(const uint8_t a)
{
    return 2.4f * (256 - a);
}

//! @brief WTIME and WLONG to ms
inline float wtime_to_ms(const uint8_t w, const bool wlong)
{
    return atime_to_ms(w) * (wlong ? 12 : 1);
}

//! @brief ms to ATIME
inline uint8_t ms_to_atime(const float ms)
{
    int tmp = 256 - std::round(ms / 2.4f);
    return static_cast<uint8_t>(std::max(std::min(tmp, 0xFF), 0x00));
}

//! @brief ms to WTIME and WLONG
std::tuple<uint8_t, bool> ms_to_wtime(const float ms);

/*!
  @brief Calculate Lux
  @param rawR Raw red value
  @param rawG Raw green value
  @param rawB Raw blue value
  @param rawC Raw clear value
  @param atime_ms Integtation time(ms)
  @param gc Gain
  @param dgf Device and Glass Factor
  @param coefR Coefficient for the R channel
  @param coefG Coefficient for the G channel
  @param coefB Coefficient for the B channel
  @return Lux (lx)
 */
float calculateLux(const uint16_t rawR, const uint16_t rawG, const uint16_t rawB, const uint16_t rawC,
                   const float atime_ms, const Gain gc, const float dgf = DGF, const float coefR = R_Coef,
                   const float coefG = G_Coef, const float coefB = B_Coef);

/*!
  @brief Calculate color temperature(degrees Kelvin)
  @param rawR Raw red value
  @param rawG Raw green value
  @param rawB Raw blue value
  @param rawC Raw clear value
  @param coefCT Coefficient for the color temperature
  @param offsetCT Offset for the the color temperature
  @return Color temperature(degrees Kelvin)
 */
float calculateColorTemperature(const uint16_t rawR, const uint16_t rawG, const uint16_t rawB, const uint16_t rawC,
                                const float coefCT = CT_Coef, const float offsetCT = CT_Offset);

/*!
  @brief Calculate Saturation
  @param atime ATIME raw value
  @return Saturation value
  @note Ripple saturation is also considered (in Analog)
 */
uint16_t calculateSaturation(const uint8_t atime);

/*!
  @brief Calculate Saturation
  @param atime_ms ATIME ms value
  @return Saturation value
  @note Ripple saturation is also considered (in Analog)
 */
inline uint16_t calculateSaturation(const float atime_ms)
{
    return calculateSaturation(ms_to_atime(atime_ms));
}

/*!
  @brief Calculate CRATIO
  @param rawR raw red value
  @param rawG raw green value
  @param rawB raw blue value
  @param rawC raw clear value
  @return CRATIO (0.0f - 1.0f)
  Guidelines for CRATIO and Light Source Correspondence
  |Compare|Description|
  |---|---|
  |CRATIO < 0.1 | LED or fluorescent light |
  |0.1 <= CRATIO < 0.25| Sunlight|
  |0.25 <0 CRATIO | Incandescent light |
*/
float calculateCRATIO(const uint16_t rawR, const uint16_t rawG, const uint16_t rawB, const uint16_t rawC);

/*!
  @brief Calculate Counts per Lux (CPL)
  @param atime_ms integtation time(ms)
  @param gc Gain
  @param dgf Device and Glass Factor
  @return CPL
*/
float calculateCPL(const float atime_ms, const Gain gc, const float dgf = DGF);

/*!
  @brief Calculate maximum Lux
  @param atime_ms integtation time(ms)
  @param gc Gain
  @param dgf Device and Glass Factor
  @return Lux(lx)
 */
inline float calculateMaxLux(const float atime_ms, const Gain gc, const float dgf = DGF)
{
    return 65535.0f / (3.0f * calculateCPL(atime_ms, gc, dgf));
}

///@cond
template <typename T, T... Ints>
struct integer_sequence {
    typedef T value_type;
    static constexpr std::size_t size()
    {
        return sizeof...(Ints);
    }
};

// Porting C++14 features to C++11
// clang-format off
template <size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

template <typename T, size_t N, T... Is>
struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Is...> {};

template <typename T, T... Is>
struct make_integer_sequence<T, 0, Is...> : integer_sequence<T, Is...> {};

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;
// clang-format on

// constexpr version of std::pow is GCC4.6.1 or higher
#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 6 || (__GNUC_MINOR__ == 6 && __GNUC_PATCHLEVEL__ > 0)))
#define M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CAN_CONSTEXPR_MAKE_GAMMA_TABLE
#define M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CONSTEXPR constexpr
#else
#define M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CONSTEXPR
#endif

M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CONSTEXPR uint8_t calculate_gamma(const float x, const float gamma)
{
    return static_cast<uint8_t>(std::pow(x, gamma) * 255);
}

template <size_t... Indices>
M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CONSTEXPR std::array<uint8_t, sizeof...(Indices)> make_gamma_table_impl(
    const float gamma, index_sequence<Indices...>)
{
    return {calculate_gamma(static_cast<float>(Indices) / 255.0f, gamma)...};
}
///@endcond

/*!
  @brief Make gamma table
  @tparam Size Table size
  @param gamma Gamma value
  @return std::array<uint8_t, Size> gamma table
  @note Make on compile time if it can
 */
template <size_t Size = 256>
M5_UNIT_COLOR_UTILITY_UNIT_COLOR_UTILITY_CONSTEXPR std::array<uint8_t, Size> make_gamma_table(const float gamma = 2.2f)
{
    return make_gamma_table_impl(gamma, make_index_sequence<Size>{});
}

}  // namespace tcs3472x
}  // namespace unit
}  // namespace m5
#endif
