/*
 * fast_gpio_bus_common.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <bits.h>
#include <platform/nxp/fast_gpio_bus.h>
/*----------------------------------------------------------------------------*/
void fastGpioBusConfigPins(struct FastGpioBus *bus,
    const struct FastGpioBusConfig *config)
{
  unsigned int number = 0;

  /* Find number of pins in the array */
  while (config->pins[number] && ++number < 32);

  assert(number && number <= 32);

  for (unsigned int index = 0; index < number; ++index)
  {
    const struct Pin pin = pinInit(config->pins[index]);
    assert(pinValid(pin));

    if (index)
    {
      assert(pin.data.port == bus->first.data.port);
      assert(pin.data.offset == bus->first.data.offset + index);
    }
    else
      bus->first = pin;

    if (config->direction == PIN_OUTPUT)
    {
      pinOutput(pin, (config->initial >> index) & 1);
      pinSetType(pin, config->type);
      pinSetSlewRate(pin, config->rate);
    }
    else
      pinInput(pin);

    pinSetPull(pin, config->pull);
  }

  bus->mask = BIT_FIELD(MASK(number), bus->first.data.offset);
}