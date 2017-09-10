################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../acars.c \
../acarsdec.c \
../air.c \
../alsa.c \
../msk.c \
../output.c \
../rtl.c \
../soundfile.c \
../uhd.c 

O_SRCS += \
../acars.o \
../acarsdec.o \
../air.o \
../alsa.o \
../msk.o \
../output.o \
../rtl.o \
../soundfile.o \
../uhd.o 

OBJS += \
./acars.o \
./acarsdec.o \
./air.o \
./alsa.o \
./msk.o \
./output.o \
./rtl.o \
./soundfile.o \
./uhd.o 

C_DEPS += \
./acars.d \
./acarsdec.d \
./air.d \
./alsa.d \
./msk.d \
./output.d \
./rtl.d \
./soundfile.d \
./uhd.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-oe-linux-gnueabi-gcc -I/usr/local/oecore-x86_64/sysroots/armv7ahf-vfp-neon-oe-linux-gnueabi/usr/include -O2 -g -Wall -c -fmessage-length=0  -march=armv7-a -mfloat-abi=hard -mfpu=neon -D WITH_UHD --sysroot=/usr/local/oecore-x86_64/sysroots/armv7ahf-vfp-neon-oe-linux-gnueabi -v -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


