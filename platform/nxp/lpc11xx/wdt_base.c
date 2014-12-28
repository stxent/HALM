/*
 * wdt_base.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <memory.h>
#include <platform/nxp/wdt_base.h>
#include <platform/nxp/lpc11xx/clocking.h>
#include <platform/nxp/lpc11xx/system.h>
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(struct WdtBase *);
/*----------------------------------------------------------------------------*/
static enum result wdtInit(void *, const void *);
static void wdtDeinit(void *);
/*----------------------------------------------------------------------------*/
static const struct EntityClass wdtTable = {
    .size = 0, /* Abstract class */
    .init = wdtInit,
    .deinit = wdtDeinit
};
/*----------------------------------------------------------------------------*/
const struct EntityClass * const WdtBase = &wdtTable;
static struct WdtBase *descriptor = 0;
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(struct WdtBase *timer)
{
  return compareExchangePointer((void **)&descriptor, 0, timer) ? E_OK : E_BUSY;
}
/*----------------------------------------------------------------------------*/
void WDT_ISR(void)
{
  descriptor->handler(descriptor);
}
/*----------------------------------------------------------------------------*/
uint32_t wdtGetClock(const struct WdtBase *timer __attribute__((unused)))
{
  return clockFrequency(WdtClock);
}
/*----------------------------------------------------------------------------*/
static enum result wdtInit(void *object, const void *configBase
    __attribute__((unused)))
{
  struct WdtBase * const timer = object;
  const enum result res = setDescriptor(timer);

  if (res != E_OK)
    return res;

  timer->handler = 0;
  timer->irq = WDT_IRQ;

  sysClockEnable(CLK_WDT);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void wdtDeinit(void *object __attribute__((unused)))
{
  /* Watchdog timer cannot be disabled */
}