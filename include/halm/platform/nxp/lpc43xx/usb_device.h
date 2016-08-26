/*
 * halm/platform/nxp/lpc43xx/usb_device.h
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_LPC43XX_USB_DEVICE_H_
#define HALM_PLATFORM_NXP_LPC43XX_USB_DEVICE_H_
/*----------------------------------------------------------------------------*/
#include <halm/usb/usb.h>
#include <halm/usb/usb_control.h>
/*----------------------------------------------------------------------------*/
#undef HEADER_PATH
#define HEADER_PATH <halm/platform/PLATFORM_TYPE/PLATFORM/usb_base.h>
#include HEADER_PATH
#undef HEADER_PATH
/*----------------------------------------------------------------------------*/
struct UsbEndpoint;
/*----------------------------------------------------------------------------*/
struct UsbDevice
{
  struct UsbBase base;

  /* Array of logical endpoints */
  struct UsbEndpoint **endpoints;
  /* Control message handler */
  struct UsbControl *control;
  /* Device status */
  uint8_t status;
  /* Device is suspended */
  bool suspended;
};
/*----------------------------------------------------------------------------*/
struct UsbEndpoint
{
  struct Entity base;

  /* Parent device */
  struct UsbDevice *device;
  /* Logical address */
  uint8_t address;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_LPC43XX_USB_DEVICE_H_ */
