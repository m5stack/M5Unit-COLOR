/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_TCS3472x.cpp
  @brief TCS3472x Unit for M5UnitUnified
*/
#include "unit_TCS3472x.hpp"
#include "../utility/unit_color_utility.hpp"
#include <M5Utility.hpp>
#include <cmath>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::tcs3472x;
using namespace m5::unit::tcs3472x::command;

namespace {

//
struct Command {
    enum class Type : uint8_t { Repeated, AutoIncrement, Reserved, Special };
    static constexpr uint8_t CMD{0x80};  // Command bit

    explicit Command(const uint8_t reg, const Type t = Type::Repeated)
        : value{(uint8_t)(CMD | (reg & 0x1F) | (m5::stl::to_underlying(t) << 5)), 0x00}
    {
    }
    Command(const uint8_t reg, const uint8_t val) : value{(uint8_t)(CMD | (reg & 0x1F)), val}
    {
    }
    std::array<uint8_t, 2> value{};
};
constexpr uint8_t clear_channel_interrupt_clear{Command::CMD | 0x66};  // Special function

struct Enable {
    inline bool AIEN() const
    {
        return value & (1U << 4);
    }
    inline bool WEN() const
    {
        return value & (1U << 3);
    }
    inline bool AEN() const
    {
        return value & (1U << 1);
    }
    inline bool PON() const
    {
        return value & (1U << 0);
    }
    //
    inline void AIEN(const bool b)
    {
        value = (value & ~(1U << 4)) | ((b != false) << 4);
    }
    inline void WEN(const bool b)
    {
        value = (value & ~(1U << 3)) | ((b != false) << 3);
    }
    inline void AEN(const bool b)
    {
        value = (value & ~(1U << 1)) | ((b != false) << 1);
    }
    inline void PON(const bool b)
    {
        value = (value & ~(1U << 0)) | ((b != false) << 0);
    }
    uint8_t value{};
};

struct Config {
    inline bool WLONG() const
    {
        return value & (1U << 1);
    }
    inline void WLONG(const bool b)
    {
        value = (value & ~(1U << 1)) | ((b != false) << 1);
    }
    uint8_t value{};
};

struct Status {
    inline bool AINT() const
    {
        return value & (1U << 4);
    }
    inline bool AVALID() const
    {
        return value & (1U << 0);
    }
    uint8_t value{};
};

}  // namespace

namespace m5 {
namespace unit {

// class UnitTCS3472x
const char UnitTCS3472x::name[] = "UnitTCS3472x";
const types::uid_t UnitTCS3472x::uid{"UnitTCS3472x"_mmh3};
const types::uid_t UnitTCS3472x::attr{0};

bool UnitTCS3472x::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    uint8_t id{};
    if (!read_register8(ID_REG, id) || !is_valid_id(id)) {
        M5_LIB_LOGE("Cannot detect %s %X", deviceName(), id);
        return false;
    }

#if 0    
    uint16_t low{}, high{};
    Persistence pers{};
    Gain gc{};
    readInterruptThreshold(low, high);
    readPersistence(pers);
    readGain(gc);
    M5_LIB_LOGW("AI:%u/%u P:%x G:%X", low, high, pers, gc);

    float a{}, w{};
    readAtime(a);
    readWtime(w);
    M5_LIB_LOGE("AT:%f WT:%f", a, w);
#endif

    return _cfg.start_periodic ? start_periodic_measurement(_cfg.gain, _cfg.atime, _cfg.wtime) : true;
}

void UnitTCS3472x::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = is_data_ready() && read_measurement(d);
            if (_updated) {
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitTCS3472x::start_periodic_measurement(const tcs3472x::Gain gc, const float atime, const float wtime)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    return writeAtime(atime) && writeWtime(wtime) && writeGain(gc) && start_periodic_measurement();
}

bool UnitTCS3472x::start_periodic_measurement()
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Enable e{};
    float atime{}, wtime{};
    if (readAtime(atime) && readWtime(wtime) && read_register8(ENABLE_REG, e.value)) {
        bool needWait = !e.PON();
        e.PON(true);  // power on
        e.AEN(true);  // RGBC enable
        e.WEN(true);  // Wait enable
        _periodic = write_register8(ENABLE_REG, e.value);
        if (_periodic) {
            _latest   = 0;
            _interval = std::ceil(atime + wtime);
            if (needWait) {
                // Datashhet says
                // A minimum interval of 2.4 ms must pass after PON is asserted before an RGBC can be initiated
                m5::utility::delay(3);
            }
        }
    }
    return _periodic;
}

