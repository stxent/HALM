/*
 * spi_base.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the MIT License
 */

#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/spi_base.h>
#include <halm/platform/stm32/spi_defs.h>
#include <halm/platform/stm32/stm32f1xx/pin_remap.h>
#include <halm/platform/stm32/system.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
struct SpiBlockDescriptor
{
  STM_SPI_Type *reg;
  /* Reset control identifier */
  enum SysBlockReset reset;
  /* Peripheral clock branch */
  enum SysClockBranch branch;
  /* Peripheral interrupt request identifier */
  IrqNumber irq;
};
/*----------------------------------------------------------------------------*/
static bool setInstance(uint8_t, struct SpiBase *);
/*----------------------------------------------------------------------------*/
static enum Result spiInit(void *, const void *);

#ifndef CONFIG_PLATFORM_STM32_SPI_NO_DEINIT
static void spiDeinit(void *);
#else
#define spiDeinit deletedDestructorTrap
#endif
/*----------------------------------------------------------------------------*/
const struct EntityClass * const SpiBase = &(const struct EntityClass){
    .size = 0, /* Abstract class */
    .init = spiInit,
    .deinit = spiDeinit
};
/*----------------------------------------------------------------------------*/
static const struct SpiBlockDescriptor spiBlockEntries[] = {
    {
        .reg = STM_SPI1,
        .branch = CLK_SPI1,
        .reset = RST_SPI1,
        .irq = SPI1_IRQ
    }, {
        .reg = STM_SPI2,
        .branch = CLK_SPI2,
        .reset = RST_SPI2,
        .irq = SPI2_IRQ
    }, {
        .reg = STM_SPI3,
        .branch = CLK_SPI3,
        .reset = RST_SPI3,
        .irq = SPI3_IRQ
    }
};
/*----------------------------------------------------------------------------*/
const struct PinEntry spiPins[] = {
    {
        .key = PIN(PORT_A, 4), /* SPI1_NSS */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 0)
    }, {
        .key = PIN(PORT_A, 5), /* SPI1_SCK */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 0)
    }, {
        .key = PIN(PORT_A, 6), /* SPI1_MISO */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 0)
    }, {
        .key = PIN(PORT_A, 7), /* SPI1_MOSI */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 0)
    }, {
        .key = PIN(PORT_A, 15), /* SPI1_NSS */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 1)
    }, {
        .key = PIN(PORT_B, 3), /* SPI1_SCK */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 1)
    }, {
        .key = PIN(PORT_B, 4), /* SPI1_MISO */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 1)
    }, {
        .key = PIN(PORT_B, 5), /* SPI1_MOSI */
        .channel = 0,
        .value = PACK_REMAP(REMAP_SPI1, 1)
    }, {
        .key = PIN(PORT_B, 12), /* SPI2_NSS */
        .channel = 1,
        .value = 0
    }, {
        .key = PIN(PORT_B, 13), /* SPI2_SCK */
        .channel = 1,
        .value = 0
    }, {
        .key = PIN(PORT_B, 14), /* SPI2_MISO */
        .channel = 1,
        .value = 0
    }, {
        .key = PIN(PORT_B, 15), /* SPI2_MOSI */
        .channel = 1,
        .value = 0
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_A, 4), /* SPI3_NSS */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 1)
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_A, 15), /* SPI3_NSS */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 0)
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_B, 3), /* SPI3_SCK */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 0)
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_B, 4), /* SPI3_MISO */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 0)
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_B, 5), /* SPI3_MOSI */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 0)
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_C, 10), /* SPI3_SCK */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 1)
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_C, 11), /* SPI3_MISO */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 1)
    }, {
        /* Available on STM32F105 and STM32F107 series only */
        .key = PIN(PORT_C, 12), /* SPI3_MOSI */
        .channel = 2,
        .value = PACK_REMAP(REMAP_SPI3, 1)
    }, {
        .key = 0 /* End of pin function association list */
    }
};
/*----------------------------------------------------------------------------*/
static struct SpiBase *instances[3] = {0};
/*----------------------------------------------------------------------------*/
static bool setInstance(uint8_t channel, struct SpiBase *object)
{
  assert(channel < ARRAY_SIZE(instances));

  if (!instances[channel])
  {
    instances[channel] = object;
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
void SPI1_ISR(void)
{
  instances[0]->handler(instances[0]);
}
/*----------------------------------------------------------------------------*/
void SPI2_ISR(void)
{
  instances[1]->handler(instances[1]);
}
/*----------------------------------------------------------------------------*/
void SPI3_ISR(void)
{
  instances[2]->handler(instances[2]);
}
/*----------------------------------------------------------------------------*/
uint32_t spiGetClock(const struct SpiBase *interface)
{
  return clockFrequency(interface->channel == 0 ? Apb2Clock : Apb1Clock);
}
/*----------------------------------------------------------------------------*/
static enum Result spiInit(void *object, const void *configBase)
{
  const struct SpiBaseConfig * const config = configBase;
  struct SpiBase * const interface = object;

  interface->channel = config->channel;
  interface->handler = 0;

  if (!setInstance(interface->channel, interface))
    return E_BUSY;

  /* Configure input and output pins */
  spiConfigPins(interface, config);

  const struct SpiBlockDescriptor * const entry =
      &spiBlockEntries[interface->channel];

  /* Enable clocks to register interface and peripheral */
  sysClockEnable(entry->branch);
  /* Reset registers to default values */
  sysResetEnable(entry->reset);
  sysResetDisable(entry->reset);

  interface->irq = entry->irq;
  interface->reg = entry->reg;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_PLATFORM_STM32_SPI_NO_DEINIT
static void spiDeinit(void *object)
{
  const struct SpiBase * const interface = object;

  sysClockDisable(spiBlockEntries[interface->channel].branch);
  instances[interface->channel] = 0;
}
#endif
