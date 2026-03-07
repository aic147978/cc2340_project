################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/ti/ccs2041/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source/ti/ble/stack_util/config/build_components.opt" @"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source/ti/ble/stack_util/config/factory_config.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -Oz -I"D:/cc2340_project/basic_ble_LP_EM_CC2340R5_freertos_ticlang" -I"D:/cc2340_project/basic_ble_LP_EM_CC2340R5_freertos_ticlang/Release" -I"D:/cc2340_project/basic_ble_LP_EM_CC2340R5_freertos_ticlang/app" -I"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source" -I"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source/ti" -I"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source/ti/common/cc26xx" -I"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source/ti/posix/ticlang" -I"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source/third_party/freertos/include" -I"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/source/third_party/freertos/portable/GCC/ARM_CM0" -I"D:/ti/simplelink_lowpower_f3_sdk_9_14_02_16/kernel/freertos" -DICALL_NO_APP_EVENTS -DCC23X0 -DNVOCMP_NWSAMEITEM=1 -DNVOCMP_NVPAGES=6 -DFREERTOS -DNVOCMP_POSIX_MUTEX -gdwarf-3 -Wunused-function -ffunction-sections -MMD -MP -MF"drivers/$(basename $(<F)).d_raw" -MT"$(@)" -I"D:/cc2340_project/basic_ble_LP_EM_CC2340R5_freertos_ticlang/Release/syscfg" -std=c99 $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


