/*
 * core/cortex/m3/asm.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef ASM_H_
#define ASM_H_
/*----------------------------------------------------------------------------*/
static void __dmb(void)
{
  __asm__ volatile ("DMB");
}
/*----------------------------------------------------------------------------*/
static void __irqDisable(void)
{
  __asm__ volatile ("CPSID i");
}
/*----------------------------------------------------------------------------*/
static void __irqEnable(void)
{
  __asm__ volatile ("CPSIE i");
}
/*----------------------------------------------------------------------------*/
static inline uint8_t __ldrexb(volatile uint8_t *address)
{
  uint8_t result;

  __asm__ volatile (
      "LDREXB %[result], [%[address]]"
      : [result] "=r" (result)
      : [address] "r" (address)
  );
  return result;
}
/*----------------------------------------------------------------------------*/
static inline uint32_t __rev(uint32_t value)
{
  uint32_t result;

  __asm__ volatile (
      "REV %[result], %[value]"
      : [result] "=r" (result)
      : [value] "r" (value)
  );
  return result;
}
/*----------------------------------------------------------------------------*/
static inline uint16_t __rev16(uint16_t value)
{
  uint16_t result;

  __asm__ volatile (
      "REV16 %[result], %[value]"
      : [result] "=r" (result)
      : [value] "r" (value)
  );
  return result;
}
/*----------------------------------------------------------------------------*/
static inline uint32_t __strexb(uint8_t value, volatile uint8_t *address)
{
  uint32_t result;

  __asm__ volatile (
      "STREXB %[result], %[value], [%[address]]"
      : [result] "=r" (result)
      : [address] "r" (address), [value] "r" (value)
  );
  return result;
}
/*----------------------------------------------------------------------------*/
static inline void __wfi(void)
{
  __asm__ volatile ("WFI");
}
/*----------------------------------------------------------------------------*/
#endif /* ASM_H_ */