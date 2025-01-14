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

    ///@name Raw value
    ///@{
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
    //! @brief Gets the raw clear value
    inline uint16_t C16() const
    {
        return ((uint16_t)raw[1] << 8) | raw[0];
    }
    //! @brief Gets the raw red value without IR component
    inline uint16_t RnoIR16() const
    {
        return std::max(std::min(R16() - IR(), (int32_t)0xFFFF), (int32_t)0x0000);
    }
    //! @brief Gets the raw green value without IR component
    inline uint16_t GnoIR16() const
    {
        return std::max(std::min(G16() - IR(), (int32_t)0xFFFF), (int32_t)0x0000);
    }
    //! @brief Gets the raw blue value without IR component
    inline uint16_t BnoIR16() const
    {
        return std::max(std::min(B16() - IR(), (int32_t)0xFFFF), (int32_t)0x0000);
    }
    //! @brief Gets the raw clear value without IR component
    inline uint16_t CnoIR16() const
    {
        return std::max(std::min(C16() - IR(), (int32_t)0xFFFF), (int32_t)0x0000);
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
    //! @brief Gets the red value without IR component
    inline uint8_t RnoIR8() const
    {
        return raw_to_uint8(R16() - IR(), C16() - IR());
    }
    //! @brief Gets the green value without IR component
    inline uint8_t GnoIR8() const
    {
        return raw_to_uint8(G16() - IR(), C16() - IR());
    }
    //! @brief Gets the blue value without IR component
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

    //! @brief Gets the IR component
    inline int32_t IR(bool usingCache = true) const
    {
        return (usingCache && _cache)
                   ? _cache
                   : (_cache = static_cast<int32_t>(
                          ((int32_t)R16() + (int32_t)G16() + (int32_t)B16() - (int32_t)C16()) * 0.5f));
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

private:
    mutable int32_t _cache{};  // IR componet value cache
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
        //! RGBC integration time(ms) if start on begin
        float atime{614.f};
        //! Wait time(ms) if start on begin
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
    /*!
      @brief Read the persistence
      @param[out] pers Persistence
      @return True if successful
     */
    bool readPersistence(tcs3472x::Persistence& pers);
    /*!
      @brief Write the persistence
      @param pers Persistence
      @return True if successful
     */
    bool writePersistence(const tcs3472x::Persistence pers);
    /*!
      @brief Read the gain control
      @param[out] gc Gain control
      @return True if successful
     */
    bool readGain(tcs3472x::Gain& gc);
    /*!
      @brief Write the gain control
      @param gc Gain control
      @return True if successful
     */
    bool writeGain(const tcs3472x::Gain gc);
    /*!
      @brief Read the The RGBC integration time (ATIME)
      @param[out] raw Raw ATIME value
      @return True if successful
     */
    bool readAtime(uint8_t& raw);
    /*!
      @brief Read the The RGBC integration time (ATIME)
      @param[out] ms ATIME in ms
      @return True if successful
     */
    bool readAtime(float& ms);
    /*!
      @brief Write the The RGBC integration time (ATIME)
      @param raw Raw ATIME value
      @return True if successful
     */
    template <typename T, typename std::enable_if<std::is_integral<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeAtime(const T raw)
    {
        return write_atime((uint8_t)raw);
    }
    /*!
      @brief Write the The RGBC integration time (ATIME)
      @param raw ATIME in ms
      @return True if successful
      @note Converted to approximate raw values and set
      @warning Valid range 2.4 - 614.4
     */
    bool writeAtime(const float ms);
    /*!
      @brief Read the wait time (WTIME)
      @param[out] raw Raw WTIME value
      @param[out] wlong x12 longer?
      @return True if successful
     */
    bool readWtime(uint8_t& raw, bool& wlong);
    /*!
      @brief Read the wait time (WTIME)
      @param[out] WTIME in ms
      @return True if successful
     */
    bool readWtime(float& ms);
    /*!
      @brief Write the wait time (WTIME)
      @param raw Raw WTIME value
      @param wlong x12 longer?
      @return True if successful
     */
    bool writeWtime(const uint8_t raw, const bool wlong);
    /*!
      @brief Write the wait time (WTIME)
      @param ms WTIME in ms
      @return True if successful
      @note Converted to approximate raw values and set
      @warning Valid range 2.4 - 7372.8
     */
    bool writeWtime(const float ms);
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param gc Gain
      @param atime Integration time(ms)
      @param wtime Wait time(ms)
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
      @param power_off To power off if true
      @return True if successful
    */
    inline bool stopPeriodicMeasurement(const bool power_off = true)
    {
        return PeriodicMeasurementAdapter<UnitTCS3472x, tcs3472x::Data>::stopPeriodicMeasurement(power_off);
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @param[out] data Measuerd data
      @param gc Gain
      @param atime Integration time(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Each setting is overwritten
    */
    bool measureSingleshot(tcs3472x::Data& d, const tcs3472x::Gain gc, const float atime);
    //! @brief Measurement single shot using current settings
    bool measureSingleshot(tcs3472x::Data& d);
    ///@}

    ///@name Interrupt
    ///@{
    /*!
      @brief Read the interrupt status
      @param[out] enable true if interrupt is enabled
      @return True if successful
     */
    bool readInterrupt(bool& enable);
    /*!
      @brief Write the interrupt status
      @param enable true if interrupt is to be enabled
      @return True if successful
     */
    bool writeInterrupt(const bool enable);
    /*!
      @brief Read the interrupt thresholds for clear channel
      @param[out] low Low threshold value
      @param[out] high High threshold value
      @return True if successful
     */
    bool readInterruptThreshold(uint16_t& low, uint16_t& high);
    /*!
      @brief Write the interrupt thresholds for clear channel
      @param low Low threshold value
      @param high High threshold value
      @return True if successful
     */
    bool writeInterruptThreshold(const uint16_t low, const uint16_t high);

    //! @brief Clear interrupt
    bool clearInterrupt();
    ///@}

    bool readStatus(uint8_t& status);

protected:
    inline virtual bool is_valid_id(const uint8_t id)
    {
        return false;
    }

    bool read_register8(const uint8_t reg, uint8_t& val);
    bool write_register8(const uint8_t reg, const uint8_t val);
    // bool read_register16(const uint8_t reg, uint16_t& val);
    // bool write_register16(const uint8_t reg, const uint16_t val);
    bool read_register(const uint8_t reg, uint8_t* buf, const uint32_t len);
    bool write_register(const uint8_t reg, const uint8_t* buf, const uint32_t len);

    bool start_periodic_measurement(const tcs3472x::Gain gc, const float atime, const float wtime);
    bool start_periodic_measurement();
    bool stop_periodic_measurement(const bool power_off);

    bool is_data_ready();
    bool read_measurement(tcs3472x::Data& d);

    bool write_atime(const uint8_t raw);

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
