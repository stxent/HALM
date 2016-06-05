/*
 * platform/nxp/gen_1/can_defs.h
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_GEN_1_CAN_BASE_H_
#define HALM_PLATFORM_NXP_GEN_1_CAN_BASE_H_
/*----------------------------------------------------------------------------*/
#include <xcore/entity.h>
#include <halm/irq.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const CanBase;
/*----------------------------------------------------------------------------*/
struct CanBaseConfig
{
  /** Mandatory: receiver input. */
  pinNumber rx;
  /** Mandatory: transmitter output. */
  pinNumber tx;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct CanBase
{
  struct Entity base;

  void *reg;
  void (*handler)(void *);
  irqNumber irq;

  /* Unique peripheral identifier */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
uint32_t canGetClock(const struct CanBase *);
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_GEN_1_CAN_BASE_H_ */
