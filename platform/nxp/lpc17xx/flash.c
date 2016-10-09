/*
 * flash.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <halm/platform/nxp/flash.h>
#include <halm/platform/nxp/lpc17xx/flash_defs.h>
/*----------------------------------------------------------------------------*/
static inline bool isPageAddressValid(const struct Flash *, uint32_t);
static inline bool isSectorAddressValid(const struct Flash *, uint32_t);
/*----------------------------------------------------------------------------*/
static enum result flashInit(void *, const void *);
static void flashDeinit(void *);
static enum result flashCallback(void *, void (*)(void *), void *);
static enum result flashGet(void *, enum ifOption, void *);
static enum result flashSet(void *, enum ifOption, const void *);
static size_t flashRead(void *, void *, size_t);
static size_t flashWrite(void *, const void *, size_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass flashTable = {
    .size = sizeof(struct Flash),
    .init = flashInit,
    .deinit = flashDeinit,

    .callback = flashCallback,
    .get = flashGet,
    .set = flashSet,
    .read = flashRead,
    .write = flashWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const Flash = &flashTable;
/*----------------------------------------------------------------------------*/
static inline bool isPageAddressValid(const struct Flash *interface,
    uint32_t address)
{
  return !(address & (FLASH_PAGE_SIZE - 1)) && address < interface->size;
}
/*----------------------------------------------------------------------------*/
static inline bool isSectorAddressValid(const struct Flash *interface,
    uint32_t address)
{
  if (address >= interface->size)
    return false;

  if (address < FLASH_SECTORS_BORDER)
    return !(address & (FLASH_SECTOR_SIZE_0 - 1));
  else
    return !(address & (FLASH_SECTOR_SIZE_1 - 1));
}
/*----------------------------------------------------------------------------*/
static enum result flashInit(void *object,
    const void *configBase __attribute__((unused)))
{
  struct Flash * const interface = object;
  const uint32_t id = flashReadId();

  switch (id)
  {
    case CODE_LPC1751_00:
    case CODE_LPC1751:
      interface->size = 32 * 1024;
      break;

    case CODE_LPC1752:
      interface->size = 64 * 1024;
      break;

    case CODE_LPC1754:
    case CODE_LPC1764:
      interface->size = 128 * 1024;
      break;

    case CODE_LPC1756:
    case CODE_LPC1763:
    case CODE_LPC1765:
    case CODE_LPC1766:
      interface->size = 256 * 1024;
      break;

    case CODE_LPC1758:
    case CODE_LPC1759:
    case CODE_LPC1767:
    case CODE_LPC1768:
    case CODE_LPC1769:
      interface->size = 512 * 1024;
      break;

    default:
      return E_ERROR;
  }

  interface->position = 0;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void flashDeinit(void *object __attribute__((unused)))
{

}
/*----------------------------------------------------------------------------*/
static enum result flashCallback(void *object __attribute__((unused)),
    void (*callback)(void *) __attribute__((unused)),
    void *argument __attribute__((unused)))
{
  return E_INVALID;
}
/*----------------------------------------------------------------------------*/
static enum result flashGet(void *object, enum ifOption option, void *data)
{
  struct Flash * const interface = object;

  switch (option)
  {
    case IF_POSITION:
      *(uint32_t *)data = interface->position;
      return E_OK;

    case IF_SIZE:
      *(uint32_t *)data = interface->size;
      return E_OK;

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static enum result flashSet(void *object, enum ifOption option,
    const void *data)
{
  struct Flash * const interface = object;

  /* Additional Flash options */
  switch ((enum flashOption)option)
  {
    case IF_FLASH_ERASE_SECTOR:
    {
      const uint32_t address = *(const uint32_t *)data;

      if (!isSectorAddressValid(interface, address))
        return E_VALUE;

      if (flashBlankCheckSector(address) == E_OK)
        return E_OK;

      return flashEraseSector(address);
    }

    default:
      break;
  }

  switch (option)
  {
    case IF_POSITION:
    {
      const uint32_t position = *(const uint32_t *)data;

      if (!isPageAddressValid(interface, position))
        return E_VALUE;

      interface->position = position;
      return E_OK;
    }

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static size_t flashRead(void *object, void *buffer, size_t length)
{
  struct Flash * const interface = object;

  if (!isPageAddressValid(interface, interface->position))
    return 0;
  if (length & (FLASH_PAGE_SIZE - 1))
    return 0;

  memcpy(buffer, (const void *)interface->position, length);

  return length;
}
/*----------------------------------------------------------------------------*/
static size_t flashWrite(void *object, const void *buffer, size_t length)
{
  struct Flash * const interface = object;

  if (!isPageAddressValid(interface, interface->position))
    return 0;
  if (length & (FLASH_PAGE_SIZE - 1))
    return 0;

  if (flashWriteBuffer(interface->position, buffer, length) != E_OK)
    return 0;

  return length;
}
