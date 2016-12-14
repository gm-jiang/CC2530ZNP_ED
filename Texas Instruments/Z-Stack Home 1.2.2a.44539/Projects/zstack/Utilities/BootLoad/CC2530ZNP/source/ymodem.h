/**
  ******************************************************************************
  * @file    ymodem.h
  * @author  yanxianping
  * @version V1.0
  * @date    2016.5.13
  * @brief   This file provides all the software function headers of the ymodem.c
  *          file.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * FOR MORE INFORMATION PLEASE READ CAREFULLY THE LICENSE AGREEMENT FILE
  * LOCATED IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __YMODEM_H_
#define __YMODEM_H_

/*application address*/
#define YMODEM_APPLICATION_ADDRESS  HAL_SB_IMG_ADDR

/*timeout definitions*/
#define ACK_DELAY             (0x20000) // about 2 second
#define PACKET_TIMEOUT             (0x10000) // about 1 second
#define ENTER_UPGRADE_TIMEOUT 0x30000 // about 3 seconds
#define SPI_SEND_TIMEOUT 0x30000 // about 3 seconds
#define START_DOWNLOAD_TIMEOUT 0x12C0000 // about 5 minutes

/*Ymodem protocol macros*/
#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)
#define PACKET_HEADER           (3)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)
#define FILE_NAME_LENGTH        (256)
#define FILE_SIZE_LENGTH        (16)
#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* two of these in succession aborts transfer */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */
#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

/*upgrade result*/
#define  RESULT_FILENAME_EMPTY  1
#define  RESULT_FILESIZE_ERROR  2
#define  RESULT_USER_ABORT  3
#define  RESULT_SUCCESS  4
#define  RESULT_VERIFY_FAIL  5
#define  RESULT_TOO_MUCH_ERROR  6

/*string to int convertion macros*/
#define IS_AF(c)  ((c >= 'A') && (c <= 'F'))
#define IS_af(c)  ((c >= 'a') && (c <= 'f'))
#define IS_09(c)  ((c >= '0') && (c <= '9'))
#define ISVALIDHEX(c)  IS_AF(c) || IS_af(c) || IS_09(c)
#define ISVALIDDEC(c)  IS_09(c)
#define CONVERTDEC(c)  (c - '0')
#define CONVERTHEX_alpha(c)  (IS_AF(c) ? (c - 'A'+10) : (c - 'a'+10))
#define CONVERTHEX(c)   (IS_09(c) ? (c - '0') : CONVERTHEX_alpha(c))

#define MAX_ERRORS              (5)

void NP_RDY_Init(void);
void YModemSPIInit(void);
void YmodemOutputString(uint8 *s);
void YModemUpgrade(void);


#endif  /* __YMODEM_H_ */

/*******************(C)COPYRIGHT 2012 STMicroelectronics *****END OF FILE****/
