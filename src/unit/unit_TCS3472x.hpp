/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_TCS3472x.hpp
  @brief TCS3472x Unit for M5UnitUnified
*/
#ifndef M5_UNIT_COLOR_UNIT_TCS3472_HPP
#define M5_UNIT_COLOR_UNIT_TCS3472_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <array>

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
    std::array<uint8_t, 8> raw{};  // Raw data ClCh/RlRh/GlGh/BlBh

    //! @brief Gets the raw clear value
    inline uint16_t C16() const
    {
        return ((uint16_t)raw[1] << 8) | raw[0];
    }
    //! @brief Gets the raw red value
    inline uint16_t R16() const
    {
        return ((uint16_t)raw[3] << 8) | raw[2];
    }
    //! @brief Gets the raw green value
    inline uint16_t G16() const
    {
        return ((uint16_t)raw[5] << 8) | raw[4];
    }
    //! @brief Gets the raw blue value
    inline uint16_t B16() const
    {
        return ((uint16_t)raw[7] << 8) | raw[6];
    }
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
    //! @brief Gets the red value with IR component removed
    inline uint8_t RnoIR8() const
    {
        return raw_to_uint8(R16() - IR(), C16() - IR());
    }
    //! @brief Gets the green value with IR component removed
    inline uint8_t GnoIR8() const
    {
        return raw_to_uint8(G16() - IR(), C16() - IR());
    }
    //! @brief Gets the blue value with IR component removed
    inline uint8_t BnoIR8() const
    {
        return raw_to_uint8(B16() - IR(), C16() - IR());
    }

    //! @brief Gets the value in RGB565 format
    inline uint16_t RGB565() const
    {
        return color565(R8(), G8(), B8());
    }
    //! @brief Gets the value in RGB888 format
    inline uint32_t RGBC888() const
    {
        return color888(R8(), G8(), B8());
    }
    //! @brief Gets the value in RGB565 format with IR component removed
    inline uint16_t RGBnoIR565() const
    {
        return color565(RnoIR8(), GnoIR8(), BnoIR8());
    }

    //! @brief Gets the IR component
    inline int32_t IR() const
    {
        return ((int32_t)R16() + (int32_t)G16() + (int32_t)B16() - (int32_t)C16()) / 2;
    }

    //! @brief Raw to uint8_t
    inline static uint8_t raw_to_uint8(const int32_t v, const int32_t c)
    {
        return std::max(std::min(static_cast<int>(c ? ((float)v / c) * 255.f : 0), 0xFF), 0x00);
    }

    ///@name Conversion (Same as M5GFX)
    ///@{
    inline static constexpr uint8_t color332(uint8_t r, uint8_t g, uint8_t b)
    {
        return ((((r >> 5) << 3) + (g >> 5)) << 2) + (b >> 6);
    }
    inline static constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
    {
        return (r >> 3) << 11 | (g >> 2) << 5 | b >> 3;
    }
    inline static constexpr uint32_t color888(uint8_t r, uint8_t g, uint8_t b)
    {
        return r << 16 | g << 8 | b;
    }
    inline static constexpr uint16_t swap565(uint8_t r, uint8_t g, uint8_t b)
    {
        return (((r >> 3) << 3) + (g >> 5)) | (((g >> 2) << 5) | (b >> 3)) << 8;
    }
    inline static constexpr uint32_t swap888(uint8_t r, uint8_t g, uint8_t b)
    {
        return b << 16 | g << 8 | r;
    }
    ///@}
};

}  // namespace tcs3472x

