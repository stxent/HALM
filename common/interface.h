/*
 * interface.h
 *
 *  Created on: Oct 20, 2012
 *      Author: xen
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_
/*------------------------------------------------------------------------------*/
#include <stdint.h>
/*------------------------------------------------------------------------------*/
#include "entity.h"
#include "mutex.h"
/*------------------------------------------------------------------------------*/
enum ifOption {
  IF_SPEED = 0,
  IF_BUFFER_SIZE,
  IF_SYNC,
  IF_QUEUE_RX,
  IF_QUEUE_TX
};
/*----------------------------------------------------------------------------*/
struct Interface;
/*----------------------------------------------------------------------------*/
/* Class descriptor */
struct InterfaceClass
{
  struct EntityClass entity;
  /* Start transmission, arguments: device address */
  enum result (*start)(struct Interface *, uint8_t *);
  /* Stop transmission */
  void (*stop)(struct Interface *);
  /* Receive data, arguments: data buffer, message size */
  unsigned int (*read)(struct Interface *, uint8_t *, unsigned int);
  /* Send data, arguments: data buffer, message size */
  unsigned int (*write)(struct Interface *, const uint8_t *, unsigned int);
  /* Get interface option, arguments: option id, pointer to save value */
  enum result (*getopt)(struct Interface *, enum ifOption, void *);
  /* Set interface option, arguments: option id, pointer to new value */
  enum result (*setopt)(struct Interface *, enum ifOption, const void *);
};
/*----------------------------------------------------------------------------*/
extern const struct EntityClass *Interface;
/*----------------------------------------------------------------------------*/
struct Interface
{
  struct Entity parent;
  struct Mutex lock;
};
/*----------------------------------------------------------------------------*/
enum result ifStart(struct Interface *, uint8_t *);
void ifStop(struct Interface *);
unsigned int ifRead(struct Interface *, uint8_t *, unsigned int);
unsigned int ifWrite(struct Interface *, const uint8_t *, unsigned int);
enum result ifGetOpt(struct Interface *, enum ifOption, void *);
enum result ifSetOpt(struct Interface *, enum ifOption, const void *);
/*----------------------------------------------------------------------------*/
#endif /* INTERFACE_H_ */
