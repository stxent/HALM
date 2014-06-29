/*
 * gpio_interrupt.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <irq.h>
#include <platform/nxp/gpio_interrupt.h>
/*----------------------------------------------------------------------------*/
static inline LPC_GPIO_Type *calcPort(uint8_t);
static void processInterrupt(uint8_t);
static enum result resetDescriptor(union PinData);
static enum result setDescriptor(union PinData, struct GpioInterrupt *);
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
static const irq_t gpioIntIrq[] = {
    PIOINT0_IRQ, PIOINT1_IRQ, PIOINT2_IRQ, PIOINT3_IRQ
};
/*----------------------------------------------------------------------------*/
const struct InterruptClass * const GpioInterrupt = &gpioIntTable;
static struct GpioInterrupt *descriptors[4] = {0};
/*----------------------------------------------------------------------------*/
static inline LPC_GPIO_Type *calcPort(uint8_t port)
{
  return (LPC_GPIO_Type *)((uint32_t)LPC_GPIO0 +
      ((uint32_t)LPC_GPIO1 - (uint32_t)LPC_GPIO0) * port);
}
/*----------------------------------------------------------------------------*/
static void processInterrupt(uint8_t channel)
{
  const struct GpioInterrupt *current = descriptors[channel];
  LPC_GPIO_Type * const reg = calcPort(channel);
  const uint32_t state = reg->MIS;

  while (current)
  {
    if ((state & (1 << current->pin.offset)) && current->callback)
      current->callback(current->callbackArgument);
    current = current->next;
  }
  reg->IC = state; //FIXME Add check
  /* Synchronizer logic causes a delay of 2 clocks */
}
/*----------------------------------------------------------------------------*/
static enum result resetDescriptor(union PinData pin)
{
  struct GpioInterrupt *current = descriptors[pin.port];

  if (!current)
    return E_ERROR;

  /* Remove the interrupt from chain */
  if (current->pin.key == pin.key)
  {
    descriptors[pin.port] = descriptors[pin.port]->next;
    if (!descriptors[pin.port])
      irqDisable(gpioIntIrq[pin.port]);

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
static enum result setDescriptor(union PinData pin,
    struct GpioInterrupt *interrupt)
{
  struct GpioInterrupt *current = descriptors[pin.port];

  /* Attach new interrupt to descriptor chain */
  if (!current)
  {
    irqEnable(gpioIntIrq[pin.port]);
    descriptors[pin.port] = interrupt;
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
void PIOINT0_ISR(void)
{
  processInterrupt(0);
}
/*----------------------------------------------------------------------------*/
void PIOINT1_ISR(void)
{
  processInterrupt(1);
}
/*----------------------------------------------------------------------------*/
void PIOINT2_ISR(void)
{
  processInterrupt(2);
}
/*----------------------------------------------------------------------------*/
void PIOINT3_ISR(void)
{
  processInterrupt(3);
}
/*----------------------------------------------------------------------------*/
static enum result gpioIntInit(void *object, const void *configPtr)
{
  const struct GpioInterruptConfig * const config = configPtr;
  struct GpioInterrupt * const interrupt = object;
  enum result res;

  struct Pin input = pinInit(config->pin);
  if (!pinGetKey(input))
    return E_VALUE;

  pinInput(input);
  interrupt->pin = input.data;

  /* Try to set peripheral descriptor */
  if ((res = setDescriptor(interrupt->pin, interrupt)) != E_OK)
    return res;

  interrupt->callback = 0;
  interrupt->mode = config->mode;
  interrupt->next = 0;

  LPC_GPIO_Type * const reg = calcPort(interrupt->pin.port);
  const uint32_t mask = 1 << interrupt->pin.offset;

  /* Configure interrupt as edge sensitive*/
  reg->IS &= ~mask;
  /* Configure edge sensitivity options */
  switch (config->mode)
  {
    case GPIO_RISING:
      reg->IEV |= mask;
      break;
    case GPIO_FALLING:
      reg->IEV &= ~mask;
      break;
    case GPIO_TOGGLE:
      reg->IBE |= mask;
      break;
  }
  /* Clear pending interrupt flag */
  reg->IC = mask;
  /* Disable interrupt masking */
  reg->IE |= mask;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void gpioIntDeinit(void *object)
{
  const union PinData data = ((struct GpioInterrupt *)object)->pin;
  const uint32_t mask = 1 << data.offset;
  LPC_GPIO_Type * const reg = calcPort(data.port);

  reg->IE &= ~mask;
  reg->IBE &= ~mask;
  reg->IEV &= ~mask;
  resetDescriptor(data);
}
/*----------------------------------------------------------------------------*/
static void gpioIntCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct GpioInterrupt * const interrupt = object;

  interrupt->callbackArgument = argument;
  interrupt->callback = callback;
}
/*----------------------------------------------------------------------------*/
static void gpioIntSetEnabled(void *object, bool state)
{
  const union PinData data = ((struct GpioInterrupt *)object)->pin;
  const uint32_t mask = 1 << data.offset;
  LPC_GPIO_Type * const reg = calcPort(data.port);

  if (state)
  {
    reg->IC = mask;
    reg->IE |= mask;
  }
  else
    reg->IE &= ~mask;
}
