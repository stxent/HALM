/*
 * usb_base.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <xcore/memory.h>
#include <halm/platform/nxp/lpc43xx/system.h>
#include <halm/platform/nxp/lpc43xx/system_defs.h>
#include <halm/platform/nxp/lpc43xx/usb_base.h>
#include <halm/platform/nxp/lpc43xx/usb_defs.h>
/*----------------------------------------------------------------------------*/
#define ENDPOINT_REQUESTS     CONFIG_PLATFORM_USB_DEVICE_POOL_SIZE
#define USB0_ENDPOINT_NUMBER  12
#define USB1_ENDPOINT_NUMBER  8
/*----------------------------------------------------------------------------*/
static void configPins(struct UsbBase *, const struct UsbBaseConfig *);
static bool setDescriptor(uint8_t, const struct UsbBase *, struct UsbBase *);
/*----------------------------------------------------------------------------*/
static enum Result devInit(void *, const void *);

#ifndef CONFIG_PLATFORM_USB_NO_DEINIT
static void devDeinit(void *);
#else
#define devDeinit deletedDestructorTrap
#endif
/*----------------------------------------------------------------------------*/
static const struct EntityClass devTable = {
    .size = 0, /* Abstract class */
    .init = devInit,
    .deinit = devDeinit
};
/*----------------------------------------------------------------------------*/
const struct PinEntry usbPins[] = {
    {
        .key = PIN(PORT_1, 5), /* USB0_PWR_FAULT */
        .channel = 0,
        .value = 4
    }, {
        .key = PIN(PORT_1, 7), /* USB0_PPWR */
        .channel = 0,
        .value = 4
    }, {
        .key = PIN(PORT_2, 0), /* USB0_PPWR */
        .channel = 0,
        .value = 3
    }, {
        .key = PIN(PORT_2, 1), /* USB0_PWR_FAULT */
        .channel = 0,
        .value = 3
    }, {
        .key = PIN(PORT_2, 3), /* USB0_PPWR */
        .channel = 0,
        .value = 7
    }, {
        .key = PIN(PORT_2, 4), /* USB0_PWR_FAULT */
        .channel = 0,
        .value = 7
    }, {
        .key = PIN(PORT_6, 3), /* USB0_PPWR */
        .channel = 0,
        .value = 1
    }, {
        .key = PIN(PORT_6, 6), /* USB0_PWR_FAULT */
        .channel = 0,
        .value = 3
    }, {
        .key = PIN(PORT_8, 0), /* USB0_PWR_FAULT */
        .channel = 0,
        .value = 1
    }, {
        .key = PIN(PORT_USB, PIN_USB0_DM), /* USB0_DM */
        .channel = 0,
        .value = 0
    }, {
        .key = PIN(PORT_USB, PIN_USB0_DP), /* USB0_DP */
        .channel = 0,
        .value = 0
    }, {
        .key = PIN(PORT_USB, PIN_USB0_ID), /* USB0_ID */
        .channel = 0,
        .value = 0
    }, {
        .key = PIN(PORT_USB, PIN_USB0_VBUS), /* USB0_VBUS */
        .channel = 0,
        .value = 0
    }, {
        .key = PIN(PORT_2, 5), /* USB1_VBUS1 */
        .channel = 1,
        .value = 2
    }, {
        .key = PIN(PORT_9, 5), /* USB1_PPWR */
        .channel = 1,
        .value = 2
    }, {
        .key = PIN(PORT_9, 6), /* USB1_PWR_FAULT */
        .channel = 1,
        .value = 2
    }, {
        .key = PIN(PORT_USB, PIN_USB1_DM), /* USB1_DM */
        .channel = 1,
        .value = 0
    }, {
        .key = PIN(PORT_USB, PIN_USB1_DP), /* USB1_DP */
        .channel = 1,
        .value = 0
    }, {
        .key = 0 /* End of pin function association list */
    }
};
/*----------------------------------------------------------------------------*/
const struct EntityClass * const UsbBase = &devTable;
static struct UsbBase *descriptors[2] = {0};
/*----------------------------------------------------------------------------*/
static void configPins(struct UsbBase *device,
    const struct UsbBaseConfig *config)
{
  const PinNumber pinArray[] = {
      config->dm, config->dp, config->connect, config->vbus
  };

  for (size_t index = 0; index < ARRAY_SIZE(pinArray); ++index)
  {
    if (pinArray[index])
    {
      const struct PinEntry * const pinEntry = pinFind(usbPins, pinArray[index],
          device->channel);
      assert(pinEntry);

      const struct Pin pin = pinInit(pinArray[index]);

      pinInput(pin);
      pinSetFunction(pin, pinEntry->value);
    }
  }
}
/*----------------------------------------------------------------------------*/
static bool setDescriptor(uint8_t channel, const struct UsbBase *state,
    struct UsbBase *device)
{
  assert(channel < ARRAY_SIZE(descriptors));

