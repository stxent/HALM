/*
 * eeprom_base.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/platform/nxp/gen_1/eeprom_base.h>
#include <halm/platform/nxp/iap.h>
#include <halm/platform/nxp/lpc13uxx/flash_defs.h>
/*----------------------------------------------------------------------------*/
static enum Result eepromInit(void *, const void *);
/*----------------------------------------------------------------------------*/
const struct EntityClass * const EepromBase = &(const struct EntityClass){
    .size = sizeof(struct EepromBase),
    .init = eepromInit,
    .deinit = 0 /* Default destructor */
};
/*----------------------------------------------------------------------------*/
static enum Result eepromInit(void *object,
    const void *configBase __attribute__((unused)))
{
  struct EepromBase * const interface = object;
  const uint32_t id = flashReadId();

  switch (id)
  {
    case CODE_LPC1315:
    case CODE_LPC1345:
      interface->size = 2 * 1024 - 64;
      break;

    case CODE_LPC1316:
    case CODE_LPC1317:
    case CODE_LPC1346:
    case CODE_LPC1347:
      interface->size = 4 * 1024 - 64;
      break;

    default:
      return E_ERROR;
  }

  return E_OK;
}