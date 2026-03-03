/******************************************************************************

@file  app_simple_gatt.c

@brief This file contains the Simple GATT application functionality

Group: WCS, BTS
Target Device: cc23xx

******************************************************************************

 Copyright (c) 2022-2026, Texas Instruments Incorporated
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

//*****************************************************************************
//! Includes
//*****************************************************************************
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <FreeRTOS.h>
#include <timers.h>
#include "ti/ble/app_util/framework/bleapputil_api.h"
#include <ti/ble/profiles/simple_gatt/simple_gatt_profile.h>
#include "ti/ble/app_util/menu/menu_module.h"
#include <app_main.h>

//*****************************************************************************
//! Defines
//*****************************************************************************
#define SIMPLE_GATT_NOTIFY_DEFAULT_PERIOD_MS      (200U)
#define SIMPLE_GATT_NOTIFY_PERIOD_MIN_MS          (50U)
#define SIMPLE_GATT_NOTIFY_PERIOD_MAX_MS          (2000U)
#define SIMPLE_GATT_NOTIFY_PERIOD_UNIT_MS         (10U)

#define SIMPLE_GATT_CMD_TARE                      (0x01U)
#define SIMPLE_GATT_CMD_ZERO                      (0x02U)
#define SIMPLE_GATT_CMD_SET_PERIOD                (0x10U)

#define SIMPLE_GATT_WEIGHT_STEP_G                 (5)

//*****************************************************************************
//! Globals
//*****************************************************************************

static void SimpleGatt_changeCB( uint8_t paramId );
static void SimpleGatt_notifyChar4(void);
static void SimpleGatt_weightNotifyTimerCB(TimerHandle_t xTimer);
static void SimpleGatt_processWriteCmd(const uint8_t *cmdBuf, uint8_t cmdLen);
static void SimpleGatt_setNotifyPeriodMs(uint16_t periodMs);

static TimerHandle_t simpleGatt_weightNotifyTimer = NULL;
static int32_t simpleGatt_weightG = 0;
static int32_t simpleGatt_tareOffsetG = 0;
static uint16_t simpleGatt_notifyPeriodMs = SIMPLE_GATT_NOTIFY_DEFAULT_PERIOD_MS;
static bool simpleGatt_waitingPeriodValue = false;

// Simple GATT Profile Callbacks
static SimpleGattProfile_CBs_t simpleGatt_profileCBs =
{
  SimpleGatt_changeCB // Simple GATT Characteristic value change callback
};

//*****************************************************************************
//! Functions
//*****************************************************************************

/*********************************************************************
 * @fn      SimpleGatt_ChangeCB
 *
 * @brief   Callback from Simple Profile indicating a characteristic
 *          value change.
 *
 * @param   paramId - parameter Id of the value that was changed.
 *
 * @return  None.
 */
static void SimpleGatt_changeCB( uint8_t paramId )
{
  uint8_t newValue = 0;

  switch( paramId )
  {
    case SIMPLEGATTPROFILE_CHAR1:
      {
        SimpleGattProfile_getParameter( SIMPLEGATTPROFILE_CHAR1, &newValue );

        // Print the new value of char 1
        MenuModule_printf(APP_MENU_PROFILE_STATUS_LINE, 0, "Profile status: Simple profile - "
                          "Char 1 value = " MENU_MODULE_COLOR_YELLOW "%d " MENU_MODULE_COLOR_RESET,
                          newValue);
      }
      break;

    case SIMPLEGATTPROFILE_CHAR3:
      {
        uint8_t cmdBuf[2] = {0};

        if(SimpleGattProfile_getParameter(SIMPLEGATTPROFILE_CHAR3, cmdBuf) == SUCCESS)
        {
          newValue = cmdBuf[0];

          // Print the new value of char 3
          MenuModule_printf(APP_MENU_PROFILE_STATUS_LINE, 0, "Profile status: Simple profile - "
                            "Char 3 cmd = " MENU_MODULE_COLOR_YELLOW "0x%02X " MENU_MODULE_COLOR_RESET,
                            newValue);

          SimpleGatt_processWriteCmd(cmdBuf, sizeof(cmdBuf));
        }
      }
      break;
    case SIMPLEGATTPROFILE_CHAR4:
      {
          // Print Notification registration to user
          MenuModule_printf(APP_MENU_PROFILE_STATUS_LINE, 0, "Profile status: Simple profile - "
                                    "Char 4 = Notification registration");

          SimpleGatt_notifyChar4();
          break;
      }
    default:
      // should not reach here!
      break;
  }
}

/*********************************************************************
 * @fn      SimpleGatt_start
 *
 * @brief   This function is called after stack initialization,
 *          the purpose of this function is to initialize and
 *          register the Simple GATT profile.
 *
 * @return  SUCCESS or stack call status
 */
