/*
 * platform/nxp/lpc43xx/usb_device.h
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_LPC43XX_USB_DEVICE_H_
#define HALM_PLATFORM_NXP_LPC43XX_USB_DEVICE_H_
/*----------------------------------------------------------------------------*/
#include <containers/list.h>
#include <usb/usb.h>
#include <usb/usb_control.h>
/*----------------------------------------------------------------------------*/
#undef HEADER_PATH
#define HEADER_PATH <platform/PLATFORM_TYPE/PLATFORM/usb_base.h>
#include HEADER_PATH
#undef HEADER_PATH
/*----------------------------------------------------------------------------*/
struct UsbEndpoint;
/*----------------------------------------------------------------------------*/
struct UsbDevice
{
  struct UsbBase base;

  /* List of registered endpoints */
  struct List endpoints;
  /* Control OUT endpoint */
  struct UsbEndpoint *ep0out;
  /* Control message handler */
  struct UsbControl *control;
  /* Active device configuration */
  uint8_t configuration;
  /* Suspend flag */
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