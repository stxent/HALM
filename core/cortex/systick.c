/*
 * systick.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <bits.h>
#include <core/cortex/systick.h>
/*----------------------------------------------------------------------------*/
/* System Tick counter enable */
#define CTRL_ENABLE                     BIT(0)
/* Interrupt enable */
#define CTRL_TICKINT                    BIT(1)
/* Clock source selection: 0 for external clock, 1 for processor clock */
#define CTRL_CLKSOURCE                  BIT(2)
/* Counter flag is set when counter counts down to zero */
#define CTRL_COUNTFLAG                  BIT(16)
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *);
static enum result setDescriptor(struct SysTickTimer *);
static void updateFrequency(struct SysTickTimer *);
/*----------------------------------------------------------------------------*/
static enum result tmrInit(void *, const void *);
static void tmrDeinit(void *);
static void tmrCallback(void *, void (*)(void *), void *);
static void tmrSetEnabled(void *, bool);
static void tmrSetFrequency(void *, uint32_t);
static void tmrSetOverflow(void *, uint32_t);
static uint32_t tmrValue(void *);
/*----------------------------------------------------------------------------*/
static const struct TimerClass timerTable = {
    .size = sizeof(struct SysTickTimer),
    .init = tmrInit,
    .deinit = tmrDeinit,

    .callback = tmrCallback,
    .setEnabled = tmrSetEnabled,
    .setFrequency = tmrSetFrequency,
    .setOverflow = tmrSetOverflow,
    .value = tmrValue
};
/*----------------------------------------------------------------------------*/
extern uint32_t coreClock; //TODO SysTick: add workaround
const struct TimerClass * const SysTickTimer = &timerTable;
static struct SysTickTimer *descriptor = 0;
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct SysTickTimer * const device = object;

  if ((SYSTICK->CTRL & CTRL_COUNTFLAG) && device->callback)
    device->callback(device->callbackArgument);
}
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(struct SysTickTimer *timer)
{
  //TODO SysTick: replace with compare and swap
  if (descriptor)
    return E_BUSY;

  descriptor = timer;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void updateFrequency(struct SysTickTimer *timer)
{
  const uint32_t state = SYSTICK->CTRL & CTRL_ENABLE;

  SYSTICK->CTRL &= ~CTRL_ENABLE;
  SYSTICK->LOAD = (coreClock / timer->frequency) * timer->overflow - 1;
  SYSTICK->VAL = 0;
  SYSTICK->CTRL |= state;
}
/*----------------------------------------------------------------------------*/
void SYSTICK_ISR(void)
{
  if (descriptor)
    descriptor->handler(descriptor);
}
/*----------------------------------------------------------------------------*/
static enum result tmrInit(void *object, const void *configPtr)
{
  const struct SysTickTimerConfig * const config = configPtr;
  struct SysTickTimer * const timer = object;

  /* Try to set peripheral descriptor */
  if (setDescriptor(timer) != E_OK)
    return E_ERROR;

  timer->handler = interruptHandler;

  timer->frequency = config->frequency ? config->frequency : coreClock;
  timer->overflow = 1;

  SYSTICK->CTRL = 0; /* Stop timer */
  SYSTICK->VAL = 0; /* Reset current timer value */
  SYSTICK->LOAD = coreClock / timer->frequency - 1;
  SYSTICK->CTRL = CTRL_ENABLE | CTRL_CLKSOURCE;

  irqEnable(SYSTICK_IRQ);
  irqSetPriority(SYSTICK_IRQ, config->priority);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void tmrDeinit(void *object __attribute__((unused)))
{
  irqDisable(SYSTICK_IRQ);
  SYSTICK->CTRL &= ~CTRL_ENABLE;
  setDescriptor(0);
}
/*----------------------------------------------------------------------------*/
static void tmrCallback(void *object, void (*callback)(void *), void *argument)
{
  struct SysTickTimer * const timer = object;

  timer->callback = callback;
  timer->callbackArgument = argument;

  if (callback)
  {
    (void)SYSTICK->CTRL; /* Clear pending interrupt */
    SYSTICK->CTRL |= CTRL_TICKINT;
  }
  else
    SYSTICK->CTRL &= ~CTRL_TICKINT;
}
/*----------------------------------------------------------------------------*/
static void tmrSetEnabled(void *object __attribute__((unused)), bool state)
{
  if (state)
    SYSTICK->CTRL |= CTRL_ENABLE;
  else
    SYSTICK->CTRL &= ~CTRL_ENABLE;
}
/*----------------------------------------------------------------------------*/
static void tmrSetFrequency(void *object, uint32_t frequency)
{
  struct SysTickTimer * const timer = object;

  timer->frequency = frequency ? frequency : coreClock;
  updateFrequency(timer);
}
/*----------------------------------------------------------------------------*/
static void tmrSetOverflow(void *object, uint32_t overflow)
{
  struct SysTickTimer * const timer = object;

  timer->overflow = overflow;
  updateFrequency(timer);
}
/*----------------------------------------------------------------------------*/
static uint32_t tmrValue(void *object __attribute__((unused)))
{
  return SYSTICK->VAL;
}

