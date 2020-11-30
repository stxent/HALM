/*
 * halm/platform/nxp/lpc13xx/pin.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_LPC13XX_PIN_H_
#define HALM_PLATFORM_NXP_LPC13XX_PIN_H_
/*----------------------------------------------------------------------------*/
enum
{
  PORT_0,
  PORT_1,
  PORT_2,
  PORT_3,
  PORT_USB
};

enum
{
  PIN_USB_DM,
  PIN_USB_DP
};
/*----------------------------------------------------------------------------*/
#undef HEADER_PATH
#define HEADER_PATH <halm/platform/PLATFORM_TYPE/GEN_PIN/pin.h>
#include HEADER_PATH
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_LPC13XX_PIN_H_ */
