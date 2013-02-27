/*
 * timer_tc.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef TIMER_TC_H_
#define TIMER_TC_H_
/*----------------------------------------------------------------------------*/
#include <LPC13xx.h>
#include "gpio.h"
#include "timer.h"
/*----------------------------------------------------------------------------*/
extern const struct TimerClass *TimerTC;
/*----------------------------------------------------------------------------*/
/* Registers are common for two timer types with different resolutions */
/* To distinguish them all timers/counter have their own symbolic identifiers */
enum tcType {
  TIMER_CT16B0 = 0,
  TIMER_CT16B1,
  TIMER_CT32B0,
  TIMER_CT32B1
};
/*----------------------------------------------------------------------------*/
struct TimerConfig
{
  uint32_t frequency;
  enum tcType channel; /* Peripheral number encoded to enumeration */
};
/*----------------------------------------------------------------------------*/
struct TimerTC
{
  struct Timer parent;

  LPC_TMR_TypeDef *reg; /* Pointer to UART registers */
  IRQn_Type irq; /* IRQ identifier */

  void (*handler)(void *); /* User interrupt handler */
  void *handlerParameters; /* User interrupt handler parameters */
  uint8_t channel; /* Peripheral number for internal use */
};
/*----------------------------------------------------------------------------*/
enum result tcSetDescriptor(enum tcType, void *);
/*----------------------------------------------------------------------------*/
#endif /* TIMER_TC_H_ */
