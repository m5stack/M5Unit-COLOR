/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_TCS3472x_data.hpp
  @brief Data types for TCS3472x
  @details Kept apart from unit_TCS3472x.hpp so that the data types and the color utilities can be used (and tested
  on the host) without M5UnitUnified
*/
#ifndef M5_UNIT_COLOR_UNIT_TCS3472X_DATA_HPP
#define M5_UNIT_COLOR_UNIT_TCS3472X_DATA_HPP

#include <algorithm>
#include <array>
#include <cstdint>

namespace m5 {
namespace unit {

/*!
  @namespace tcs3472x
  @brief For TCS3472x
 */
namespace tcs3472x {

/*!
  @enum Persistence
  @brief Interrupt persistence
 */
enum class Persistence : uint8_t {
    Every,    //!< Every RGBC cycle generates an interrupt
    Cycle1,   //!< 1 clear channel value outside of threshold range
    Cycle2,   //!< 2 clear channel value outside of threshold range
    Cycle3,   //!< 3 clear channel value outside of threshold range
    Cycle5,   //!< 5 clear channel value outside of threshold range
    Cycle10,  //!< 10 clear channel value outside of threshold range
    Cycle15,  //!< 15 clear channel value outside of threshold range
    Cycle20,  //!< 20 clear channel value outside of threshold range
    Cycle25,  //!< 25 clear channel value outside of threshold range
    Cycle30,  //!< 30 clear channel value outside of threshold range
    Cycle35,  //!< 35 clear channel value outside of threshold range
    Cycle40,  //!< 40 clear channel value outside of threshold range
    Cycle45,  //!< 45 clear channel value outside of threshold range
    Cycle50,  //!< 50 clear channel value outside of threshold range
    Cycle55,  //!< 55 clear channel value outside of threshold range
    Cycle60,  //!< 60 clear channel value outside of threshold range
};

/*!
  @enum Gain
  @brief RGBC Gain Control
 */
enum class Gain : uint8_t {
    Controlx1,   //!< 1x gain
    Controlx4,   //!< 4x gain
    Controlx16,  //!< 16x gain
    Controlx60,  //!< 60x gain
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 8> raw{};  //!< Raw data ClCh/RlRh/GlGh/BlBh

    ///@name Raw value
    ///@{
    //! @brief Gets the raw red value
    inline uint16_t R16() const
    {
        return (static_cast<uint16_t>(raw[3]) << 8) | raw[2];
    }
    //! @brief Gets the raw green value
    inline uint16_t G16() const
    {
        return (static_cast<uint16_t>(raw[5]) << 8) | raw[4];
    }
    //! @brief Gets the raw blue value
    inline uint16_t B16() const
    {
        return (static_cast<uint16_t>(raw[7]) << 8) | raw[6];
    }
    //! @brief Gets the raw clear value
    inline uint16_t C16() const
    {
        return (static_cast<uint16_t>(raw[1]) << 8) | raw[0];
    }
    //! @brief Gets the raw red value without IR component
    inline uint16_t RnoIR16() const
    {
        return std::max(std::min(R16() - IR(), static_cast<int32_t>(0xFFFF)), static_cast<int32_t>(0x0000));
    }
    //! @brief Gets the raw green value without IR component
    inline uint16_t GnoIR16() const
    {
        return std::max(std::min(G16() - IR(), static_cast<int32_t>(0xFFFF)), static_cast<int32_t>(0x0000));
    }
    //! @brief Gets the raw blue value without IR component
    inline uint16_t BnoIR16() const
    {
        return std::max(std::min(B16() - IR(), static_cast<int32_t>(0xFFFF)), static_cast<int32_t>(0x0000));
    }
    //! @brief Gets the raw clear value without IR component
    inline uint16_t CnoIR16() const
    {
        return std::max(std::min(C16() - IR(), static_cast<int32_t>(0xFFFF)), static_cast<int32_t>(0x0000));
    }
    ///@}

