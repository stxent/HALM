/*
 * rtc_base.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/platform/nxp/gen_1/rtc_base.h>
#include <halm/platform/nxp/lpc17xx/system.h>
/*----------------------------------------------------------------------------*/
static void resetInstance(void);
static bool setInstance(struct RtcBase *);
/*----------------------------------------------------------------------------*/
static enum Result clkInit(void *, const void *);

#ifndef CONFIG_PLATFORM_NXP_RTC_NO_DEINIT
static void clkDeinit(void *);
#else
#define clkDeinit deletedDestructorTrap
#endif
/*----------------------------------------------------------------------------*/
static const struct EntityClass clkTable = {
    .size = sizeof(struct RtcBase),
    .init = clkInit,
    .deinit = clkDeinit
};
/*----------------------------------------------------------------------------*/
const struct EntityClass * const RtcBase = &clkTable;
static struct RtcBase *instance = 0;
/*----------------------------------------------------------------------------*/
static void resetInstance(void)
{
  instance = 0;
}
/*----------------------------------------------------------------------------*/
static bool setInstance(struct RtcBase *object)
{
  if (!instance)
  {
    instance = object;
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
void RTC_ISR(void)
{
  instance->handler(instance);
}
/*----------------------------------------------------------------------------*/
static enum Result clkInit(void *object,
    const void *configBase __attribute__((unused)))
{
  struct RtcBase * const clock = object;

  if (setInstance(clock))
  {
    clock->reg = LPC_RTC;
    clock->irq = RTC_IRQ;
    clock->handler = 0;

    if (!sysPowerStatus(PWR_RTC))
      sysPowerEnable(PWR_RTC);

    return E_OK;
  }
  else
    return E_BUSY;
}
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_PLATFORM_NXP_RTC_NO_DEINIT
static void clkDeinit(void *object __attribute__((unused)))
{
  sysPowerDisable(PWR_RTC);
  resetInstance();
}
#endif
