/*
 * halm/platform/lpc/sct_timer.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef HALM_PLATFORM_LPC_SCT_TIMER_H_
#define HALM_PLATFORM_LPC_SCT_TIMER_H_
/*----------------------------------------------------------------------------*/
#include <halm/platform/lpc/sct_base.h>
/*----------------------------------------------------------------------------*/
/* Requires SctCounterConfig configuration structure */
extern const struct TimerClass * const SctCounter;
extern const struct TimerClass * const SctUnifiedCounter;

/* Requires SctTimerConfig configuration structure */
extern const struct TimerClass * const SctTimer;
extern const struct TimerClass * const SctUnifiedTimer;

struct SctCounterConfig
{
  /** Mandatory: pin used as an input. */
  PinNumber pin;
  /** Optional: timer interrupt priority. */
  IrqPriority priority;
  /** Optional: edge sensitivity of the selected clock input. */
  enum PinEvent edge;
  /** Mandatory: timer part. */
  enum SctPart part;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};

struct SctTimerConfig
{
  /**
   * Optional: desired timer tick rate. An actual peripheral frequency is used
   * when option is set to zero.
   */
  uint32_t frequency;
  /** Optional: timer interrupt priority. */
  IrqPriority priority;
  /** Mandatory: timer part. */
  enum SctPart part;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};

struct SctTimer
{
  struct SctBase base;

  void (*callback)(void *);
  void *callbackArgument;

  /* Desired timer frequency */
  uint32_t frequency;
  /* Match channel used for counter reset */
  uint8_t event;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_LPC_SCT_TIMER_H_ */
