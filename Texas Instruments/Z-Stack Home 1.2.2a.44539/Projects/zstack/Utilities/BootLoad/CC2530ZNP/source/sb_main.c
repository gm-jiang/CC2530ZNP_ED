/**************************************************************************************************
  Filename:       sb_main.c
  Revised:        $Date: 2014-11-19 13:29:24 -0800 (Wed, 19 Nov 2014) $
  Revision:       $Revision: 41175 $

  Description:    This module contains the main functionality of a Boot Loader for CC2530ZNP.
                  It is a minimal subset of functionality from ZMain.c, OnBoard.c and various
                  _hal_X.c modules for the CC2530ZNP build.


  Copyright 2010-2014 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_board_cfg.h"
#include "hal_adc.h"
#include "hal_types.h"
#include "sb_main.h"
#include "ymodem.h"
/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */


/* ------------------------------------------------------------------------------------------------
 *                                       Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */

/* ISR's implemented in the boot loader must be able to quickly determine whether to jump to the
 * boot code handlers or the run code handlers. So mark the bottom of the C call stack space with
 * a special value. Since the boot code linker file starts the C call stack space one byte higher
 * than the run code, the IAR generated initialization code does not initialize this byte;
 * but the boot code does - marking it with the magic value.
 */

/* ------------------------------------------------------------------------------------------------
 *                                       Local Functions
 * ------------------------------------------------------------------------------------------------
 */

void vddWait(uint8 vdd);



/**************************************************************************************************
 * @fn          vddWait
 *
 * @brief       Loop waiting for 16 reads of the Vdd over the requested limit.
 *
 * input parameters
 *
 * @param       vdd - Vdd level to wait for.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void vddWait(uint8 vdd)
{
  static uint8 max_verified_voltage = 0;
  uint8 cnt = 16;

  if (vdd <= max_verified_voltage)
  {
    return;
  }
  
  do {
    do {
      ADCCON3 = 0x0F;
      while (!(ADCCON1 & 0x80));
    } while (ADCH < vdd);
  } while (--cnt);

  max_verified_voltage = vdd;
}

/**************************************************************************************************
 * @fn          main
 *
 * @brief       ISR for the reset vector.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */

void main(void)
{
  vddWait(VDD_MIN_NV);
  HAL_BOARD_INIT();

  YModemSPIInit();

  YModemUpgrade();

  asm("LJMP 0x2000\n");

  HAL_SYSTEM_RESET();
}
/**************************************************************************************************
*/
