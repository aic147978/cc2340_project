/******************************************************************************

 @file  main_freertos.c

 @brief main entry of the BLE stack sample application.

 Group: WCS, BTS
 Target Device: cc23xx

 ******************************************************************************
 
 Copyright (c) 2013-2026, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 
 
 *****************************************************************************/

/*******************************************************************************
 * INCLUDES
 */

/* RTOS header files */
#include <FreeRTOS.h>
#include <stdint.h>
#include <task.h>
#ifdef __ICCARM__
    #include <DLib_Threads.h>
#endif
#include <ti/drivers/Power.h>
#include <ti/devices/DeviceFamily.h>

#include "ti/ble/stack_util/icall/app/icall.h"
#include "ti/ble/stack_util/health_toolkit/assert.h"
#include "ti/ble/stack_util/bcomdef.h"

#ifndef USE_DEFAULT_USER_CFG
#include "ti/ble/app_util/config/ble_user_config.h"
// BLE user defined configuration
icall_userCfg_t user0Cfg = BLE_USER_CFG;
#endif // USE_DEFAULT_USER_CFG


/*******************************************************************************
 * MACROS
 */

/*******************************************************************************
 * CONSTANTS
 */

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * LOCAL VARIABLES
 */
/*
 * 用户可调参数：控制是否在启动阶段将 ICall 计时配置同步到 BLE 用户配置。
 * 设计意图：有些项目会在 Bootloader 或早期初始化中提前写入 user0Cfg，
 * 如果需要保留外部值，可将该宏设置为 0，避免在 main() 中再次覆盖。
 */
#define APP_SYNC_ICALL_TIMER_TO_USERCFG    (1U)

/*******************************************************************************
 * GLOBAL VARIABLES
 */

/*******************************************************************************
 * EXTERNS
 */
extern void appMain(void);
extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

/*******************************************************************************
 * @fn          Main
 *
 * @brief       Application Main
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
/**
 * @brief 主入口函数（关键函数）
 *
 * 设计意图：按“断言回调注册 -> 板级初始化 -> BLE 计时配置同步 -> 应用任务创建 -> 调度器启动”
 * 的顺序完成系统上电启动，确保 BLE 协议栈与 FreeRTOS 的时间基准一致。
 *
 * 需要根据硬件修改：Board_init() 依赖具体开发板和外设电源树配置，
 * 如迁移到自定义硬件，请在对应 Board 文件中核对时钟、GPIO、射频供电配置。
 *
 * BLE协议字段数据格式说明：
 * 1) user0Cfg.appServiceInfo->timerTickPeriod：无符号整数（tick/单位时间），
 *    表示 ICall 系统 tick 周期，供 BLE 协议定时器换算使用。
 * 2) user0Cfg.appServiceInfo->timerMaxMillisecond：无符号整数（毫秒），
 *    表示 ICall 支持的最大定时毫秒值，供 BLE 协议超时参数边界检查。
 *
 * @return 始终返回 0；正常情况下流程不会执行到 return，因为调度器会接管 CPU。
 */
int main()
{
  /* Register Application callback to trap asserts raised in the Stack */
  halAssertCback = AssertHandler;
  RegisterAssertCback(AssertHandler);

  /*
   * 需要根据硬件修改：板级初始化会打开特定硬件外设与底层驱动，
   * 在不同 PCB 或电源域设计下应复核该初始化流程是否匹配。
   */
  Board_init();

#if (APP_SYNC_ICALL_TIMER_TO_USERCFG == 1U)
  /* Update User Configuration of the stack */
  user0Cfg.appServiceInfo->timerTickPeriod = ICall_getTickPeriod();
  user0Cfg.appServiceInfo->timerMaxMillisecond  = ICall_getMaxMSecs();
#endif

  /* Initialize all applications tasks */
  appMain();

  /* Start the FreeRTOS scheduler */
  vTaskStartScheduler();

  return 0;

}

//*****************************************************************************
//
//! \brief Application defined stack overflow hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
/**
 * @brief FreeRTOS 栈溢出钩子（关键函数）
 *
 * 设计意图：统一把任务栈溢出故障映射为 BLE/系统通用断言，
 * 复用 AssertHandler 的故障收敛策略，避免出现多套错误处理逻辑。
 *
 * @param pxTask 触发栈溢出的任务句柄（当前版本未使用，保留用于后续日志扩展）。
 * @param pcTaskName 触发栈溢出的任务名称（当前版本未使用，保留用于后续日志扩展）。
 */
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    /*
     * 设计意图：在资源紧张场景下，优先进入断言路径防止系统继续运行造成状态破坏。
     * 这里保留参数但不直接打印，避免在异常上下文中引入额外堆栈/串口依赖。
     */
    (void)pxTask;
    (void)pcTaskName;
    AssertHandler(HAL_ASSERT_CAUSE_STACK_OVERFLOW_ERROR, 0);
}