  return compareExchangePointer((void **)(descriptors + channel), state,
      device);
}
/*----------------------------------------------------------------------------*/
void USB0_ISR(void)
{
  descriptors[0]->handler(descriptors[0]);
}
/*----------------------------------------------------------------------------*/
void USB1_ISR(void)
{
  descriptors[1]->handler(descriptors[1]);
}
/*----------------------------------------------------------------------------*/
static enum Result devInit(void *object, const void *configBase)
{
  const struct UsbBaseConfig * const config = configBase;
  struct UsbBase * const device = object;
  enum Result res;

  device->channel = config->channel;
  device->handler = 0;
  device->queueHeads = 0;

  /* Try to set peripheral descriptor */
  if (!setDescriptor(device->channel, 0, device))
    return E_BUSY;

  res = arrayInit(&device->descriptorPool, sizeof(struct TransferDescriptor *),
      ENDPOINT_REQUESTS);
  if (res != E_OK)
    return res;

  configPins(device, configBase);

  if (!device->channel)
  {
    sysClockEnable(CLK_M4_USB0);
    sysClockEnable(CLK_USB0);
    sysResetEnable(RST_USB0);

    device->irq = USB0_IRQ;
    device->reg = LPC_USB0;
    device->numberOfEndpoints = USB0_ENDPOINT_NUMBER;

    LPC_CREG->CREG0 &= ~CREG0_USB0PHY;
    LPC_USB0->OTGSC = OTGSC_VD | OTGSC_OT;
  }
  else
  {
    sysClockEnable(CLK_M4_USB1);
    sysClockEnable(CLK_USB1);
    sysResetEnable(RST_USB1);

    device->irq = USB1_IRQ;
    device->reg = LPC_USB1;
    device->numberOfEndpoints = USB1_ENDPOINT_NUMBER;

    LPC_SCU->SFSUSB = SFSUSB_ESEA | SFSUSB_EPWR | SFSUSB_VBUS;
  }

  device->queueHeads = memalign(2048, sizeof(struct QueueHead)
      * device->numberOfEndpoints);
  if (!device->queueHeads)
    return E_MEMORY;

  device->descriptorMemory = memalign(32, sizeof(struct TransferDescriptor)
      * ENDPOINT_REQUESTS);
  if (!device->descriptorMemory)
    return E_MEMORY;

  for (size_t index = 0; index < ENDPOINT_REQUESTS; ++index)
  {
    struct TransferDescriptor * const entry = device->descriptorMemory + index;
    arrayPushBack(&device->descriptorPool, &entry);
  }

  return E_OK;
}
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_PLATFORM_USB_NO_DEINIT
static void devDeinit(void *object)
{
  struct UsbBase * const device = object;

  free(device->descriptorMemory);
  free(device->queueHeads);

  if (!device->channel)
  {
    LPC_CREG->CREG0 |= CREG0_USB0PHY;

    sysClockDisable(CLK_USB0);
    sysClockDisable(CLK_M4_USB0);
  }
  else
  {
    LPC_SCU->SFSUSB &= ~(SFSUSB_EPWR | SFSUSB_VBUS);

    sysClockDisable(CLK_USB1);
    sysClockDisable(CLK_M4_USB1);
  }

  setDescriptor(device->channel, device, 0);
}
#endif
