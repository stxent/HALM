/*
 * usb/requests.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef USB_REQUESTS_H_
#define USB_REQUESTS_H_
/*----------------------------------------------------------------------------*/
struct UsbSetupPacket
{
  uint8_t requestType; /* Characteristics of the specific request */
  uint8_t request;     /* Specific request */
  uint16_t value;      /* Request specific parameter */
  uint16_t index;      /* Request specific parameter */
  uint16_t length;     /* Length of data transfered in data phase */
};

#define REQTYPE_GET_DIR(x)    (((x)>>7)&0x01)
#define REQTYPE_GET_TYPE(x)   (((x)>>5)&0x03)
#define REQTYPE_GET_RECIP(x)  ((x)&0x1F)

#define REQTYPE_DIR_TO_DEVICE 0
#define REQTYPE_DIR_TO_HOST   1

#define REQTYPE_TYPE_STANDARD 0
#define REQTYPE_TYPE_CLASS    1
#define REQTYPE_TYPE_VENDOR   2
#define REQTYPE_TYPE_RESERVED 3

#define REQTYPE_RECIP_DEVICE    0
#define REQTYPE_RECIP_INTERFACE 1
#define REQTYPE_RECIP_ENDPOINT  2
#define REQTYPE_RECIP_OTHER     3

/* standard requests */
#define REQ_GET_STATUS        0x00
#define REQ_CLEAR_FEATURE     0x01
#define REQ_SET_FEATURE       0x03
#define REQ_SET_ADDRESS       0x05
#define REQ_GET_DESCRIPTOR    0x06
#define REQ_SET_DESCRIPTOR    0x07
#define REQ_GET_CONFIGURATION 0x08
#define REQ_SET_CONFIGURATION 0x09
#define REQ_GET_INTERFACE     0x0A
#define REQ_SET_INTERFACE     0x0B
#define REQ_SYNCH_FRAME       0x0C

/* class requests HID */
#define HID_GET_REPORT      0x01
#define HID_GET_IDLE        0x02
#define HID_GET_PROTOCOL    0x03
#define HID_SET_REPORT      0x09
#define HID_SET_IDLE        0x0A
#define HID_SET_PROTOCOL    0x0B

/* feature selectors */
#define FEA_ENDPOINT_HALT   0x00
#define FEA_REMOTE_WAKEUP   0x01
#define FEA_TEST_MODE       0x02

struct UsbDescriptorHeader
{
  uint8_t length;
  uint8_t type;
};

#define DESC_DEVICE           1
#define DESC_CONFIGURATION    2
#define DESC_STRING           3
#define DESC_INTERFACE        4
#define DESC_ENDPOINT         5
#define DESC_DEVICE_QUALIFIER 6
#define DESC_OTHER_SPEED      7
#define DESC_INTERFACE_POWER  8

#define DESC_HID_HID        0x21
#define DESC_HID_REPORT     0x22
#define DESC_HID_PHYSICAL   0x23

#define GET_DESC_TYPE(x)    (((x)>>8)&0xFF)
#define GET_DESC_INDEX(x)   ((x)&0xFF)
/*----------------------------------------------------------------------------*/
#endif /* USB_REQUESTS_H_ */