/*!
  @class UnitTCS3472x
  @brief Color recognition unit
  @details Base class for TCS34725/7
*/
class UnitTCS3472x : public Component, public PeriodicMeasurementAdapter<UnitTCS3472x, tcs3472x::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitTCS3472x, 0x00);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! RGBC integration time if start on begin
        float atime{614.f};
        //! Wait time if start on begin
        float wtime{2.4f};
        //! Gain if start on begin
        tcs3472x::Gain gain{tcs3472x::Gain::Controlx4};
    };

    explicit UnitTCS3472x(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<tcs3472x::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitTCS3472x()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest measured Red
    inline uint8_t R8() const
    {
        return !empty() ? oldest().R8() : 0U;
    }
    //! @brief Oldest measured Green
    inline uint8_t G8() const
    {
        return !empty() ? oldest().G8() : 0U;
    }
    //! @brief Oldest measured Blue
    inline uint8_t B8() const
    {
        return !empty() ? oldest().B8() : 0U;
    }
    //! @brief Oldest measured RGB565
    inline uint16_t RGB565() const
    {
        return !empty() ? oldest().RGB565() : 0U;
    }
    ///@}

    ///@name Settings
    ///@{
    bool readInterruptThreshold(uint16_t& low, uint16_t& high);
    bool writeInterruptThreshold(const uint16_t low, const uint16_t high);

    bool readPersistence(tcs3472x::Persistence& pers);
    bool writePersistence(const tcs3472x::Persistence pers);

    bool readGain(tcs3472x::Gain& gc);
    bool writeGain(const tcs3472x::Gain gc);

    bool readAtime(uint8_t& raw);
    bool readAtime(float& ms);
    bool writeAtime(const uint8_t raw);
    bool writeAtime(const float ms);

    bool readWtime(uint8_t& raw, bool& wlong);
    bool readWtime(float& ms);
    bool writeWtime(const uint8_t raw, const bool wlong);
    bool writeWtime(const float ms);

    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const tcs3472x::Gain gc, const float atime, const float wtime)
    {
        return PeriodicMeasurementAdapter<UnitTCS3472x, tcs3472x::Data>::startPeriodicMeasurement(gc, atime, wtime);
    }
    //! @brief Start periodic measurement using current settings
    inline bool startPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitTCS3472x, tcs3472x::Data>::startPeriodicMeasurement();
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitTCS3472x, tcs3472x::Data>::stopPeriodicMeasurement();
    }
    ///@}

protected:
    inline virtual bool is_valid_id(const uint8_t id)
    {
        return false;
    }

    bool read_register8(const uint8_t reg, uint8_t& val);
    bool write_register8(const uint8_t reg, const uint8_t val);
    bool read_register16(const uint8_t reg, uint16_t& val);
    bool write_register16(const uint8_t reg, const uint16_t val);
    bool read_register(const uint8_t reg, uint8_t* buf, const uint32_t len);
    bool write_register(const uint8_t reg, const uint8_t* buf, const uint32_t len);

    bool start_periodic_measurement(const tcs3472x::Gain gc, const float atime, const float wtime);
    bool start_periodic_measurement();
    bool stop_periodic_measurement();

    bool is_data_ready();
    bool read_measurement(tcs3472x::Data& d);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitTCS3472x, tcs3472x::Data);

private:
    std::unique_ptr<m5::container::CircularBuffer<tcs3472x::Data>> _data{};
    config_t _cfg{};
};

/*!
  @class UnitTCS34725
  @brief Color recognition unit
 */
class UnitTCS34725 : public UnitTCS3472x {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitTCS34725, 0x29);

public:
    explicit UnitTCS34725(const uint8_t addr = DEFAULT_ADDRESS) : UnitTCS3472x(addr)
    {
    }
    virtual ~UnitTCS34725()
    {
    }
    static constexpr uint8_t UNIT_ID{0x44};

protected:
    inline virtual bool is_valid_id(const uint8_t id) override
    {
        return id == UNIT_ID;
    }
};

/*!
  @class UnitTCS34727
  @brief Color recognition unit
 */
class UnitTCS34727 : public UnitTCS3472x {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitTCS34727, 0x29);

public:
    explicit UnitTCS34727(const uint8_t addr = DEFAULT_ADDRESS) : UnitTCS3472x(addr)
    {
    }
    virtual ~UnitTCS34727()
    {
    }
    static constexpr uint8_t UNIT_ID{0x4D};

protected:
    inline virtual bool is_valid_id(const uint8_t id) override
    {
        return id == UNIT_ID;
    }
};

///@cond 0
namespace tcs3472x {
namespace command {
// R/W
constexpr uint8_t ENABLE_REG{0x00};
constexpr uint8_t ATIME_REG{0x01};
constexpr uint8_t WTIME_REG{0x03};
constexpr uint8_t AILTL_REG{0x04};
constexpr uint8_t AILTH_REG{0x05};
constexpr uint8_t AIHTL_REG{0x06};
constexpr uint8_t AIHTH_REG{0x07};
constexpr uint8_t PERS_REG{0x0C};
constexpr uint8_t CONFIG_REG{0x0D};
constexpr uint8_t CONTROL_REG{0x0F};
// R
constexpr uint8_t ID_REG{0x12};
constexpr uint8_t STATUS_REG{0x13};
constexpr uint8_t CDATAL_REG{0x14};
constexpr uint8_t CDATAH_REG{0x15};
constexpr uint8_t RDATAL_REG{0x16};
constexpr uint8_t RDATAH_REG{0x17};
constexpr uint8_t GDATAL_REG{0x18};
constexpr uint8_t GDATAH_REG{0x19};
constexpr uint8_t BDATAL_REG{0x1A};
constexpr uint8_t BDATAH_REG{0x1B};

}  // namespace command
}  // namespace tcs3472x
///@endcond

}  // namespace unit
}  // namespace m5
#endif
