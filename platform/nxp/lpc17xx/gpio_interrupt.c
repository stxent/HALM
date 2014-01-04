/*
 * gpio_interrupt.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <irq.h>
#include <macro.h>
#include <platform/nxp/gpio_interrupt.h>
#include <platform/nxp/lpc17xx/gpio_defs.h>
/*----------------------------------------------------------------------------*/
static void disableInterrupt(union GpioPin);
static void enableInterrupt(union GpioPin, enum gpioIntMode);
static void processInterrupt(uint8_t);
static enum result resetDescriptor(union GpioPin);
static enum result setDescriptor(union GpioPin, struct GpioInterrupt *);
/*----------------------------------------------------------------------------*/
static enum result gpioIntInit(void *, const void *);
static void gpioIntDeinit(void *);
static void gpioIntCallback(void *, void (*)(void *), void *);
static void gpioIntSetEnabled(void *, bool);
/*----------------------------------------------------------------------------*/
static const struct InterruptClass gpioIntTable = {
    .size = sizeof(struct GpioInterrupt),
    .init = gpioIntInit,
    .deinit = gpioIntDeinit,

    .callback = gpioIntCallback,
    .setEnabled = gpioIntSetEnabled
};
/*----------------------------------------------------------------------------*/
const struct InterruptClass *GpioInterrupt = &gpioIntTable;
static struct GpioInterrupt *descriptors[2] = {0};
/*----------------------------------------------------------------------------*/
static void disableInterrupt(union GpioPin pin)
{
  const uint32_t mask = 1 << pin.offset;

  switch (pin.port)
  {
    case 0:
      LPC_GPIOINT->ENF0 &= ~mask;
      LPC_GPIOINT->ENR0 &= ~mask;
      break;
    case 2:
      LPC_GPIOINT->ENF2 &= ~mask;
      LPC_GPIOINT->ENR2 &= ~mask;
      break;
  }
}
/*----------------------------------------------------------------------------*/
static void enableInterrupt(union GpioPin pin, enum gpioIntMode mode)
{
  const uint32_t mask = 1 << pin.offset;

  switch (pin.port)
  {
    case 0:
      /* Clear pending interrupt flag */
      LPC_GPIOINT->CLR0 = mask;
      /* Configure edge sensitivity options */
      if (mode != GPIO_RISING)
        LPC_GPIOINT->ENF0 |= mask;
      if (mode != GPIO_FALLING)
        LPC_GPIOINT->ENR0 |= mask;
      break;
    case 2:
      LPC_GPIOINT->CLR2 = mask;
      if (mode != GPIO_RISING)
        LPC_GPIOINT->ENF2 |= mask;
      if (mode != GPIO_FALLING)
        LPC_GPIOINT->ENR2 |= mask;
      break;
  }
}
/*----------------------------------------------------------------------------*/
static void processInterrupt(uint8_t channel)
{
  struct GpioInterrupt *current;
  uint32_t state;

  switch (channel)
  {
    case 0:
      state = LPC_GPIOINT->STATR0 | LPC_GPIOINT->STATF0;
      current = descriptors[0];
      break;
    case 2:
      state = LPC_GPIOINT->STATR2 | LPC_GPIOINT->STATF2;
      current = descriptors[1];
      break;
    default:
      return;
  }

  while (current)
  {
    if ((state & (1 << current->pin.offset)) && current->callback)
      current->callback(current->callbackArgument);
    current = current->next;
  }

  switch (channel)
  {
    case 0:
      LPC_GPIOINT->CLR0 = state;
      break;
    case 2:
      LPC_GPIOINT->CLR2 = state;
      break;
  }
}
/*----------------------------------------------------------------------------*/
static enum result resetDescriptor(union GpioPin pin)
{
  const uint8_t index = !pin.port ? 0 : 1;
  struct GpioInterrupt *current = descriptors[index];

  /* Remove the interrupt from chain */
  if (!current)
    return E_ERROR;
  if (current->pin.key == pin.key)
  {
    descriptors[index] = descriptors[index]->next;
    if (!descriptors[0] && !descriptors[1])
      irqDisable(EINT3_IRQ);
    return E_OK;
  }
  else
  {
    while (current->next)
    {
      if (current->next->pin.key == pin.key)
      {
        current->next = current->next->next;
        return E_OK;
      }
      current = current->next;
    }
    return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(union GpioPin pin,
    struct GpioInterrupt *interrupt)
{
  const uint8_t index = !pin.port ? 0 : 1;
  struct GpioInterrupt *current = descriptors[index];

  /* Attach new interrupt to descriptor chain */
  if (!current)
  {
    if (!descriptors[0] && !descriptors[1])
      irqEnable(EINT3_IRQ);
    descriptors[index] = interrupt;
  }
  else
  {
    while (current->next)
    {
      if (current->pin.key == pin.key)
        return E_BUSY;
      current = current->next;
    }
    if (current->pin.key != pin.key)
      current->next = interrupt;
    else
      return E_BUSY;
  }
  return E_OK;
}
/*----------------------------------------------------------------------------*/
void EINT3_ISR(void)
{
  if (LPC_GPIOINT->STATUS & STATUS_P0INT)
    processInterrupt(0);
  if (LPC_GPIOINT->STATUS & STATUS_P2INT)
    processInterrupt(2);
}
/*----------------------------------------------------------------------------*/
static enum result gpioIntInit(void *object, const void *configPtr)
{
  const struct GpioInterruptConfig * const config = configPtr;
  struct GpioInterrupt *interrupt = object;
  enum result res;

  struct Gpio input = gpioInit(config->pin);
  /* External interrupt functionality is available only on two ports */
  if (!gpioGetKey(input) || (input.pin.port != 0 && input.pin.port != 2))
    return E_VALUE;

  gpioInput(input);
  interrupt->pin = input.pin;

  /* Try to set peripheral descriptor */
  if ((res = setDescriptor(interrupt->pin, interrupt)) != E_OK)
    return res;

  interrupt->callback = 0;
  interrupt->mode = config->mode;
  interrupt->next = 0;

  enableInterrupt(interrupt->pin, interrupt->mode);
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void gpioIntDeinit(void *object)
{
  const union GpioPin pin = ((struct GpioInterrupt *)object)->pin;

  disableInterrupt(pin);
  resetDescriptor(pin);
}
/*----------------------------------------------------------------------------*/
static void gpioIntCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct GpioInterrupt *interrupt = object;

  interrupt->callbackArgument = argument;
  interrupt->callback = callback;
}
/*----------------------------------------------------------------------------*/
static void gpioIntSetEnabled(void *object, bool state)
{
  struct GpioInterrupt *interrupt = object;

  if (state)
    enableInterrupt(interrupt->pin, interrupt->mode);
  else
    disableInterrupt(interrupt->pin);
}