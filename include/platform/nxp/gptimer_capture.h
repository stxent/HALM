/*
 * platform/nxp/gptimer_capture.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_GPTIMER_CAPTURE_H_
#define HALM_PLATFORM_NXP_GPTIMER_CAPTURE_H_
/*----------------------------------------------------------------------------*/
#include <capture.h>
#include <platform/nxp/gptimer_base.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const GpTimerCaptureUnit;
extern const struct CaptureClass * const GpTimerCapture;
/*----------------------------------------------------------------------------*/
struct GpTimerCapture;
/*----------------------------------------------------------------------------*/
struct GpTimerCaptureUnitConfig
{
  /** Mandatory: timer frequency. */
  uint32_t frequency;
  /** Optional: interrupt priority. */
  irqPriority priority;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct GpTimerCaptureUnit
{
  struct GpTimerBase base;

  /* Registered capture handlers */
  struct GpTimerCapture *descriptors[4];
};
/*----------------------------------------------------------------------------*/
struct GpTimerCaptureConfig
{
  /** Mandatory: peripheral unit. */
  struct GpTimerCaptureUnit *parent;
  /** Mandatory: pin used as an input. */
  pinNumber pin;
  /** Mandatory: capture mode. */
  enum pinEvent event;
  /** Optional: enables pull-up or pull-down resistors for input pin. */
  enum pinPull pull;
};
/*----------------------------------------------------------------------------*/
struct GpTimerCapture
{
  struct Capture base;

  void (*callback)(void *);
  void *callbackArgument;

  /* Pointer to a parent unit */
  struct GpTimerCaptureUnit *unit;
  /* Pointer to a capture register */
  const volatile uint32_t *value;
  /* Channel identifier */
  uint8_t channel;
  /* Capture event */
  enum pinEvent event;
};
/*----------------------------------------------------------------------------*/
void *gpTimerCaptureCreate(void *, pinNumber);
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_GPTIMER_CAPTURE_H_ */
