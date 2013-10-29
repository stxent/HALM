/*
 * core/cortex/m0/core_defs.h
  * Based on original by ARM Limited
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CORE_DEFS_H_
#define CORE_DEFS_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
/*----------------------------------------------------------------------------*/
#define __rw__ volatile
#define __r__ const volatile
#define __w__ volatile
/*------------------System Control Block--------------------------------------*/
typedef struct
{
  __r__  uint32_t CPUID; /* CPU ID Base Register */
  __rw__ uint32_t ICSR; /* Interrupt Control State Register */
         uint32_t RESERVED0;
  __rw__ uint32_t AIRCR; /* Application Interrupt and Reset Control Register */
  __rw__ uint32_t SCR; /* System Control Register */
  __rw__ uint32_t CCR; /* Configuration Control Register */
         uint32_t RESERVED1;
  __rw__ uint8_t  SHP[2]; /* System Handlers Priority Registers */
} SCB_Type;
/*------------------System Tick Timer-----------------------------------------*/
typedef struct
{
  __rw__ uint32_t CTRL; /* SysTick Control and Status Register */
  __rw__ uint32_t LOAD; /* SysTick Reload Value Register */
  __rw__ uint32_t VAL; /* SysTick Current Value Register */
  __r__  uint32_t CALIB; /* SysTick Calibration Register */
} SYSTICK_Type;
/*------------------Power Management Unit-------------------------------------*/
typedef struct
{
  __rw__ uint32_t ISER[1]; /* Interrupt Set Enable Register */
         uint32_t RESERVED0[31];
  __rw__ uint32_t ICER[1]; /* Interrupt Clear Enable Register */
         uint32_t RESERVED1[31];
  __rw__ uint32_t ISPR[1]; /* Interrupt Set Pending Register */
         uint32_t RESERVED2[31];
  __rw__ uint32_t ICPR[1]; /* Interrupt Clear Pending Register */
         uint32_t RESERVED3[95];
  __rw__ uint32_t IPR[8]; /* Interrupt Priority Registers */
}  NVIC_Type;
/*----------------------------------------------------------------------------*/
/* Base addresses of Cortex-M3 Hardware */
#define SCS_BASE        (0xE000E000UL)
#define SYSTICK_BASE    (SCS_BASE + 0x0010UL)
#define NVIC_BASE       (SCS_BASE + 0x0100UL)
#define SCB_BASE        (SCS_BASE + 0x0D00UL)
/*----------------------------------------------------------------------------*/
/* Hardware declaration */
#define SCB             ((SCB_Type *)SCB_BASE)
#define SYSTICK         ((SYSTICK_Type *)SYSTICK_BASE)
#define NVIC            ((NVIC_Type *)NVIC_BASE)
/*----------------------------------------------------------------------------*/
#undef __w__
#undef __r__
#undef __rw__
/*----------------------------------------------------------------------------*/
#endif /* CORE_DEFS_H_ */