/*
 * platform/nxp/sct_pwm.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_SCT_PWM_H_
#define HALM_PLATFORM_NXP_SCT_PWM_H_
/*----------------------------------------------------------------------------*/
#include <platform/nxp/sct_base.h>
#include <pwm.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const SctPwmUnit;
extern const struct PwmClass * const SctPwm;
extern const struct PwmClass * const SctPwmDoubleEdge;
/*----------------------------------------------------------------------------*/
struct SctPwmUnitConfig
{
  /** Mandatory: switching frequency. */
  uint32_t frequency;
  /** Mandatory: cycle resolution. */
  uint32_t resolution;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
  /** Optional: timer part. */
  enum sctPart part;
};
/*----------------------------------------------------------------------------*/
struct SctPwmUnit
{
  struct SctBase base;

  /* Timer frequency */
  uint32_t frequency;
  /* Cycle width measured in timer ticks */
  uint32_t resolution;
  /* Match channel used for counter reset */
  uint8_t event;
};
/*----------------------------------------------------------------------------*/
struct SctPwmConfig
{
  /** Mandatory: peripheral unit. */
  struct SctPwmUnit *parent;
  /** Mandatory: pin used as an output for modulated signal. */
  pinNumber pin;
};
/*----------------------------------------------------------------------------*/
struct SctPwm
{
  struct Pwm base;

  /* Pointer to a parent unit */
  struct SctPwmUnit *unit;
  /* Output channel number */
  uint8_t channel;
  /* Event number */
  uint8_t event;
};
/*----------------------------------------------------------------------------*/
struct SctPwmDoubleEdgeConfig
{
  /** Mandatory: peripheral unit. */
  struct SctPwmUnit *parent;
  /** Mandatory: pin used as an output for modulated signal. */
  pinNumber pin;
};
/*----------------------------------------------------------------------------*/
struct SctPwmDoubleEdge
{
  struct Pwm base;

  /* Pointer to a parent unit */
  struct SctPwmUnit *unit;
  /* Output channel number */
  uint8_t channel;
  /* Event number for leading edge */
  uint8_t leadingEvent;
  /* Event number for trailing edge */
  uint8_t trailingEvent;
};
/*----------------------------------------------------------------------------*/
void *sctPwmCreate(void *, pinNumber);
void *sctPwmCreateDoubleEdge(void *, pinNumber);
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_SCT_PWM_H_ */
