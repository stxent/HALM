/*
 * platform/nxp/lpc13xx/clocking.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef PLATFORM_NXP_LPC13XX_CLOCKING_H_
#define PLATFORM_NXP_LPC13XX_CLOCKING_H_
/*----------------------------------------------------------------------------*/
#include <clock.h>
/*----------------------------------------------------------------------------*/
enum clockSource
{
  CLOCK_INTERNAL,
  CLOCK_EXTERNAL,
  CLOCK_PLL,
  CLOCK_USB_PLL,
  CLOCK_WDT,
  CLOCK_MAIN
};
/*----------------------------------------------------------------------------*/
enum wdtFrequency
{
  /* Watchdog oscillator analog output frequency in kHz */
  WDT_FREQ_600 = 0x01,
  WDT_FREQ_1050,
  WDT_FREQ_1400,
  WDT_FREQ_1750,
  WDT_FREQ_2100,
  WDT_FREQ_2400,
  WDT_FREQ_2700,
  WDT_FREQ_3000,
  WDT_FREQ_3250,
  WDT_FREQ_3500,
  WDT_FREQ_3750,
  WDT_FREQ_4000,
  WDT_FREQ_4200,
  WDT_FREQ_4400,
  WDT_FREQ_4600
};
/*----------------------------------------------------------------------------*/
extern const struct ClockClass * const ExternalOsc;
extern const struct ClockClass * const InternalOsc;
extern const struct ClockClass * const WdtOsc;
extern const struct ClockClass * const SystemPll;
extern const struct ClockClass * const UsbPll;
extern const struct ClockClass * const MainClock;
extern const struct ClockClass * const UsbClock;
extern const struct ClockClass * const WdtClock;
/*----------------------------------------------------------------------------*/
struct ExternalOscConfig
{
  uint32_t frequency;
  bool bypass;
};
/*----------------------------------------------------------------------------*/
struct WdtOscConfig
{
  uint8_t divider;
  enum wdtFrequency frequency;
};
/*----------------------------------------------------------------------------*/
struct PllConfig
{
  uint16_t multiplier;
  uint8_t divider;
  enum clockSource source;
};
/*----------------------------------------------------------------------------*/
struct MainClockConfig
{
  uint8_t divider;
  enum clockSource source;
};
/*----------------------------------------------------------------------------*/
struct UsbClockConfig
{
  enum clockSource source;
};
/*----------------------------------------------------------------------------*/
struct WdtClockConfig
{
  uint8_t divider;
  enum clockSource source;
};
/*----------------------------------------------------------------------------*/
#endif /* PLATFORM_NXP_LPC13XX_CLOCKING_H_ */
