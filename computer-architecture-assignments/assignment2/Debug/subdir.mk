################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../a.c \
../lc2sim.c \
../makeRun.c \
../temp.c 

OBJS += \
./a.o \
./lc2sim.o \
./makeRun.o \
./temp.o 

C_DEPS += \
./a.d \
./lc2sim.d \
./makeRun.d \
./temp.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


