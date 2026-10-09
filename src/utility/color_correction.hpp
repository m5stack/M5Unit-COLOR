/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file color_correction.hpp
  @brief Color corrections for 8-bit RGB values (pure hue, brightness normalization, grayscale, color names)
  @details Each function takes an 8-bit RGB value, such as Data::R8() or Calibration::R8(), and returns the corrected
  color. Pass calibrated values (Calibration) for the best results, so that white and gray have equal channels.
*/
#ifndef M5_UNIT_COLOR_UTILITY_COLOR_CORRECTION_HPP
#define M5_UNIT_COLOR_UTILITY_COLOR_CORRECTION_HPP

#include "../unit/unit_TCS3472x.hpp"
#include <cstdint>

namespace m5 {
namespace unit {
namespace tcs3472x {

/*!
  @struct RGB8
  @brief 8-bit RGB value
  @note Kept as a C++11 aggregate (no default member initializers), so RGB8{r, g, b} works. RGB8{} is zero
 */
struct RGB8 {
    uint8_t r;  //!< Red (0-255)
    uint8_t g;  //!< Green (0-255)
    uint8_t b;  //!< Blue (0-255)

    //! @brief Gets the value in RGB565 format
    inline uint16_t to565() const
    {
        return Data::color565(r, g, b);
    }
    //! @brief Gets the value in RGB888 format
    inline uint32_t to888() const
    {
        return Data::color888(r, g, b);
    }
};

/*!
  @brief Pure hue
  @details Removes the white component (the smallest channel) and scales the largest remaining channel to 255.
  The result is the most saturated color of the same hue (a dull blue becomes a vivid blue). Useful to classify
  colors by hue regardless of brightness and dullness
  @param r Red (0-255)
  @param g Green (0-255)
  @param b Blue (0-255)
  @return Pure hue color. Gray and white (all channels equal) are returned unchanged
 */
RGB8 pureHue(const uint8_t r, const uint8_t g, const uint8_t b);

/*!
  @brief Brightness normalization
  @details Scales all channels by the same factor so that the largest channel becomes 255. The hue and saturation are
  kept, so the color looks the same at different distances or illuminance
  @param r Red (0-255)
  @param g Green (0-255)
  @param b Blue (0-255)
  @return Normalized color. Black (all channels 0) is returned unchanged
 */
RGB8 normalizeBrightness(const uint8_t r, const uint8_t g, const uint8_t b);

/*!
  @brief Grayscale
  @details Luma by the ITU-R BT.709 weights (0.2126 R + 0.7152 G + 0.0722 B)
  @param r Red (0-255)
  @param g Green (0-255)
  @param b Blue (0-255)
  @return Gray color (r = g = b = luma)
 */
RGB8 grayscale(const uint8_t r, const uint8_t g, const uint8_t b);

/*!
  @enum ColorName
  @brief Color names for nearestColorName
 */
enum class ColorName : uint8_t {
    Black,    //!< Black
    White,    //!< White
    Gray,     //!< Gray
    Red,      //!< Red
    Orange,   //!< Orange
    Yellow,   //!< Yellow
    Green,    //!< Green
    Cyan,     //!< Cyan
    Blue,     //!< Blue
    Purple,   //!< Purple
    Magenta,  //!< Magenta
    Brown,    //!< Brown
};

/*!
  @brief Nearest color name
  @details Classifies the color by HSV: dark colors are Black, low saturation colors are White or Gray, and the others
  are named by hue (dark orange is Brown). The thresholds are rough guides for a typical indoor environment
  @param r Red (0-255)
  @param g Green (0-255)
  @param b Blue (0-255)
  @return Color name
 */
ColorName nearestColorName(const uint8_t r, const uint8_t g, const uint8_t b);

/*!
  @brief Representative color of the color name
  @param name Color name
  @return Representative color
 */
RGB8 colorNameToRGB(const ColorName name);

/*!
  @brief String of the color name
  @param name Color name
  @return Color name string (e.g. "Red")
 */
const char* colorNameToString(const ColorName name);

}  // namespace tcs3472x
}  // namespace unit
}  // namespace m5
#endif
