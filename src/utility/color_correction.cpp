/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file color_correction.cpp
  @brief Color corrections for 8-bit RGB values
*/
#include "color_correction.hpp"
#include <algorithm>

namespace {

using m5::unit::tcs3472x::ColorName;
using m5::unit::tcs3472x::RGB8;

// Below this value (max channel) the color is Black
constexpr int32_t BLACK_VALUE_MAX{40};
// Below this saturation (percent) the color is White or Gray
constexpr int32_t GRAY_SATURATION_MAX{20};
// At or above this value (max channel) a low saturation color is White
constexpr int32_t WHITE_VALUE_MIN{200};
// Below this value (max channel) an orange hue is Brown
constexpr int32_t BROWN_VALUE_MAX{150};

struct HueRange {
    int32_t upper;  // Exclusive upper bound of the hue (degrees)
    ColorName name;
};
// Red wraps around: [0, 15) and [345, 360)
constexpr HueRange hue_table[] = {
    {15, ColorName::Red},     {45, ColorName::Orange},   {70, ColorName::Yellow},
    {165, ColorName::Green},  {195, ColorName::Cyan},    {255, ColorName::Blue},
    {290, ColorName::Purple}, {345, ColorName::Magenta}, {360, ColorName::Red},
};

struct NamedColor {
    RGB8 rgb;
    const char* str;
};
// Indexed by ColorName
constexpr NamedColor named_table[] = {
    {{0, 0, 0}, "Black"},      {{255, 255, 255}, "White"}, {{128, 128, 128}, "Gray"},  {{255, 0, 0}, "Red"},
    {{255, 165, 0}, "Orange"}, {{255, 255, 0}, "Yellow"},  {{0, 255, 0}, "Green"},     {{0, 255, 255}, "Cyan"},
    {{0, 0, 255}, "Blue"},     {{128, 0, 255}, "Purple"},  {{255, 0, 255}, "Magenta"}, {{139, 69, 19}, "Brown"},
};
static_assert(sizeof(named_table) / sizeof(named_table[0]) == static_cast<size_t>(ColorName::Brown) + 1,
              "named_table must cover every ColorName");

// v * 255 / range, rounded
inline uint8_t scale_to_255(const int32_t v, const int32_t range)
{
    return static_cast<uint8_t>((v * 255 + range / 2) / range);
}

// Hue in degrees [0, 360). hi > lo is required
int32_t hue_degrees(const int32_t r, const int32_t g, const int32_t b, const int32_t hi, const int32_t lo)
{
    const int32_t d = hi - lo;
    int32_t h{};
    if (hi == r) {
        h = 60 * (g - b) / d;
    } else if (hi == g) {
        h = 120 + 60 * (b - r) / d;
    } else {
        h = 240 + 60 * (r - g) / d;
    }
    return (h < 0) ? h + 360 : h;
}

}  // namespace

namespace m5 {
namespace unit {
namespace tcs3472x {

RGB8 pureHue(const uint8_t r, const uint8_t g, const uint8_t b)
{
    const int32_t lo = std::min(std::min(r, g), b);
    const int32_t hi = std::max(std::max(r, g), b);
    if (hi == lo) {
        return RGB8{r, g, b};
    }
    const int32_t range = hi - lo;
    return RGB8{scale_to_255(r - lo, range), scale_to_255(g - lo, range), scale_to_255(b - lo, range)};
}

RGB8 normalizeBrightness(const uint8_t r, const uint8_t g, const uint8_t b)
{
    const int32_t hi = std::max(std::max(r, g), b);
    if (hi == 0) {
        return RGB8{r, g, b};
    }
    return RGB8{scale_to_255(r, hi), scale_to_255(g, hi), scale_to_255(b, hi)};
}

RGB8 grayscale(const uint8_t r, const uint8_t g, const uint8_t b)
{
    // BT.709 weights scaled to 256 (54 + 183 + 19)
    const uint8_t y = static_cast<uint8_t>((54 * r + 183 * g + 19 * b + 128) >> 8);
    return RGB8{y, y, y};
}

ColorName nearestColorName(const uint8_t r, const uint8_t g, const uint8_t b)
{
    const int32_t lo = std::min(std::min(r, g), b);
    const int32_t hi = std::max(std::max(r, g), b);
    if (hi < BLACK_VALUE_MAX) {
        return ColorName::Black;
    }
    if ((hi - lo) * 100 < GRAY_SATURATION_MAX * hi) {
        return (hi >= WHITE_VALUE_MIN) ? ColorName::White : ColorName::Gray;
    }
    const int32_t h = hue_degrees(r, g, b, hi, lo);
    for (auto&& e : hue_table) {
        if (h < e.upper) {
            return (e.name == ColorName::Orange && hi < BROWN_VALUE_MAX) ? ColorName::Brown : e.name;
        }
    }
    return ColorName::Red;
}

RGB8 colorNameToRGB(const ColorName name)
{
    return named_table[static_cast<size_t>(name)].rgb;
}

const char* colorNameToString(const ColorName name)
{
    return named_table[static_cast<size_t>(name)].str;
}

}  // namespace tcs3472x
}  // namespace unit
}  // namespace m5
