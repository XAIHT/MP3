################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/cs43l22.c \
../Src/gpio.c \
../Src/i2c.c \
../Src/i2s.c \
../Src/main.c \
../Src/mp3_data.c \
../Src/mp3_decoder.c \
../Src/mp3_player.c \
../Src/spi.c \
../Src/stm32f4xx_hal_msp.c \
../Src/stm32f4xx_it.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/system_stm32f4xx.c \
../Src/wav_data.c \
../Src/wav_player.c 

OBJS += \
./Src/cs43l22.o \
./Src/gpio.o \
./Src/i2c.o \
./Src/i2s.o \
./Src/main.o \
./Src/mp3_data.o \
./Src/mp3_decoder.o \
./Src/mp3_player.o \
./Src/spi.o \
./Src/stm32f4xx_hal_msp.o \
./Src/stm32f4xx_it.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/system_stm32f4xx.o \
./Src/wav_data.o \
./Src/wav_player.o 

C_DEPS += \
./Src/cs43l22.d \
./Src/gpio.d \
./Src/i2c.d \
./Src/i2s.d \
./Src/main.d \
./Src/mp3_data.d \
./Src/mp3_decoder.d \
./Src/mp3_player.d \
./Src/spi.d \
./Src/stm32f4xx_hal_msp.d \
./Src/stm32f4xx_it.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/system_stm32f4xx.d \
./Src/wav_data.d \
./Src/wav_player.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Inc -IC:/Users/angel/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/STM32F4xx_HAL_Driver/Inc -IC:/Users/angel/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -IC:/Users/angel/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/CMSIS/Device/ST/STM32F4xx/Include -IC:/Users/angel/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/cs43l22.cyclo ./Src/cs43l22.d ./Src/cs43l22.o ./Src/cs43l22.su ./Src/gpio.cyclo ./Src/gpio.d ./Src/gpio.o ./Src/gpio.su ./Src/i2c.cyclo ./Src/i2c.d ./Src/i2c.o ./Src/i2c.su ./Src/i2s.cyclo ./Src/i2s.d ./Src/i2s.o ./Src/i2s.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/mp3_data.cyclo ./Src/mp3_data.d ./Src/mp3_data.o ./Src/mp3_data.su ./Src/mp3_decoder.cyclo ./Src/mp3_decoder.d ./Src/mp3_decoder.o ./Src/mp3_decoder.su ./Src/mp3_player.cyclo ./Src/mp3_player.d ./Src/mp3_player.o ./Src/mp3_player.su ./Src/spi.cyclo ./Src/spi.d ./Src/spi.o ./Src/spi.su ./Src/stm32f4xx_hal_msp.cyclo ./Src/stm32f4xx_hal_msp.d ./Src/stm32f4xx_hal_msp.o ./Src/stm32f4xx_hal_msp.su ./Src/stm32f4xx_it.cyclo ./Src/stm32f4xx_it.d ./Src/stm32f4xx_it.o ./Src/stm32f4xx_it.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/system_stm32f4xx.cyclo ./Src/system_stm32f4xx.d ./Src/system_stm32f4xx.o ./Src/system_stm32f4xx.su ./Src/wav_data.cyclo ./Src/wav_data.d ./Src/wav_data.o ./Src/wav_data.su ./Src/wav_player.cyclo ./Src/wav_player.d ./Src/wav_player.o ./Src/wav_player.su

.PHONY: clean-Src