bool UnitTCS3472x::stop_periodic_measurement(const bool power_off)
{
    Enable e{};
    if (read_register8(ENABLE_REG, e.value)) {
        e.AEN(false);       // disable RGBC
        e.PON(!power_off);  // power off if true
        if (write_register8(ENABLE_REG, e.value)) {
            _periodic = false;
            return true;
        }
    }
    return false;
}

bool UnitTCS3472x::measureSingleshot(tcs3472x::Data& d, const tcs3472x::Gain gc, const float atime)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    return writeAtime(atime) && writeGain(gc) && measureSingleshot(d);
}

bool UnitTCS3472x::measureSingleshot(tcs3472x::Data& d)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Enable e{};
    float atime{};
    if (readAtime(atime) && read_register8(ENABLE_REG, e.value)) {
        bool needWait = !e.PON();
        e.PON(true);  // power on
        e.AEN(true);  // RGBC enable
        if (!write_register8(ENABLE_REG, e.value)) {
            return false;
        }
        uint32_t at     = std::ceil(atime);
        auto timeout_at = m5::utility::millis() + at + 1000;
        m5::utility::delay(at + needWait ? 3 : 0);  // Wait during ATIME
        do {
            if (is_data_ready() && read_measurement(d)) {
                return true;
            }
            m5::utility::delay(1);
        } while (m5::utility::millis() <= timeout_at);
    }
    return false;
}

bool UnitTCS3472x::readPersistence(Persistence& pers)
{
    uint8_t v{};
    if (read_register8(PERS_REG, v)) {
        pers = static_cast<Persistence>(v & 0x0F);
        return true;
    }
    return false;
}

bool UnitTCS3472x::writePersistence(const Persistence pers)
{
    uint8_t v{};
    if (read_register8(PERS_REG, v)) {
        v = (v & 0xF0) | (m5::stl::to_underlying(pers) & 0x0F);
        return write_register8(PERS_REG, v);
    }

    return false;
}

bool UnitTCS3472x::readGain(tcs3472x::Gain& gc)
{
    uint8_t v{};
    if (read_register8(CONTROL_REG, v)) {
        gc = static_cast<Gain>(v & 0x03);
        return true;
    }
    return false;
}

bool UnitTCS3472x::writeGain(const tcs3472x::Gain gc)
{
    return write_register8(CONTROL_REG, m5::stl::to_underlying(gc) & 0x03);
}

bool UnitTCS3472x::readAtime(uint8_t& raw)
{
    if (read_register8(ATIME_REG, raw)) {
        return true;
    }
    return false;
}

bool UnitTCS3472x::readAtime(float& ms)
{
    uint8_t v{};
    if (readAtime(v)) {
        ms = atime_to_ms(v);
        return true;
    }
    return false;
}

bool UnitTCS3472x::write_atime(const uint8_t raw)
{
    return write_register8(ATIME_REG, raw);
}

bool UnitTCS3472x::writeAtime(const float ms)
{
    if (!std::isfinite(ms) || (ms < AT_NORMAL_MIN || ms > AT_NORMAL_MAX)) {
        M5_LIB_LOGW("Valid range [%.1f - %.1f], %f", AT_NORMAL_MIN, AT_NORMAL_MAX, ms);
        return false;
    }

    float fa = std::fmax(std::fmin(ms, AT_NORMAL_MAX), AT_NORMAL_MIN);
    return write_atime(ms_to_atime(fa));
}

bool UnitTCS3472x::readWtime(uint8_t& raw, bool& wlong)
{
    Config c{};
    if (read_register8(WTIME_REG, raw) && read_register8(CONFIG_REG, c.value)) {
        wlong = c.WLONG();
        return true;
    }
    return false;
}

bool UnitTCS3472x::readWtime(float& ms)
{
    uint8_t raw{};
    bool wlong{};
    if (readWtime(raw, wlong)) {
        ms = wtime_to_ms(raw, wlong);
        return true;
    }
    return false;
}

bool UnitTCS3472x::writeWtime(const uint8_t raw, const bool wlong)
{
    Config c{};
    c.WLONG(wlong);
    return write_register8(WTIME_REG, raw) && write_register8(CONFIG_REG, c.value);
}

bool UnitTCS3472x::writeWtime(const float ms)
{
    if (!std::isfinite(ms) || (ms < WT_NORMAL_MIN || ms > WT_LONG_MAX)) {
        M5_LIB_LOGW("Valid range [%.1f - %.1f], %f", WT_NORMAL_MIN, WT_LONG_MAX, ms);
        return false;
    }

    float ft = std::fmax(std::fmin(ms, WT_LONG_MAX), WT_NORMAL_MIN);
    uint8_t raw{};
    bool wlong{};
    std::tie(raw, wlong) = ms_to_wtime(ft);
    return writeWtime(raw, wlong);
}

