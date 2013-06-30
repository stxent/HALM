/*
 * lpc13xx_sys.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <LPC13xx.h>
#include "macro.h"
#include "system.h"
/*----------------------------------------------------------------------------*/
uint32_t sysCoreClock = 12000000; //FIXME
/*----------------------------------------------------------------------------*/
/* Changes SYSAHBCLKCTRL register, reset value 0x0000485F */
inline void sysClockEnable(enum sysClockDevice peripheral)
{
  LPC_SYSCON->SYSAHBCLKCTRL |= BIT(peripheral);
}
/*----------------------------------------------------------------------------*/
inline void sysClockDisable(enum sysClockDevice peripheral)
{
  LPC_SYSCON->SYSAHBCLKCTRL &= ~BIT(peripheral);
}
/*----------------------------------------------------------------------------*/
inline void usleep(uint32_t period)
{
  volatile uint32_t count = sysCoreClock / 3000000 * period;

  __asm__ __volatile__(
      "1:\n"
      "   SUBS.W %[count], %[count], #1\n"
      "   BNE 1b\n"
      : [count] "=r"(count)
      : "0" (count)
      : "r3"
  );
}
/*----------------------------------------------------------------------------*/
inline void msleep(uint32_t period)
{
  usleep(1000 * period);
}