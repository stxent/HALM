/*
 * halm/platform/nxp/adc_bus.h
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_ADC_BUS_H_
#define HALM_PLATFORM_NXP_ADC_BUS_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include <xcore/interface.h>
#include <halm/target.h>
/*----------------------------------------------------------------------------*/
#undef HEADER_PATH
#define HEADER_PATH <halm/platform/PLATFORM_TYPE/GEN_ADC/adc_unit.h>
#include HEADER_PATH
#undef HEADER_PATH
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const AdcBus;
/*----------------------------------------------------------------------------*/
struct AdcBusConfig
{
  /** Mandatory: peripheral unit. */
  struct AdcUnit *parent;
  /** Mandatory: pointer to an array of pins terminated with zero element. */
  const PinNumber *pins;
};
/*----------------------------------------------------------------------------*/
struct AdcBus
{
  struct Entity parent;

  /* Pointer to a parent unit */
  struct AdcUnit *unit;

  void (*callback)(void *);
  void *callbackArgument;

  /* Pointer to an input buffer */
  void *buffer;

  /* Pin descriptors */
  struct AdcPin *pins;

  /* Event channel */
  uint8_t event;
  /* Channel mask */
  uint8_t mask;
  /* Number of pins */
  uint8_t number;
  /* Selection between blocking mode and zero copy mode */
  bool blocking;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_ADC_BUS_H_ */
