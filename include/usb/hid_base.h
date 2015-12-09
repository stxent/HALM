/*
 * usb/hid_base.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef USB_HID_BASE_H_
#define USB_HID_BASE_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <usb/usb.h>
/*----------------------------------------------------------------------------*/
extern const struct UsbDriverClass * const HidBase;
/*----------------------------------------------------------------------------*/
struct Hid;
struct HidDescriptor;
/*----------------------------------------------------------------------------*/
struct HidBaseConfig
{
  /** Mandatory: pointer to an upper half of the driver. */
  struct Hid *owner;
  /** Mandatory: USB device. */
  void *device;
  /** Mandatory: report descriptor. */
  void *report;
  /** Mandatory: size of the report descriptor. */
  uint16_t reportSize;

  struct
  {
    /** Mandatory: identifier of the notification endpoint. */
    uint8_t interrupt;
  } endpoint;
};
/*----------------------------------------------------------------------------*/
struct HidBase
{
  struct UsbDriver parent;

  struct Hid *owner;
  struct UsbDevice *device;

  struct HidDescriptor *hidDescriptor;
  struct UsbEndpointDescriptor *endpointDescriptor;

  const void *reportDescriptor;
  uint16_t reportDescriptorSize;

  uint8_t idleTime;
};
/*----------------------------------------------------------------------------*/
#endif /* USB_HID_BASE_H_ */