bool UnitTCS3472x::readInterrupt(bool& enable)
{
    Enable e{};
    if (read_register8(ENABLE_REG, e.value)) {
        enable = e.AIEN();
        return true;
    }
    return false;
}

bool UnitTCS3472x::writeInterrupt(const bool enable)
{
    Enable e{};
    if (read_register8(ENABLE_REG, e.value)) {
        e.AIEN(enable);
        return write_register8(ENABLE_REG, e.value);
    }
    return false;
}

bool UnitTCS3472x::readInterruptThreshold(uint16_t& low, uint16_t& high)
{
    uint8_t rbuf[4]{};
    if (read_register(AILTL_REG, rbuf, 4)) {
        low  = ((uint16_t)rbuf[1] << 8) | rbuf[0];
        high = ((uint16_t)rbuf[3] << 8) | rbuf[2];
        return true;
    }
    return false;
}

bool UnitTCS3472x::writeInterruptThreshold(const uint16_t low, const uint16_t high)
{
    uint8_t buf[4]{};
    buf[0] = low & 0xFF;
    buf[1] = (low >> 8) & 0xFF;
    buf[2] = high & 0xFF;
    buf[3] = (high >> 8) & 0xFF;
    return write_register(AILTL_REG, buf, 4);
}

bool UnitTCS3472x::clearInterrupt()
{
    return writeWithTransaction(&clear_channel_interrupt_clear, 1) == m5::hal::error::error_t::OK;
}

bool UnitTCS3472x::readStatus(uint8_t& status)
{
    return read_register8(STATUS_REG, status);
}

//
bool UnitTCS3472x::is_data_ready()
{
    Status s{};
    return read_register8(STATUS_REG, s.value) && s.AVALID();
}

bool UnitTCS3472x::read_measurement(tcs3472x::Data& d)
{
    return read_register(CDATAL_REG, d.raw.data(), d.raw.size());
}

bool UnitTCS3472x::read_register8(const uint8_t reg, uint8_t& val)
{
    Command cmd{reg};
    return (writeWithTransaction(cmd.value.data(), 1U) == m5::hal::error::error_t::OK) &&
           (readWithTransaction(&val, 1) == m5::hal::error::error_t::OK);
}

bool UnitTCS3472x::write_register8(const uint8_t reg, const uint8_t val)
{
    Command cmd{reg, val};
    return writeWithTransaction(cmd.value.data(), cmd.value.size()) == m5::hal::error::error_t::OK;
}

#if 0
bool UnitTCS3472x::read_register16(const uint8_t reg, uint16_t& val)
{
    Command cmd{reg,Command::Type::AutoIncrement};
    uint8_t rbuf[2]{};
    if ((writeWithTransaction(cmd.value.data(), 1U) == m5::hal::error::error_t::OK) &&
        (readWithTransaction(rbuf, 2) == m5::hal::error::error_t::OK)) {
        val = ((uint16_t)rbuf[1] << 8) | rbuf[0];
        return true;
    }
    return false;
}

bool UnitTCS3472x::write_register16(const uint8_t reg, const uint16_t val)
{
    uint8_t buf[2]{};
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    return write_register(reg, buf, 2);
}
#endif

bool UnitTCS3472x::read_register(const uint8_t reg, uint8_t* buf, const uint32_t len)
{
    Command cmd{reg, Command::Type::AutoIncrement};
    return (writeWithTransaction(cmd.value.data(), 1U) == m5::hal::error::error_t::OK) &&
           (readWithTransaction(buf, len) == m5::hal::error::error_t::OK);
}
bool UnitTCS3472x::write_register(const uint8_t reg, const uint8_t* buf, const uint32_t len)
{
    Command cmd{reg, Command::Type::AutoIncrement};
    uint8_t wbuf[len + 1]{};
    wbuf[0] = cmd.value[0];
    std::memcpy(wbuf + 1, buf, len);
    return (writeWithTransaction(wbuf, len + 1) == m5::hal::error::error_t::OK);
}

// class UnitTCS34725
const char UnitTCS34725::name[] = "UnitTCS34725";
const types::uid_t UnitTCS34725::uid{"UnitTCS34725"_mmh3};
const types::uid_t UnitTCS34725::attr{0};

// class UnitTCS34727
const char UnitTCS34727::name[] = "UnitTCS34727";
const types::uid_t UnitTCS34727::uid{"UnitTCS34727"_mmh3};
const types::uid_t UnitTCS34727::attr{0};

}  // namespace unit
}  // namespace m5
