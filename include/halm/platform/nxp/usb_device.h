/*
 * halm/platform/nxp/usb_device.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_USB_DEVICE_H_
#define HALM_PLATFORM_NXP_USB_DEVICE_H_
/*----------------------------------------------------------------------------*/
#include <halm/irq.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
extern const struct UsbDeviceClass * const UsbDevice;
/*----------------------------------------------------------------------------*/
struct UsbDevice;
/*----------------------------------------------------------------------------*/
struct UsbDeviceConfig
{
  /** Mandatory: USB bidirectional D- line. */
  pinNumber dm;
  /** Mandatory: USB bidirectional D+ line. */
  pinNumber dp;
  /** Mandatory: output pin used for soft connect feature. */
  pinNumber connect;
  /** Mandatory: monitors the presence of USB bus power. */
  pinNumber vbus;
  /** Mandatory: Vendor Identifier. */
  uint16_t vid;
  /** Mandatory: Product Identifier. */
  uint16_t pid;
  /** Optional: interrupt priority. */
  irqPriority priority;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct UsbEndpointConfig
{
  /** Mandatory: hardware device. */
  struct UsbDevice *parent;
  /** Mandatory: logical address of the endpoint. */
  uint8_t address;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_USB_DEVICE_H_ */
