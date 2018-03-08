/*
 * halm/platform/nxp/gppwm.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_GPPWM_H_
#define HALM_PLATFORM_NXP_GPPWM_H_
/*----------------------------------------------------------------------------*/
#include <halm/platform/nxp/gppwm_base.h>
#include <halm/pwm.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const GpPwmUnit;

struct GpPwmUnitConfig
{
  /** Mandatory: switching frequency. */
  uint32_t frequency;
  /** Mandatory: cycle resolution. */
  uint32_t resolution;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};

struct GpPwmUnit
{
  struct GpPwmUnitBase base;

  /* Desired timer frequency */
  uint32_t frequency;
  /* Cycle width measured in timer ticks */
  uint32_t resolution;
  /* Match blocks currently in use */
  uint8_t matches;
};
/*----------------------------------------------------------------------------*/
extern const struct PwmClass * const GpPwm;

struct GpPwmConfig
{
  /** Mandatory: peripheral unit. */
  struct GpPwmUnit *parent;
  /** Mandatory: pin used as an output for modulated signal. */
  PinNumber pin;
};

struct GpPwm
{
  struct Pwm base;

  /* Pointer to a parent unit */
  struct GpPwmUnit *unit;
  /* Pointer to a match register */
  volatile uint32_t *value;
  /* Match channel number */
  uint8_t channel;
  /* Mask for the Latch Enable Register */
  uint8_t latch;
};
/*----------------------------------------------------------------------------*/
extern const struct PwmClass * const GpPwmDoubleEdge;

struct GpPwmDoubleEdgeConfig
{
  /** Mandatory: peripheral unit. */
  struct GpPwmUnit *parent;
  /** Mandatory: pin used as an output for modulated signal. */
  PinNumber pin;
};

struct GpPwmDoubleEdge
{
  struct Pwm base;

  /* Pointer to a parent unit */
  struct GpPwmUnit *unit;
  /* Pointer to a match register containing leading edge time */
  volatile uint32_t *leading;
  /* Pointer to a match register containing trailing edge time */
  volatile uint32_t *trailing;
  /* Number of the main match channel */
  uint8_t channel;
  /* Mask for the Latch Enable Register */
  uint8_t latch;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

void *gpPwmCreate(void *, PinNumber);
void *gpPwmCreateDoubleEdge(void *, PinNumber);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_GPPWM_H_ */
