/**
  ******************************************************************************
  * @file    cs43l22.c
  * @author  SystemAgent
  * @brief   This file provides the CS43L22 Audio Codec driver.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "cs43l22.h"
#include "i2c.h"

/* Private defines -----------------------------------------------------------*/
#define I2C_TIMEOUT 100

/* External variables --------------------------------------------------------*/
// Defined in i2c.c/main.c
extern I2C_HandleTypeDef hi2c1;

/* Private function prototypes -----------------------------------------------*/
static void CS43L22_WriteRegister(uint8_t RegisterAddr, uint8_t Value);

/**
  * @brief  Un-Resets and Initializes the CS43L22 Codec.
  * @param  None
  * @retval None
  */
void CS43L22_Init(void)
{
  HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(5);

  CS43L22_WriteRegister(0x00, 0x99);
  CS43L22_WriteRegister(0x47, 0x80);

  uint8_t reg32 = 0;
  if (HAL_I2C_Mem_Read(&hi2c1, CS43L22_I2C_ADDRESS, 0x32, I2C_MEMADD_SIZE_8BIT, &reg32, 1, I2C_TIMEOUT) != HAL_OK)
  {
    Error_Handler();
  }
  CS43L22_WriteRegister(0x32, reg32 | 0x80u);
  CS43L22_WriteRegister(0x32, reg32 & (uint8_t)~0x80u);
  CS43L22_WriteRegister(0x00, 0x00);

  CS43L22_WriteRegister(CS43L22_REG_POWER_CTL1, 0x01);
  CS43L22_WriteRegister(CS43L22_REG_POWER_CTL2, 0xAF);
  CS43L22_WriteRegister(CS43L22_REG_CLOCKING_CTL, 0x81);
  CS43L22_WriteRegister(CS43L22_REG_INTERFACE_CTL1, 0x07);
  CS43L22_WriteRegister(CS43L22_REG_INTERFACE_CTL2, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_PASSTHROUGH_A, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_PASSTHROUGH_B, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_ANALOG_ZC_SR_SETT, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_MISC_CTL, 0x04);
  CS43L22_WriteRegister(CS43L22_REG_PLAYBACK_CTL1, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_PLAYBACK_CTL2, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_PCMA_VOL, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_PCMB_VOL, 0x00);

  CS43L22_SetVolume(0xE0);
  CS43L22_WriteRegister(CS43L22_REG_HEADPHONE_A_VOL, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_HEADPHONE_B_VOL, 0x00);

  CS43L22_WriteRegister(CS43L22_REG_POWER_CTL1, 0x9E);
  HAL_Delay(10);
}

/**
  * @brief  Sets the volume.
  * @param  volume: 0 to 255
  * @retval None
  */
void CS43L22_SetVolume(uint8_t volume)
{
  // Master Volume A and B
  CS43L22_WriteRegister(CS43L22_REG_MASTER_A_VOL, volume);
  CS43L22_WriteRegister(CS43L22_REG_MASTER_B_VOL, volume);
}

/**
  * @brief  Enables the Beep Generator for testing.
  * @retval None
  */
void CS43L22_Beep(void)
{
  // Ensure codec is powered and outputs are enabled
  CS43L22_WriteRegister(CS43L22_REG_POWER_CTL2, 0xAF);

  // Beep generator must be mixed to output and audible.
  // Configure a reasonable beep volume/off time.
  CS43L22_WriteRegister(CS43L22_REG_BEEP_VOL_OFF_TIME, 0x06);
  CS43L22_WriteRegister(CS43L22_REG_BEEP_FREQ_ON_TIME, 0x00);
  CS43L22_WriteRegister(CS43L22_REG_BEEP_TONE_CFG, 0xC0); // Play beep continuously
}

/**
  * @brief  Start audio playback (unmute/power up).
  * @retval None
  */
void CS43L22_Start(void)
{
    CS43L22_WriteRegister(CS43L22_REG_PLAYBACK_CTL1, 0x00);
    CS43L22_WriteRegister(CS43L22_REG_POWER_CTL1, 0x9E);
}

/**
  * @brief  Stop audio.
  * @retval None
  */
void CS43L22_Stop(void)
{
    // Write 0x01 to Power Ctl 1 (Power Down)
    CS43L22_WriteRegister(CS43L22_REG_POWER_CTL1, 0x01);
}

/* Private functions ---------------------------------------------------------*/

static void CS43L22_WriteRegister(uint8_t RegisterAddr, uint8_t Value)
{
  if (HAL_I2C_Mem_Write(&hi2c1, CS43L22_I2C_ADDRESS, RegisterAddr, I2C_MEMADD_SIZE_8BIT, &Value, 1, I2C_TIMEOUT) != HAL_OK)
  {
    Error_Handler();
  }
}