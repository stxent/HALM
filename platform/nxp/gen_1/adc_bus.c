/*
 * adc_bus.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <halm/platform/nxp/adc_bus.h>
#include <halm/platform/nxp/gen_1/adc_defs.h>
/*----------------------------------------------------------------------------*/
#define MAX_CHANNELS  8
#define SAMPLE_SIZE   sizeof(uint16_t)
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *);
static void setupChannels(struct AdcBus *, const PinNumber *);
/*----------------------------------------------------------------------------*/
static enum Result adcInit(void *, const void *);
static void adcDeinit(void *);
static enum Result adcSetCallback(void *, void (*)(void *), void *);
static enum Result adcGetParam(void *, enum IfParameter, void *);
static enum Result adcSetParam(void *, enum IfParameter, const void *);
static size_t adcRead(void *, void *, size_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass adcTable = {
    .size = sizeof(struct AdcBus),
    .init = adcInit,
    .deinit = adcDeinit,

    .setCallback = adcSetCallback,
    .getParam = adcGetParam,
    .setParam = adcSetParam,
    .read = adcRead,
    .write = 0
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const AdcBus = &adcTable;
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct AdcBus * const interface = object;
  struct AdcUnit * const unit = interface->unit;
  LPC_ADC_Type * const reg = unit->base.reg;
  uint16_t *buffer = interface->buffer;

  /* Stop automatic conversion */
  reg->CR &= ~CR_BURST;
  /* Disable interrupts */
  reg->INTEN = 0;
  irqDisable(unit->base.irq);

  /* Read values and clear interrupt flags */
  for (size_t index = 0; index < interface->number; ++index)
    *buffer++ = DR_RESULT_VALUE(reg->DR[interface->pins[index].channel]);

  /* Reset buffer pointer to notify that the conversion is completed */
  interface->buffer = 0;

  adcUnitUnregister(unit);

  if (interface->callback)
    interface->callback(interface->callbackArgument);
}
/*----------------------------------------------------------------------------*/
static void setupChannels(struct AdcBus *interface, const PinNumber *pins)
{
  interface->mask = 0x00;
  interface->event = 0;

  for (size_t index = 0; index < interface->number; ++index)
  {
    adcConfigPin(&interface->unit->base, pins[index], &interface->pins[index]);

    const uint8_t channel = interface->pins[index].channel;

    /*
     * Check whether the order of pins is correct and all pins
     * are unique. Pins must be sorted by analog channel number to ensure
     * direct connection between pins in the configuration and resulting values
     * after a conversion.
     */
    assert(!(interface->mask >> channel));

    if (channel > interface->event)
      interface->event = channel;
    interface->mask |= 1 << channel;
  }
}
/*----------------------------------------------------------------------------*/
static enum Result adcInit(void *object, const void *configBase)
{
  const struct AdcBusConfig * const config = configBase;
  assert(config);

  struct AdcBus * const interface = object;

  /* Initialize input pins */
  const PinNumber *currentPin = config->pins;
  size_t number = 0;

  while (*currentPin++ && number < MAX_CHANNELS)
    ++number;

  assert(number && number <= MAX_CHANNELS);

  interface->pins = malloc(number * sizeof(struct AdcPin));
  if (!interface->pins)
    return E_MEMORY;

  interface->number = number;
  interface->unit = config->parent;

  setupChannels(interface, config->pins);

  interface->callback = 0;
  interface->blocking = true;
  interface->buffer = 0;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void adcDeinit(void *object)
{
  const struct AdcBus * const interface = object;

  for (size_t index = 0; index < interface->number; ++index)
    adcReleasePin(interface->pins[index]);

  free(interface->pins);
}
/*----------------------------------------------------------------------------*/
static enum Result adcSetCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct AdcBus * const interface = object;

  interface->callbackArgument = argument;
  interface->callback = callback;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum Result adcGetParam(void *object, enum IfParameter parameter,
    void *data __attribute__((unused)))
{
  struct AdcBus * const interface = object;

  switch (parameter)
  {
    case IF_STATUS:
      return interface->buffer ? E_BUSY : E_OK;

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static enum Result adcSetParam(void *object, enum IfParameter parameter,
    const void *data __attribute__((unused)))
{
  struct AdcBus * const interface = object;

  switch (parameter)
  {
    case IF_BLOCKING:
      interface->blocking = true;
      return E_OK;

    case IF_ZEROCOPY:
      interface->blocking = false;
      return E_OK;

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static size_t adcRead(void *object, void *buffer, size_t length)
{
  struct AdcBus * const interface = object;
  struct AdcUnit * const unit = interface->unit;
  LPC_ADC_Type * const reg = unit->base.reg;
  const size_t samples = length / SAMPLE_SIZE;

  if (samples < interface->number)
    return 0;

  if (adcUnitRegister(unit, interruptHandler, interface) != E_OK)
    return 0;

  interface->buffer = buffer;

  /* Clear pending interrupts */
  (void)reg->DR[interface->event];
  /* Enable interrupts for the channel with the highest number */
  reg->INTEN = INTEN_AD(interface->event);
  irqEnable(unit->base.irq);

  /* Enable channels */
  reg->CR = (reg->CR & ~CR_SEL_MASK) | CR_SEL(interface->mask);

  /* Start the conversion */
  reg->CR |= CR_BURST;

  if (interface->blocking)
  {
    while (interface->buffer)
      barrier();
  }

  return samples * SAMPLE_SIZE;
}
