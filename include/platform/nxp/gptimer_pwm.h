/*
 * platform/nxp/gptimer_pwm.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef GPTIMER_PWM_H_
#define GPTIMER_PWM_H_
/*----------------------------------------------------------------------------*/
#include <pwm.h>
#include "gptimer_base.h"
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const GpTimerPwmUnit;
extern const struct PwmClass * const GpTimerPwm;
/*----------------------------------------------------------------------------*/
struct GpTimerPwmUnitConfig
{
  /** Mandatory: switching frequency. */
  uint32_t frequency;
  /** Mandatory: cycle resolution. */
  uint32_t resolution;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct GpTimerPwmUnit
{
  struct GpTimerBase parent;

  /* Cycle width measured in timer ticks */
  uint32_t resolution;
  /* Match block used for period setup */
  uint8_t current;
  /* Match blocks currently in use */
  uint8_t matches;
};
/*----------------------------------------------------------------------------*/
struct GpTimerPwmConfig
{
  /** Mandatory: peripheral unit. */
  struct GpTimerPwmUnit *parent;
  /** Optional: initial duration in timer ticks. */
  uint32_t duration;
  /** Mandatory: pin used as an output for modulated signal. */
  gpio_t pin;
};
/*----------------------------------------------------------------------------*/
struct GpTimerPwm
{
  struct Pwm parent;

  /* Pointer to a parent unit */
  struct GpTimerPwmUnit *unit;
  /* Pointer to a match register */
  volatile uint32_t *value;
  /* Match channel number */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
void *gpTimerPwmCreate(void *, gpio_t, uint32_t);
/*----------------------------------------------------------------------------*/
#endif /* GPTIMER_PWM_H_ */
