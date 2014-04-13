/*
 * platform/nxp/gppwm.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef GPPWM_H_
#define GPPWM_H_
/*----------------------------------------------------------------------------*/
#include <gpio.h>
#include <pwm.h>
#include "gppwm_base.h"
/*----------------------------------------------------------------------------*/
extern const struct EntityClass *GpPwmUnit;
extern const struct PwmClass *GpPwm;
/*----------------------------------------------------------------------------*/
struct GpPwmUnitConfig
{
  uint32_t frequency; /* Mandatory: cycle frequency */
  uint32_t resolution; /* Mandatory: cycle resolution */
  uint8_t channel; /* Mandatory: timer block */
};
/*----------------------------------------------------------------------------*/
struct GpPwmUnit
{
  struct GpPwmUnitBase parent;

  uint32_t resolution;
  uint8_t matches; /* Match blocks currently in use */
};
/*----------------------------------------------------------------------------*/
struct GpPwmConfig
{
  /** Mandatory: peripheral unit. */
  struct GpPwmUnit *parent;
  /** Optional: initial duration in timer ticks. */
  uint32_t duration;
  /** Mandatory: pin used as an output for modulated signal. */
  gpio_t pin;
};
/*----------------------------------------------------------------------------*/
struct GpPwm
{
  struct Pwm parent;

  /* Pointer to a parent unit */
  struct GpPwmUnit *unit;
  /* Pointer to a match register */
  uint32_t *value;
  /* Match channel number */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct GpPwmDoubleEdgeConfig
{
  /** Mandatory: peripheral unit. */
  struct GpPwmUnit *parent;
  /** Optional: initial leading edge time in timer ticks. */
  uint32_t leading;
  /** Optional: initial trailing edge time in timer ticks. */
  uint32_t trailing;
  /** Mandatory: pin used as an output for modulated signal. */
  gpio_t pin;
};
/*----------------------------------------------------------------------------*/
struct GpPwmDoubleEdge
{
  struct Pwm parent;

  /* Pointer to a parent unit */
  struct GpPwmUnit *unit;
  /* Pointer to a match register containing leading edge time */
  uint32_t *leading;
  /* Pointer to a match register containing trailing edge time */
  uint32_t *trailing;
  /* Number of the main match channel */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
void *gpPwmCreate(void *, gpio_t, uint32_t);
void *gpPwmCreateDoubleEdge(void *, gpio_t, uint32_t, uint32_t);
/*----------------------------------------------------------------------------*/
#endif /* GPPWM_H_ */
