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
#include <tuple>

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
constexpr float T_NORMAL_FACTOR = 2.4f;
constexpr float WT_LONG_FACTOR  = 2.4f * 12.f;
constexpr float T_NORMAL_MIN    = T_NORMAL_FACTOR;        // 2.4ms
constexpr float T_NORMAL_MAX    = 256 * T_NORMAL_FACTOR;  // 614.4ms
//    constexpr float WT_LONG_MIN      = WT_LONG_FACTOR;          // 28.8ms
constexpr float WT_LONG_MAX = 256 * WT_LONG_FACTOR;  // 7372.8ms
///@}

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

//! @brief ms to ATIME raw
inline uint8_t ms_to_atime(const float ms)
{
    int tmp = 256 - (ms / 2.4f);
    return static_cast<uint8_t>(std::max(std::min(tmp, 0xFF), 0x00));
}

//! @brief ms to WTIME and WLONG
inline std::tuple<uint8_t, bool> ms_to_wtime(const float ms)
{
    float clamped = std::fmax(std::fmin(ms, WT_LONG_MAX), T_NORMAL_MIN);
    bool wlong    = (clamped > T_NORMAL_MAX);
    float factor  = wlong ? WT_LONG_FACTOR : T_NORMAL_FACTOR;
    uint8_t wtime = static_cast<uint8_t>(std::round(256 - (clamped / factor)));
    return std::make_tuple(wtime, wlong);
}

/*!
  @brief Calculate Lux
  @param rawR raw red value
  @param rawG raw green value
  @param rawB raw blue value
  @param rawC raw clear value
  @param atime_ms integtation time(ms)
  @param gc Gain
  @param dgf Device and Glass Factor
  @param coefR Coefficient for the R channel
  @param coefG Coefficient for the G channel
  @param coefB Coefficient for the B channel
  @return Lux (lx)
 */
float calculateLux(const int32_t rawR, const int32_t rawG, const int32_t rawB, const int32_t rawC, const float atime_ms,
                   const Gain gc, const float dgf = DGF, const float coefR = R_Coef, const float coefG = G_Coef,
                   const float coefB = B_Coef);

/*!
  @brief Calculate color temperature(degrees Kelvin)
  @param rawR raw red value
  @param rawG raw green value
  @param rawB raw blue value
  @param rawC raw clear value
  @return color temperature(degrees Kelvin)
 */
float calculateColorTemperature(const int32_t rawR, const int32_t rawG, const int32_t rawB, const int32_t rawC);

/*!
  @brief calculate Saturation
  @param raw ATIME raw value
  @return Saturation value
  @note Ripple saturation is also considered (in Analog)
 */
uint16_t calculateSaturation(const uint8_t raw);

/*!
  @brief calculate Saturation
  @param ms ATIME ms value
  @return Saturation value
  @note Ripple saturation is also considered (in Analog)
 */
inline uint16_t calculateSaturation(const float ms)
{
    return calculateSaturation(ms_to_atime(ms));
}

/*!
  @brief Calculate CRATIO
  @param rawR raw red value
  @param rawG raw green value
  @param rawB raw blue value
  @param rawC raw clear value
  @return CRATIO (0.0f - 1.0f)
  |Compare|Description|
  |---|---|
  |CRATIO < 0.1 | LED or fluorescent light |
  |0.1 <= CRATIO < 0.25| Sunlight|
  |0.25 <0 CRATIO | Incandescent light |
  @note Correspondence between CRATIO and light source
*/
float calculateCRATIO(const int32_t rawR, const int32_t rawG, const int32_t rawB, const int32_t rawC);

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

template <size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

template <typename T, size_t N, T... Is>
struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Is...> {};

template <typename T, T... Is>
struct make_integer_sequence<T, 0, Is...> : integer_sequence<T, Is...> {};

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

template <typename... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

constexpr float c_pow(const float base, const float exp)
{
    return (exp <= 0.0f) ? 1.0f : (exp == 1.0f) ? base : base * c_pow(base, exp - 1.0f);
}

constexpr uint8_t calculate_gamma(const float x, const float gamma)
{
    return static_cast<uint8_t>(c_pow(x, static_cast<int>(gamma)) * 255);
}

template <size_t... Indices>
constexpr std::array<uint8_t, sizeof...(Indices)> make_gamma_table_impl(const float gamma, index_sequence<Indices...>)
{
    return {calculate_gamma(static_cast<float>(Indices) / 255.0f, gamma)...};
}
///@endcond

/*!
  @brief Make gamma table on compile time
  @tparam Size Table size
  @param gamma
 */
template <size_t Size = 256>
constexpr std::array<uint8_t, Size> make_gamma_table(const float gamma = 2.2f)
{
    return make_gamma_table_impl(gamma, make_index_sequence<Size>{});
}

}  // namespace tcs3472x
}  // namespace unit
}  // namespace m5
#endif
