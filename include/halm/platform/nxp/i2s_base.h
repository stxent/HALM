/*
 * halm/platform/nxp/i2s_base.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_I2S_BASE_H_
#define HALM_PLATFORM_NXP_I2S_BASE_H_
/*----------------------------------------------------------------------------*/
#include <xcore/interface.h>
#include <halm/irq.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const I2sBase;
/*----------------------------------------------------------------------------*/
enum i2sWidth
{
  I2S_WIDTH_8,
  I2S_WIDTH_16,
  I2S_WIDTH_32
};
/*----------------------------------------------------------------------------*/
struct I2sRateConfig
{
  uint8_t x;
  uint8_t y;
};
/*----------------------------------------------------------------------------*/
struct I2sBaseConfig
{
  struct
  {
    /** Optional: receive clock. */
    PinNumber sck;
    /** Optional: receive word select. */
    PinNumber ws;
    /** Optional: receive data. */
    PinNumber sda;
    /** Optional: master clock output. */
    PinNumber mclk;
  } rx;

  struct
  {
    /** Optional: transmit clock. */
    PinNumber sck;
    /** Optional: transmit word select. */
    PinNumber ws;
    /** Optional: transmit data. */
    PinNumber sda;
    /** Optional: master clock output. */
    PinNumber mclk;
  } tx;

  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct I2sBase
{
  struct Interface base;

  void *reg;
  void (*handler)(void *);
  IrqNumber irq;

  /* Unique peripheral identifier */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
enum result i2sCalcRate(struct I2sBase *, uint32_t, struct I2sRateConfig *);
uint32_t i2sGetClock(const struct I2sBase *);
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_I2S_BASE_H_ */
