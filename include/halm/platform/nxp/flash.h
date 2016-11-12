/*
 * halm/platform/nxp/flash.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_FLASH_H_
#define HALM_PLATFORM_NXP_FLASH_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <xcore/interface.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const Flash;
/*----------------------------------------------------------------------------*/
enum flashOption
{
  /** Erase sector. */
  IF_FLASH_ERASE_SECTOR = IF_OPTION_END,
  /** Erase page in sector. */
  IF_FLASH_ERASE_PAGE
};
/*----------------------------------------------------------------------------*/
struct Flash
{
  struct Interface base;

  /* Current address */
  uint32_t position;
  /* Size of the Flash memory */
  uint32_t size;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_FLASH_H_ */
