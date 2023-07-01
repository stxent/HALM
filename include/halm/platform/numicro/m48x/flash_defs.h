/*
 * halm/platform/numicro/m48x/flash_defs.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef HALM_PLATFORM_NUMICRO_FLASH_DEFS_H_
#error This header should not be included directly
#endif

#ifndef HALM_PLATFORM_NUMICRO_M48X_FLASH_DEFS_H_
#define HALM_PLATFORM_NUMICRO_M48X_FLASH_DEFS_H_
/*----------------------------------------------------------------------------*/
#include <xcore/bits.h>
/*----------------------------------------------------------------------------*/
#define FLASH_PAGE_SIZE                 4096
#define FLASH_BLOCK_SIZE                (FLASH_PAGE_SIZE * 4)

#define FLASH_BANK_0_ADDRESS            0x00000000UL
#define FLASH_BANK_1_ADDRESS            0x00040000UL
#define FLASH_LDROM_ADDRESS             0x00100000UL
#define FLASH_SPROM_ADDRESS             0x00200000UL
#define FLASH_CONFIG_ADDRESS            0x00300000UL
#define FLASH_KPROM_ADDRESS             0x00301000UL
#define FLASH_OTP_ADDRESS               0x00310000UL
/*------------------ISP Control register--------------------------------------*/
#define ISPCTL_ISPEN                    BIT(0)
#define ISPCTL_BS                       BIT(1)
#define ISPCTL_APUEN                    BIT(3)
#define ISPCTL_CFGUEN                   BIT(4)
#define ISPCTL_LDUEN                    BIT(5)
#define ISPCTL_ISPFF                    BIT(6)
#define ISPCTL_BL                       BIT(16)
/*------------------ISP Data register-----------------------------------------*/
#define ISPDAT_SPROM_ERASE_KEY          0x0055AA03UL
/*------------------ISP Command register--------------------------------------*/
enum
{
  CMD_FLASH_READ_32             = 0x00,
  CMD_READ_UNIQUE_ID            = 0x04,
  CMD_READ_FLASH_ALLONE_RESULT  = 0x08,
  CMD_READ_COMPANY_ID           = 0x0B,
  CMD_READ_DEVICE_ID            = 0x0C,
  CMD_READ_CHECKSUM             = 0x0D,
  CMD_FLASH_PROGRAM_32          = 0x21,
  CMD_FLASH_PAGE_ERASE          = 0x22,
  CMD_FLASH_BANK_ERASE          = 0x23,
  CMD_FLASH_BLOCK_ERASE         = 0x25,
  CMD_FLASH_PROGRAM_MULTI       = 0x27,
  CMD_RUN_FLASH_ALLONE          = 0x28,
  CMD_RUN_CHECKSUM              = 0x2D,
  CMD_VECTOR_REMAP              = 0x2E,
  CMD_FLASH_READ_64             = 0x40,
  CMD_FLASH_PROGRAM_64          = 0x61
};

#define ISPCMD_CMD(value)               BIT_FIELD((value), 0)
/*------------------ISP Trigger Control register------------------------------*/
#define ISPTRG_ISPGO                    BIT(0)
/*------------------Flash Access Time Control register------------------------*/
#define FTCTL_CACHEINV                  BIT(9)
/*------------------ISP Status register---------------------------------------*/
enum
{
  CBS_LDROM_WITH_IAP    = 0,
  CBS_LDROM_WITHOUT_IAP = 1,
  CBS_APROM_WITH_IAP    = 2,
  CBS_APROM_WITHOUT_IAP = 3
};

#define ISPSTS_ISPBUSY                  BIT(0)

#define ISPSTS_CBS_MASK                 BIT_FIELD(MASK(2), 1)
#define ISPSTS_CBS(value)               BIT_FIELD((value), 1)
#define ISPSTS_CBS_VALUE(reg)           FIELD_VALUE((reg), ISPSTS_CBS_MASK, 1)

#define ISPSTS_MBS                      BIT(3)
#define ISPSTS_PGFF                     BIT(5)
#define ISPSTS_ISPFF                    BIT(6)
#define ISPSTS_ALLONE                   BIT(7)

#define ISPSTS_VECMAP_MASK              BIT_FIELD(MASK(15), 9)
#define ISPSTS_VECMAP(value)            BIT_FIELD((value), 9)
#define ISPSTS_VECMAP_VALUE(reg)        FIELD_VALUE((reg), ISPSTS_CBS_MASK, 9)
/*------------------Flash Access Cycle Control register-----------------------*/
#define CYCCTL_CYCLE_MAX                8
#define CYCCTL_CYCLE_MASK               BIT_FIELD(MASK(4), 0)
#define CYCCTL_CYCLE(value)             BIT_FIELD((value), 0)
#define CYCCTL_CYCLE_VALUE(reg)         FIELD_VALUE((reg), CYCCTL_CYCLE_MASK, 0)
/*------------------KPROM KEY Comparison Trigger Control register-------------*/
#define KPKEYTRG_KPKEYGO                BIT(0)
#define KPKEYTRG_TCEN                   BIT(1)
/*------------------KPROM KEY Status register---------------------------------*/
#define KPKEYSTS_KEYBUSY                BIT(0)
#define KPKEYSTS_KEYLOCK                BIT(1)
#define KPKEYSTS_KEYMATCH               BIT(2)
#define KPKEYSTS_FORBID                 BIT(3)
#define KPKEYSTS_KEYFLAG                BIT(4)
#define KPKEYSTS_CFGFLAG                BIT(5)
/*------------------ISP Multi-program Status register-------------------------*/
#define MPSTS_MPBUSY                    BIT(0)
#define MPSTS_PPGO                      BIT(1)
#define MPSTS_ISPFF                     BIT(2)
#define MPSTS_KEYBUSY                   BIT(3)
#define MPSTS_D0                        BIT(4)
#define MPSTS_D1                        BIT(5)
#define MPSTS_D2                        BIT(6)
#define MPSTS_D3                        BIT(7)
/*----------------------------------------------------------------------------*/
// TODO KPKEY and XOM
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NUMICRO_M48X_FLASH_DEFS_H_ */