bStatus_t SimpleGatt_start( void )
{
  bStatus_t status = SUCCESS;

  // Add Simple GATT service
  status = SimpleGattProfile_addService();
  if (status != SUCCESS)
  {
	// Return status value
    return(status);
  }

  // Setup the Simple GATT Characteristic Values
  // For more information, see the GATT and GATTServApp sections in the User's Guide:
  // All the documentation and collateral applicable can be found on TI Developer Zone - https://dev.ti.com/
  {
    uint8_t charValue1 = 1;
    uint8_t charValue2 = 2;
    uint8_t charValue3 = 3;
    uint8_t charValue4 = 4;
    uint8_t charValue5[SIMPLEGATTPROFILE_CHAR5_LEN] = { 1, 2, 3, 4, 5 };

    SimpleGattProfile_setParameter( SIMPLEGATTPROFILE_CHAR1, sizeof(uint8_t),
                                    &charValue1 );
    SimpleGattProfile_setParameter( SIMPLEGATTPROFILE_CHAR2, sizeof(uint8_t),
                                    &charValue2 );
    SimpleGattProfile_setParameter( SIMPLEGATTPROFILE_CHAR3, sizeof(uint8_t),
                                    &charValue3 );
    SimpleGattProfile_setParameter( SIMPLEGATTPROFILE_CHAR4, sizeof(uint8_t),
                                    &charValue4 );
    SimpleGattProfile_setParameter( SIMPLEGATTPROFILE_CHAR5, SIMPLEGATTPROFILE_CHAR5_LEN,
                                    charValue5 );
  }
  // Register callback with SimpleGATTprofile
  status = SimpleGattProfile_registerAppCBs( &simpleGatt_profileCBs );

  if((status == SUCCESS) && (simpleGatt_weightNotifyTimer == NULL))
  {
    simpleGatt_weightNotifyTimer = xTimerCreate("SgWtNtf",
                                                pdMS_TO_TICKS(simpleGatt_notifyPeriodMs),
                                                pdTRUE,
                                                NULL,
                                                SimpleGatt_weightNotifyTimerCB);

    if(simpleGatt_weightNotifyTimer != NULL)
    {
      if(xTimerStart(simpleGatt_weightNotifyTimer, 0) != pdPASS)
      {
        status = FAILURE;
      }
    }
    else
    {
      status = FAILURE;
    }
  }

  // Return status value
  return(status);
}

/*********************************************************************
 * @fn      SimpleGatt_notifyChar4
 *
 * @brief   This function is called when WriteReq has been received to Char 4 or to Char 3.
 *          The purpose of this function is to send notification of Char 3 with the value
 *          of Char 3.
 *
 * @return  void
 */
static void SimpleGatt_notifyChar4(void)
{
  int32_t reportedWeightG = simpleGatt_weightG - simpleGatt_tareOffsetG;
  uint8_t payload[sizeof(int32_t)] = {0};

  payload[0] = (uint8_t)(reportedWeightG & 0xFF);
  payload[1] = (uint8_t)((reportedWeightG >> 8) & 0xFF);
  payload[2] = (uint8_t)((reportedWeightG >> 16) & 0xFF);
  payload[3] = (uint8_t)((reportedWeightG >> 24) & 0xFF);

  // Only attempt notification while connected.
  if(linkDB_NumActive() > 0)
  {
    // Char 4 value update triggers notify when CCCD is enabled.
    (void)SimpleGattProfile_setParameter(SIMPLEGATTPROFILE_CHAR4, sizeof(payload), payload);
  }
}

static void SimpleGatt_weightNotifyTimerCB(TimerHandle_t xTimer)
{
  (void)xTimer;

  if(linkDB_NumActive() == 0)
  {
    return;
  }

  simpleGatt_weightG += SIMPLE_GATT_WEIGHT_STEP_G;
  if(simpleGatt_weightG > 200000)
  {
    simpleGatt_weightG = 0;
  }

  SimpleGatt_notifyChar4();
}

static void SimpleGatt_setNotifyPeriodMs(uint16_t periodMs)
{
  if(simpleGatt_weightNotifyTimer != NULL)
  {
    simpleGatt_notifyPeriodMs = periodMs;
    (void)xTimerChangePeriod(simpleGatt_weightNotifyTimer,
                             pdMS_TO_TICKS(simpleGatt_notifyPeriodMs),
                             0);
  }
}

static void SimpleGatt_processWriteCmd(const uint8_t *cmdBuf, uint8_t cmdLen)
{
  uint8_t cmd;

  if((cmdLen == 0U) || (cmdBuf == NULL))
  {
    return;
  }

  cmd = cmdBuf[0];

  if(simpleGatt_waitingPeriodValue)
  {
    uint16_t periodMs = (uint16_t)cmd * SIMPLE_GATT_NOTIFY_PERIOD_UNIT_MS;
    simpleGatt_waitingPeriodValue = false;

    if((periodMs >= SIMPLE_GATT_NOTIFY_PERIOD_MIN_MS) &&
       (periodMs <= SIMPLE_GATT_NOTIFY_PERIOD_MAX_MS))
    {
      SimpleGatt_setNotifyPeriodMs(periodMs);
    }
    return;
  }

  switch(cmd)
  {
    case SIMPLE_GATT_CMD_TARE:
      simpleGatt_tareOffsetG = simpleGatt_weightG;
      break;

    case SIMPLE_GATT_CMD_ZERO:
      simpleGatt_weightG = 0;
      simpleGatt_tareOffsetG = 0;
      break;

    case SIMPLE_GATT_CMD_SET_PERIOD:
      if(cmdLen >= 2U)
      {
        uint16_t periodMs = (uint16_t)cmdBuf[1] * SIMPLE_GATT_NOTIFY_PERIOD_UNIT_MS;
        if((periodMs >= SIMPLE_GATT_NOTIFY_PERIOD_MIN_MS) &&
           (periodMs <= SIMPLE_GATT_NOTIFY_PERIOD_MAX_MS))
        {
          SimpleGatt_setNotifyPeriodMs(periodMs);
        }
      }
      else
      {
        simpleGatt_waitingPeriodValue = true;
      }
      break;

    default:
      // Unknown command. Ignore safely.
      break;
  }
}
