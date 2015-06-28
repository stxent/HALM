/*
 * platform/nxp/lpc17xx/usb_base.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef PLATFORM_NXP_LPC17XX_USB_BASE_H_
#define PLATFORM_NXP_LPC17XX_USB_BASE_H_
/*----------------------------------------------------------------------------*/
#include <interface.h>
#include <irq.h>
#include <pin.h>
#include <containers/list.h>
#include <containers/queue.h>
#include <usb/usb.h>
/*----------------------------------------------------------------------------*/
extern const struct UsbDeviceClass * const UsbDeviceBase;
/*----------------------------------------------------------------------------*/
struct UsbDeviceBaseConfig
{
  /** Mandatory: USB bidirectional D- line. */
  pin_t dm;
  /** Mandatory: USB bidirectional D+ line. */
  pin_t dp;
  /** Mandatory: output pin used for soft connect feature. */
  pin_t connect;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct UsbDeviceBase
{
  struct Entity parent;

  void *reg;
  void (*handler)(void *);
  irq_t irq;

  /* Unique peripheral identifier */
  uint8_t channel;

  /* List of registered endpoints */
  struct List endpoints;
};
/*----------------------------------------------------------------------------*/
extern const struct UsbEndpointClass * const UsbEndpoint;
/*----------------------------------------------------------------------------*/
struct UsbEndpointConfig
{
  /** Mandatory: hardware device. */
  struct UsbDeviceBase *parent;
  /** Mandatory: maximum packet size. */
  uint16_t size;
  /** Mandatory: logical address of the endpoint. */
  uint8_t address;
};
/*----------------------------------------------------------------------------*/
struct UsbEndpoint
{
  struct Entity parent;

  /* Parent device */
  struct UsbDeviceBase *device;
  /* Pending requests */
  struct Queue requests;
  /* Maximum packet size */
  uint16_t size;
  /* Logical address */
  uint8_t address;
  /* Busy flag */
  bool busy; // TODO Replace with spinlock
};
/*----------------------------------------------------------------------------*/
#endif /* PLATFORM_NXP_LPC17XX_USB_BASE_H_ */
