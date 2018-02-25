/*
 * nvic.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/core/core_defs.h>
#include <halm/core/cortex/armv7m/nvic_defs.h>
#include <halm/core/cortex/nvic.h>
/*----------------------------------------------------------------------------*/
void nvicResetCore(void)
{
  uint32_t value;

  value = SCB->AIRCR & ~AIRCR_VECTKEY_MASK;
  value |= AIRCR_VECTKEY(0x5FA) | AIRCR_SYSRESETREQ;
  SCB->AIRCR = value;
}
