/*
 * halm/platform/nxp/wdt_base.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_WDT_BASE_H_
#define HALM_PLATFORM_NXP_WDT_BASE_H_
/*----------------------------------------------------------------------------*/
#include <xcore/entity.h>
#include <halm/irq.h>
/*----------------------------------------------------------------------------*/
#undef HEADER_PATH
#define HEADER_PATH <halm/platform/PLATFORM_TYPE/PLATFORM/wdt_base.h>
#include HEADER_PATH
#undef HEADER_PATH
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const WdtBase;

struct WdtBaseConfig
{
  enum WdtClockSource source;
};

struct WdtBase
{
  struct Entity base;

  void (*handler)(void *);
  IrqNumber irq;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

uint32_t wdtGetClock(const struct WdtBase *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_WDT_BASE_H_ */
