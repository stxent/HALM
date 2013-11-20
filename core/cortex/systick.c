/*
 * systick.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <irq.h>
#include <macro.h>
#include <core/cortex/systick.h>
/*----------------------------------------------------------------------------*/
#define CTRL_ENABLE                     BIT(0) /* System tick timer enable */
#define CTRL_TICKINT                    BIT(1) /* Interrupt enable */
/* Clock source selection: 0 for clock divider, 1 for system clock source */
#define CTRL_CLKSOURCE                  BIT(2)
#define CTRL_COUNTFLAG                  BIT(16) /* Set when counter reaches 0 */
/*----------------------------------------------------------------------------*/
#define DEFAULT_PRIORITY 255 /* Lowest interrupt priority in Cortex-M3 */
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *);
static enum result setDescriptor(struct SysTickTimer *);
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
const struct TimerClass *SysTickTimer = &timerTable;
/*----------------------------------------------------------------------------*/
static struct SysTickTimer *descriptor = 0;
/*----------------------------------------------------------------------------*/
extern uint32_t sysCoreClock;
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct SysTickTimer *device = object;

  if ((SYSTICK->CTRL & CTRL_COUNTFLAG) && device->callback)
    device->callback(device->callbackArgument);
}
/*----------------------------------------------------------------------------*/
enum result setDescriptor(struct SysTickTimer *device)
{
  if (descriptor)
    return E_BUSY;

  descriptor = device;
  return E_OK;
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
  /* Set pointer to device configuration data */
  const struct SysTickTimerConfig * const config = configPtr;
  struct SysTickTimer *device = object;

  assert(config->frequency);

  /* Try to set peripheral descriptor */
  if (setDescriptor(device) != E_OK)
    return E_ERROR;

  /* Set timer interrupt handler to default handler */
  device->handler = interruptHandler;

  device->frequency = config->frequency;
  device->overflow = 1; //FIXME

  SYSTICK->CTRL = 0; /* Stop timer */
  SYSTICK->VAL = 0; /* Reset current timer value */
  SYSTICK->LOAD = sysCoreClock / device->frequency - 1;
  SYSTICK->CTRL = CTRL_ENABLE | CTRL_CLKSOURCE;

  /* Enable interrupt */
  irqEnable(SYSTICK_IRQ);
  /* Set interrupt priority, lowest by default */
  irqSetPriority(SYSTICK_IRQ, DEFAULT_PRIORITY);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void tmrDeinit(void *object __attribute__((unused)))
{
  /* Disable interrupt */
  irqDisable(SYSTICK_IRQ);
  /* Disable timer */
  SYSTICK->CTRL &= ~CTRL_ENABLE;
  /* Reset descriptor */
  setDescriptor(0);
}
/*----------------------------------------------------------------------------*/
static void tmrCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct SysTickTimer *device = object;

  device->callback = callback;
  device->callbackArgument = argument;

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
  struct SysTickTimer *device = object;

  device->frequency = frequency;

  SYSTICK->CTRL &= ~CTRL_ENABLE;
  /* FIXME overflow + 1? */
  SYSTICK->LOAD = (sysCoreClock / device->frequency) * device->overflow - 1;
  SYSTICK->VAL = 0;
  SYSTICK->CTRL |= CTRL_ENABLE;
}
/*----------------------------------------------------------------------------*/
static void tmrSetOverflow(void *object, uint32_t overflow)
{
  struct SysTickTimer *device = object;

  device->overflow = overflow;

  SYSTICK->CTRL &= ~CTRL_ENABLE;
  /* FIXME overflow + 1? */
  SYSTICK->LOAD = (sysCoreClock / device->frequency) * device->overflow - 1;
  SYSTICK->VAL = 0;
  SYSTICK->CTRL |= CTRL_ENABLE;
}
/*----------------------------------------------------------------------------*/
static uint32_t tmrValue(void *object __attribute__((unused)))
{
  return SYSTICK->VAL;
}

