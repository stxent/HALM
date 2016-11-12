/*
 * flash.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <halm/platform/nxp/flash.h>
#include <halm/platform/nxp/iap.h>
#include <halm/platform/nxp/lpc43xx/flash_defs.h>
/*----------------------------------------------------------------------------*/
static bool isAddressValid(const struct Flash *, uint32_t);
static bool isPageAddressValid(const struct Flash *, uint32_t);
static bool isSectorAddressValid(const struct Flash *, uint32_t);
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
static bool isAddressValid(const struct Flash *interface, uint32_t address)
{
  const uint32_t localAddress = address & FLASH_BANK_MASK;
  const uint32_t sizeBankA = FLASH_SIZE_DECODE_A(interface->size);
  const uint32_t sizeBankB = FLASH_SIZE_DECODE_B(interface->size);
  const uint8_t bank = addressToBank(address);

  if (!bank)
    return address >= FLASH_BANK_A && localAddress < sizeBankA;
  else
    return address >= FLASH_BANK_B && localAddress < sizeBankB;
}
/*----------------------------------------------------------------------------*/
static bool isPageAddressValid(const struct Flash *interface,
    uint32_t address)
{
  if (!isAddressValid(interface, address))
    return false;
  if (address & (FLASH_PAGE_SIZE - 1))
    return false;

  return true;
}
/*----------------------------------------------------------------------------*/
static bool isSectorAddressValid(const struct Flash *interface,
    uint32_t address)
{
  if (!isAddressValid(interface, address))
    return false;

  const uint32_t localAddress = address & FLASH_BANK_MASK;
  const uint32_t mask = localAddress < FLASH_SECTORS_BORDER ?
      FLASH_SECTOR_SIZE_0 - 1 : FLASH_SECTOR_SIZE_1 - 1;

  return !(localAddress & mask);
}
/*----------------------------------------------------------------------------*/
static enum result flashInit(void *object,
    const void *configBase __attribute__((unused)))
{
  struct Flash * const interface = object;
  const uint32_t id = flashReadId();

  if (!(id & FLASH_AVAILABLE))
    return E_ERROR; /* Flashless part */

  const uint32_t memory = flashReadConfigId() & FLASH_SIZE_MASK;

  switch (memory)
  {
    case FLASH_SIZE_256_256:
      interface->size = FLASH_SIZE_ENCODE(256 * 1024, 256 * 1024);
      break;

    case FLASH_SIZE_384_384:
      interface->size = FLASH_SIZE_ENCODE(384 * 1024, 384 * 1024);
      break;

    case FLASH_SIZE_512_0:
      interface->size = FLASH_SIZE_ENCODE(512 * 1024, 0);
      break;

    case FLASH_SIZE_512_512:
      interface->size = FLASH_SIZE_ENCODE(512 * 1024, 512 * 1024);
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
      *(uint32_t *)data = FLASH_SIZE_DECODE_A(interface->size)
          + FLASH_SIZE_DECODE_B(interface->size);
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
        return E_ADDRESS;
      if (flashBlankCheckSector(address) == E_OK)
        return E_OK;

      flashInitWrite();

      return flashEraseSector(address);
    }

    case IF_FLASH_ERASE_PAGE:
    {
      const uint32_t address = *(const uint32_t *)data;

      if (!isPageAddressValid(interface, address))
        return E_ADDRESS;

      flashInitWrite();

      return flashErasePage(address);
    }

    default:
      break;
  }

  switch (option)
  {
    case IF_POSITION:
    {
      const uint32_t position = *(const uint32_t *)data;

      if (isPageAddressValid(interface, position))
      {
        interface->position = position;
        return E_OK;
      }
      else
        return E_ADDRESS;
    }

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static size_t flashRead(void *object, void *buffer, size_t length)
{
  struct Flash * const interface = object;

  if (!isAddressValid(interface, interface->position))
    return 0;
  if (!isAddressValid(interface, interface->position + length - 1))
    return 0;

  memcpy(buffer, (const void *)interface->position, length);
  return length;
}
/*----------------------------------------------------------------------------*/
static size_t flashWrite(void *object, const void *buffer, size_t length)
{
  struct Flash * const interface = object;

  flashInitWrite();

  if (flashWriteBuffer(interface->position, buffer, length) == E_OK)
    return length;
  else
    return 0;
}
