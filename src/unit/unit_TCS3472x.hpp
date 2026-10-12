/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_TCS3472x.hpp
  @brief TCS3472x Unit for M5UnitUnified
*/
#ifndef M5_UNIT_COLOR_UNIT_TCS3472X_HPP
#define M5_UNIT_COLOR_UNIT_TCS3472X_HPP

#include "unit_TCS3472x_data.hpp"
#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <array>

namespace m5 {
namespace unit {

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
        float atime{614.4f};
        //! Wait time(ms) if start on begin
        float wtime{2.4f};
        //! Gain if start on begin
        tcs3472x::Gain gain{tcs3472x::Gain::Controlx4};
    };

    /*!
      @brief Constructor
      @param addr I2C address
     */
    explicit UnitTCS3472x(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<tcs3472x::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    //! @brief Destructor
    virtual ~UnitTCS3472x()
    {
    }

    //! @brief Begin communication with the sensor
    //! @return True if initialization succeeded
    virtual bool begin() override;
    //! @brief Update periodic measurement data
    //! @param force If true, update immediately without waiting for the configured interval
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    //! @brief Gets the configuration
    inline config_t config() const
    {
        return _cfg;
    }
    //! @brief Set the configuration
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
      @brief Read the RGBC integration time (ATIME)
      @param[out] raw Raw ATIME value
      @return True if successful
     */
    bool readAtime(uint8_t& raw);
    /*!
      @brief Read the RGBC integration time (ATIME)
      @param[out] ms ATIME in ms
      @return True if successful
     */
    bool readAtime(float& ms);
    /*!
      @brief Write the RGBC integration time (ATIME)
      @param raw Raw ATIME value
      @return True if successful
     */
    template <typename T, typename std::enable_if<std::is_integral<T>::value, std::nullptr_t>::type = nullptr>
    inline bool writeAtime(const T raw)
    {
        return write_atime(static_cast<uint8_t>(raw));
    }
    /*!
      @brief Write the RGBC integration time (ATIME)
      @param ms ATIME in ms
      @return True if successful, false if ms is out of range or writing fails
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
      @param[out] ms WTIME in ms
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
      @return True if successful, false if ms is out of range or writing fails
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
    /*!
      @brief Start periodic measurement using current settings
      @return True if successful
    */
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
      @param[out] d Measured data
      @param gc Gain
      @param atime Integration time(ms)
      @return True if successful
      @warning During periodic measurement, an error is returned
      @warning Each setting is overwritten
    */
    bool measureSingleshot(tcs3472x::Data& d, const tcs3472x::Gain gc, const float atime);
    /*!
      @brief Measurement single shot using current settings
      @param[out] d Measured data
      @return True if successful
      @warning During periodic measurement, an error is returned
    */
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

    /*!
      @brief Clear interrupt
      @return True if successful
     */
    bool clearInterrupt();
    ///@}

    /*!
      @brief Read the status register
      @param[out] status Status register value
      @return True if successful
     */
    bool readStatus(uint8_t& status);

protected:
    inline virtual bool is_valid_id(const uint8_t /*id*/) const
    {
        return false;
    }

    bool read_register8(const uint8_t reg, uint8_t& val);
    bool write_register8(const uint8_t reg, const uint8_t val);
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
    //! @brief Constructor
    //! @param addr I2C address
    explicit UnitTCS34725(const uint8_t addr = DEFAULT_ADDRESS) : UnitTCS3472x(addr)
    {
    }
    //! @brief Destructor
    virtual ~UnitTCS34725()
    {
    }
    //! @brief Device ID for TCS34725
    static constexpr uint8_t UNIT_ID{0x44};

protected:
    inline virtual bool is_valid_id(const uint8_t id) const override
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
    //! @brief Constructor
    //! @param addr I2C address
    explicit UnitTCS34727(const uint8_t addr = DEFAULT_ADDRESS) : UnitTCS3472x(addr)
    {
    }
    //! @brief Destructor
    virtual ~UnitTCS34727()
    {
    }
    //! @brief Device ID for TCS34727
    static constexpr uint8_t UNIT_ID{0x4D};

protected:
    inline virtual bool is_valid_id(const uint8_t id) const override
    {
        return id == UNIT_ID;
    }
};

///@cond INTERNAL
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
