/*
 * halm/platform/lpc/lpc11exx/pin.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef HALM_PLATFORM_LPC_PIN_H_
#error This header should not be included directly
#endif

#ifndef HALM_PLATFORM_LPC_LPC11EXX_PIN_H_
#define HALM_PLATFORM_LPC_LPC11EXX_PIN_H_
/*----------------------------------------------------------------------------*/
#include <halm/platform/platform_defs.h>
/*----------------------------------------------------------------------------*/
enum
{
  PORT_0,
  PORT_1
};
/*----------------------------------------------------------------------------*/
#undef HEADER_PATH
#define HEADER_PATH <halm/platform/PLATFORM_TYPE/GEN_PIN/pin.h>
#include HEADER_PATH
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_LPC_LPC11EXX_PIN_H_ */