    ///@name RGB
    ///@{
    //! @brief Gets the red value (0-255)
    inline uint8_t R8() const
    {
        return raw_to_uint8(R16(), C16());
    }
    //! @brief Gets the green value (0-255)
    inline uint8_t G8() const
    {
        return raw_to_uint8(G16(), C16());
    }
    //! @brief Gets the blue value (0-255)
    inline uint8_t B8() const
    {
        return raw_to_uint8(B16(), C16());
    }
    //! @brief Gets the red value without IR component (0-255)
    inline uint8_t RnoIR8() const
    {
        return raw_to_uint8(RnoIR16(), CnoIR16());
    }
    //! @brief Gets the green value without IR component (0-255)
    inline uint8_t GnoIR8() const
    {
        return raw_to_uint8(GnoIR16(), CnoIR16());
    }
    //! @brief Gets the blue value without IR component (0-255)
    inline uint8_t BnoIR8() const
    {
        return raw_to_uint8(BnoIR16(), CnoIR16());
    }

    //! @brief Gets the value in RGB565 format
    inline uint16_t RGB565() const
    {
        return color565(R8(), G8(), B8());
    }
    //! @brief Gets the value in RGB888 format
    inline uint32_t RGB888() const
    {
        return color888(R8(), G8(), B8());
    }
    //! @brief Gets the value in RGB565 format without IR component
    inline uint16_t RGBnoIR565() const
    {
        return color565(RnoIR8(), GnoIR8(), BnoIR8());
    }
    //! @brief Gets the value in RGB888 format without IR component
    inline uint32_t RGBnoIR888() const
    {
        return color888(RnoIR8(), GnoIR8(), BnoIR8());
    }
    ///@}

    /*!
      @brief Gets the IR component
      @param usingCache If true, use cached value when available
      @return IR component value
      @note The cache is rebuilt each time the unit reads a measurement into this object.
      If raw is modified directly, call with usingCache = false
     */
    inline int32_t IR(const bool usingCache = true) const
    {
        if (!usingCache || !_cacheValid) {
            _cache      = static_cast<int32_t>((static_cast<int32_t>(R16()) + static_cast<int32_t>(G16()) +
                                           static_cast<int32_t>(B16()) - static_cast<int32_t>(C16())) *
                                          0.5f);
            _cacheValid = true;
        }
        return _cache;
    }

    /*!
      @brief Raw to uint8_t
      @param v Raw channel value
      @param c Raw clear channel value
      @return Scaled uint8_t value
     */
    inline static uint8_t raw_to_uint8(const int32_t v, const int32_t c)
    {
        return std::max(std::min(static_cast<int>(c ? (static_cast<float>(v) / c) * 255.f : 0), 0xFF), 0x00);
    }

    ///@name Conversion (Same as M5GFX)
    ///@{
    /*!
      @brief Converts 8-bit RGB channels to RGB332 format
      @param r Red (0-255)
      @param g Green (0-255)
      @param b Blue (0-255)
      @return RGB332 value
     */
    inline static constexpr uint8_t color332(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        return ((((r >> 5) << 3) + (g >> 5)) << 2) + (b >> 6);
    }
    /*!
      @brief Converts 8-bit RGB channels to RGB565 format
      @param r Red (0-255)
      @param g Green (0-255)
      @param b Blue (0-255)
      @return RGB565 value
     */
    inline static constexpr uint16_t color565(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        return (r >> 3) << 11 | (g >> 2) << 5 | b >> 3;
    }
    /*!
      @brief Converts 8-bit RGB channels to RGB888 format
      @param r Red (0-255)
      @param g Green (0-255)
      @param b Blue (0-255)
      @return RGB888 value
     */
    inline static constexpr uint32_t color888(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        return r << 16 | g << 8 | b;
    }
    /*!
      @brief Converts 8-bit RGB channels to byte-swapped RGB565 format
      @param r Red (0-255)
      @param g Green (0-255)
      @param b Blue (0-255)
      @return byte-swapped RGB565 value
     */
    inline static constexpr uint16_t swap565(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        return (((r >> 3) << 3) + (g >> 5)) | (((g >> 2) << 5) | (b >> 3)) << 8;
    }
    /*!
      @brief Converts 8-bit RGB channels to byte-swapped RGB888 format
      @param r Red (0-255)
      @param g Green (0-255)
      @param b Blue (0-255)
      @return byte-swapped RGB888 value
     */
    inline static constexpr uint32_t swap888(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        return b << 16 | g << 8 | r;
    }
    ///@}

private:
    mutable int32_t _cache{};    // IR component value cache
    mutable bool _cacheValid{};  // True if _cache holds a computed value
};

}  // namespace tcs3472x

}  // namespace unit
}  // namespace m5
#endif
