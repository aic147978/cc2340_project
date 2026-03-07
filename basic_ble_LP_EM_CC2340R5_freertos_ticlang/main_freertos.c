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

#include "drivers/hx711.h"
#include <ti/display/Display.h>
#include "ti_drivers_config.h"
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
static Display_Handle gDisplayHandle = NULL;
static HX711_Handle_t gHx711;
static TaskHandle_t hx711TaskHandle = NULL;
/*******************************************************************************
 * GLOBAL VARIABLES
 */

/*******************************************************************************
 * EXTERNS
 */
 static void HX711Task(void *pvParameters);
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
int main()
{
  /* Register Application callback to trap asserts raised in the Stack */
  halAssertCback = AssertHandler;
  RegisterAssertCback(AssertHandler);

  Board_init();

  /* Update User Configuration of the stack */
  user0Cfg.appServiceInfo->timerTickPeriod = ICall_getTickPeriod();
  user0Cfg.appServiceInfo->timerMaxMillisecond  = ICall_getMaxMSecs();
 Display_printf(gDisplayHandle, 0, 0, "boardboard start");
  /* Initialize all applications tasks */
  appMain();
  xTaskCreate(HX711Task,
              "HX711Task",
              1024,
              NULL,
              1,
              &hx711TaskHandle);
  /* Start the FreeRTOS scheduler */
  vTaskStartScheduler();

  return 0;

}

static void HX711Task(void *pvParameters)
{
    int32_t raw;

    (void)pvParameters;

    /* 初始化 Display */
    Display_init();
    gDisplayHandle = Display_open(CONFIG_Display_0, NULL);

    if (gDisplayHandle == NULL)
    {
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    Display_printf(gDisplayHandle, 0, 0, "HX711 task start");

    /* 初始化 HX711 */
    HX711_init(&gHx711);

    /* 等待 HX711 就绪 */
    if (!HX711_waitReady(1000000))
    {
        Display_printf(gDisplayHandle, 0, 0, "HX711 not ready");
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    Display_printf(gDisplayHandle, 0, 0, "HX711 ready");

    while (1)
    {
        /* 先只读原始值，不做重量计算 */
        raw = HX711_readAverage(5);

        Display_printf(gDisplayHandle, 0, 0, "raw = %ld", raw);

        /* 500ms 打印一次 */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
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
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    //Handle FreeRTOS Stack Overflow
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
void AssertHandler(uint8_t assertCause, uint8_t assertSubcause)
{
    // check the assert cause
    switch(assertCause)
    {
        case HAL_ASSERT_CAUSE_OUT_OF_MEMORY:
        {
            // ERROR: OUT OF MEMORY
            HAL_ASSERT_SPINLOCK;
            break;
        }

        case HAL_ASSERT_CAUSE_INTERNAL_ERROR:
        {
            // check the subcause
            if(assertSubcause == HAL_ASSERT_SUBCAUSE_FW_INERNAL_ERROR)
            {
                // ERROR: INTERNAL FW ERROR
                HAL_ASSERT_SPINLOCK;
            }
            else
            {
                // ERROR: INTERNAL ERROR
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