/*******************************************************************************
 * @fn          AssertHandler
 *
 * @brief       This is the Application's callback handler for asserts raised
 *              in the stack.  When EXT_HAL_ASSERT is defined in the Stack Wrapper
 *              project this function will be called when an assert is raised,
 *              and can be used to observe or trap a violation from expected
 *              behavior.
 *
 *              As an example, for Heap allocation failures the Stack will raise
 *              HAL_ASSERT_CAUSE_OUT_OF_MEMORY as the assertCause and
 *              HAL_ASSERT_SUBCAUSE_NONE as the assertSubcause.  An application
 *              developer could trap any malloc failure on the stack by calling
 *              HAL_ASSERT_SPINLOCK under the matching case.
 *
 *              An application developer is encouraged to extend this function
 *              for use by their own application.  To do this, add assert.c
 *              to your project workspace, the path to assert.h (this can
 *              be found on the stack side). Asserts are raised by including
 *              assert.h and using macro HAL_ASSERT(cause) to raise an
 *              assert with argument assertCause.  the assertSubcause may be
 *              optionally set by macro HAL_ASSERT_SET_SUBCAUSE(subCause) prior
 *              to asserting the cause it describes. More information is
 *              available in assert.h.
 *
 * input parameters
 *
 * @param       assertCause    - Assert cause as defined in assert.h.
 * @param       assertSubcause - Optional assert subcause (see assert.h).
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
/**
 * @brief 统一断言处理函数（关键函数）
 *
 * 设计意图：将 BLE 协议栈、ICall、RTOS 的异常统一收敛到一个入口，
 * 便于后续按产品需求扩展为“记录日志/复位系统/上报故障码”等策略。
 * 当前实现采用 HAL_ASSERT_SPINLOCK 进行故障锁定，以确保问题可被调试器稳定捕获。
 *
 * @param assertCause 断言主因（uint8_t，枚举值，数据格式为 8 位无符号整型故障码）。
 * @param assertSubcause 断言子因（uint8_t，枚举值，数据格式为 8 位无符号整型补充码）。
 */
void AssertHandler(uint8_t assertCause, uint8_t assertSubcause)
{
    /*
     * 设计意图：优先按主因分类，再在必要时解析子因，
     * 保持分支结构清晰，方便维护不同错误来源的处理策略。
     */
    switch(assertCause)
    {
        case HAL_ASSERT_CAUSE_OUT_OF_MEMORY:
        {
            /* 内存耗尽代表系统已无法保证后续行为可预测，因此立即锁定现场。 */
            HAL_ASSERT_SPINLOCK;
            break;
        }

        case HAL_ASSERT_CAUSE_INTERNAL_ERROR:
        {
            /* 先判断内部错误的细分原因，为后续扩展差异化恢复策略预留位置。 */
            if(assertSubcause == HAL_ASSERT_SUBCAUSE_FW_INERNAL_ERROR)
            {
                /* 固件内部一致性错误，通常需要停机排查版本与配置匹配性。 */
                HAL_ASSERT_SPINLOCK;
            }
            else
            {
                /* 其他内部错误同样先锁定，避免带病运行引发二次故障。 */
                HAL_ASSERT_SPINLOCK;
            }
            break;
        }

        case HAL_ASSERT_CAUSE_ICALL_ABORT:
        {
            HAL_ASSERT_SPINLOCK;
            break;
        }

        case HAL_ASSERT_CAUSE_ICALL_TIMEOUT:
        {
            HAL_ASSERT_SPINLOCK;
            break;
        }

        case HAL_ASSERT_CAUSE_WRONG_API_CALL:
        {
            HAL_ASSERT_SPINLOCK;
            break;
        }

        case HAL_ASSERT_CAUSE_STACK_OVERFLOW_ERROR:
        {
            HAL_ASSERT_SPINLOCK;
            break;
        }

        case HAL_ASSERT_CAUSE_LL_INIT_RNG_NOISE_FAILURE:
        {
            /*
             * Device must be reset to recover from this case.
             * 需要根据硬件修改：若产品有外部看门狗/PMIC 复位链路，
             * 请在此处补充对应硬件复位时序，确保射频与电源域被完整重置。
             *
             * The HAL_ASSERT_SPINLOCK with is replacable with custom handling,
             * at the end of which Power_reset(); MUST be called.
             *
             * BLE5-stack functionality will be compromised when LL_initRNGNoise
             * fails.
             */
            HAL_ASSERT_SPINLOCK;
            break;
        }

        default:
        {
            HAL_ASSERT_SPINLOCK;
            break;
        }
    }

    return;
}

/*******************************************************************************
 */
