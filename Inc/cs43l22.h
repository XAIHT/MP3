/**
  ******************************************************************************
  * @file    cs43l22.h
  * @author  SystemAgent
  * @brief   This file contains all the functions prototypes for the CS43L22.c driver.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CS43L22_H
#define __CS43L22_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* CS43L22 address */
#define CS43L22_I2C_ADDRESS             0x94

/* CS43L22 Registers */
#define CS43L22_REG_ID                  0x01
#define CS43L22_REG_POWER_CTL1          0x02
#define CS43L22_REG_POWER_CTL2          0x04
#define CS43L22_REG_CLOCKING_CTL        0x05
#define CS43L22_REG_INTERFACE_CTL1      0x06
#define CS43L22_REG_INTERFACE_CTL2      0x07
#define CS43L22_REG_PASSTHROUGH_A       0x08
#define CS43L22_REG_PASSTHROUGH_B       0x09
#define CS43L22_REG_ANALOG_ZC_SR_SETT   0x0A
#define CS43L22_REG_PASSTHROUGH_GANG_CTL 0x0C
#define CS43L22_REG_PLAYBACK_CTL1       0x0D
#define CS43L22_REG_MISC_CTL            0x0E
#define CS43L22_REG_PLAYBACK_CTL2       0x0F
#define CS43L22_REG_PASSTHROUGH_A_VOL   0x14
#define CS43L22_REG_PASSTHROUGH_B_VOL   0x15
#define CS43L22_REG_PCMA_VOL            0x1A
#define CS43L22_REG_PCMB_VOL            0x1B
#define CS43L22_REG_BEEP_FREQ_ON_TIME   0x1C
#define CS43L22_REG_BEEP_VOL_OFF_TIME   0x1D
#define CS43L22_REG_BEEP_TONE_CFG       0x1E
#define CS43L22_REG_TONE_CTL            0x1F
#define CS43L22_REG_MASTER_A_VOL        0x20
#define CS43L22_REG_MASTER_B_VOL        0x21
#define CS43L22_REG_HEADPHONE_A_VOL     0x22
#define CS43L22_REG_HEADPHONE_B_VOL     0x23
#define CS43L22_REG_MASTER_LIM_CFL      0x24
#define CS43L22_REG_MASTER_LIM_CFR      0x25
#define CS43L22_REG_POWER_STATUS        0x26
#define CS43L22_REG_BAT_COMP            0x27
#define CS43L22_REG_VP_BAT_LEVEL        0x28
#define CS43L22_REG_SPEAKER_STATUS      0x29
#define CS43L22_REG_TEMPMON_CTL         0x30
#define CS43L22_REG_THERMAL_FOLD        0x32
#define CS43L22_REG_CHARGE_PUMP_FREQ    0x34

/* Exported functions ------------------------------------------------------- */
void CS43L22_Init(void);
void CS43L22_SetVolume(uint8_t volume);
void CS43L22_Start(void);
void CS43L22_Stop(void);
void CS43L22_Beep(void); // Simple beep function for testing

#ifdef __cplusplus
}
#endif

#endif /* __CS43L22_H */
