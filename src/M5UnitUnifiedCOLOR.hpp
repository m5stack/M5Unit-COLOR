/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file M5UnitUnifiedCOLOR.hpp
  @brief Main header of M5UnitCOLOR using M5UnitUnified

  @mainpage M5Unit-COLOR
  Library for UnitCOLOR using M5UnitUnified.
*/
#ifndef M5_UNIT_UNIFIED_COLOR_HPP
#define M5_UNIT_UNIFIED_COLOR_HPP

#include "unit/unit_TCS3472x.hpp"
#include "utility/unit_color_utility.hpp"

/*!
  @namespace m5
  @brief Top level namespace of M5stack
 */
namespace m5 {

/*!
  @namespace unit
  @brief Unit-related namespace
 */
namespace unit {
using UnitColor = m5::unit::UnitTCS34725;
}  // namespace unit
}  // namespace m5
#endif
