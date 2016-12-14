/**
  ******************************************************************************
  * @file    ymodem.c
  * @author  yanxianping
  * @version V1.0
  * @date    2016.5.13
  * @brief   This file provides all the software functions related to the ymodem
  *          protocol.
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


/*includes*/
#include "hal_board_cfg.h"
#include "hal_types.h"
#include "hal_dma.h"
#include "hal_flash.h"
#include "_hal_uart_spi.c"
#include "sb_main.h"

#include "ymodem.h"
#include "string.h"

/*global variables*/
uint8 packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
uint8 file_size[FILE_SIZE_LENGTH];
uint8 FileName[FILE_NAME_LENGTH];
halDMADesc_t dmaCh0 = {0};

static uint32 Str2Int(uint8 *inputstr, int32 *intnum)
{
  uint32 i = 0, res = 0;
  uint32 val = 0;

  if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X'))
  {
    if (inputstr[2] == '\0')
    {
      return 0;
    }
    for (i = 2; i < 11; i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1; */
        res = 1;
        break;
      }
      if (ISVALIDHEX(inputstr[i]))
      {
        val = (val << 4) + CONVERTHEX(inputstr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 8 digit hex --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }
  else /* max 10-digit decimal input */
  {
    for (i = 0;i < 11;i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1 */
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0))
      {
        val = val << 10;
        *intnum = val;
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0))
      {
        val = val << 20;
        *intnum = val;
        res = 1;
        break;
      }
      else if (ISVALIDDEC(inputstr[i]))
      {
        val = val * 10 + CONVERTDEC(inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 10 digit decimal --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }

  return res;
}


static int32 YmodemReceiveBytes (uint8 *buff, uint32 len, uint32 timeout)
{
  return HalUARTReadSPI(buff, len, timeout);
}

static void YmodemSendByte (uint8 data)
{
  HalUARTWriteSPI(&data, 1, START_DOWNLOAD_TIMEOUT);
  return ;
}

static void YmodemSendBytes (uint8 *buff, uint32 len)
{
  HalUARTWriteSPI(buff, len, START_DOWNLOAD_TIMEOUT);
  return ;
}

void NP_RDY_Init(void)
{
  // Select general purpose on I/O pins.
  P0SEL &= ~(NP_RDYIn_BIT);      // P0.3 MRDY - GPIO
  P0SEL &= ~(NP_RDYOut_BIT);     // P0.4 SRDY - GPIO

  // Select GPIO direction.
  P0DIR &= ~NP_RDYIn_BIT;        // P0.3 MRDY - IN
  P0DIR |= NP_RDYOut_BIT;        // P0.4 SRDY - OUT

  /*set P0.3 Pullup*/
  P0INP &= ~NP_RDYIn_BIT;
  P2INP &= ~BV(5);

  SRDY_CLR();
}

void YModemSPIInit(void)
{
  HAL_DMA_SET_ADDR_DESC0(&dmaCh0);
  NP_RDY_Init();
  HalUARTInitSPI();
  return ;
}

static int YmodemSaveImage(uint32 address, uint8 *data, uint32 len)
{
  if ((address % HAL_FLASH_PAGE_SIZE) == 0)
  {
    HalFlashErase(address / HAL_FLASH_PAGE_SIZE);
  }

  HalFlashWrite(address / HAL_FLASH_WORD_SIZE, data, len / HAL_FLASH_WORD_SIZE);

  return 0;
}

void YmodemOutputString(uint8 *s)
{
  YmodemSendBytes(s, strlen((const char*)s));
  return ;
}

static uint16 YmodemCalCRC(uint8 *buf, uint32 len)
{
  uint16 mCrc = 0;
  uint32 i = 0, j = 0;

  for(j = 0; j < len; j++)
  {
      mCrc = mCrc^(((uint16)buf[j]) << 8);
      for (i=8; i!=0; i--)
      {
          if (mCrc & 0x8000)//MSB is 1?
              mCrc = (mCrc << 1) ^ 0x1021;
          else
              mCrc = mCrc << 1;
      }
  }
  return mCrc;
}

static int32 YmodemDownloading (void)
{
  int32 i = 0;
  uint8 ch = 0;
  uint8 *file_ptr = NULL;
  uint16 packet_size = 0;
  uint16 crc_source = 0;
  uint16 crc_cal = 0;
  int32 filesize = 0;
  int32 packets_received = 0;
  int32 errors = 0;
  uint8 imgInfoAck[] = {0x06,0x43};
  uint32 time = ACK_DELAY;
  static uint32 flashdestination = YMODEM_APPLICATION_ADDRESS;

  while (1)
  {
    if (errors > MAX_ERRORS)
    {
      YmodemSendByte(CA);
      YmodemSendByte(CA);
      return RESULT_TOO_MUCH_ERROR;
    }

    if (YmodemReceiveBytes(&ch, 1, START_DOWNLOAD_TIMEOUT) != 1)
    {
      YmodemSendByte(CRC16);
      errors++;
      continue;
    }


    switch (ch)
    {
      case SOH:
      case STX:
        packet_size = (ch == SOH) ? PACKET_SIZE : PACKET_1K_SIZE;
        *packet_data = ch;

        /*receive a packet*/
        if (YmodemReceiveBytes(packet_data + 1, packet_size + PACKET_OVERHEAD - 1, START_DOWNLOAD_TIMEOUT) != packet_size + PACKET_OVERHEAD - 1)
        {
          YmodemSendByte(CRC16);
          errors++;
          break ;
        }

        /*packet number check*/
        if ((packet_data[PACKET_SEQNO_INDEX] != (packet_data[PACKET_SEQNO_COMP_INDEX] ^ 0xff)&0xff)
          || (packet_data[PACKET_SEQNO_INDEX]  != packets_received))
        {
          YmodemSendByte(CRC16);
          errors++;
          break ;
        }

        /*crc check*/
        crc_source = packet_data[packet_size + PACKET_OVERHEAD - 2];
        crc_source = crc_source << 8;
        crc_source |= packet_data[packet_size + PACKET_OVERHEAD - 1];
        crc_cal = YmodemCalCRC(&packet_data[PACKET_HEADER], packet_size);
        if (crc_source != crc_cal)
        {
          YmodemSendByte(CRC16);
          errors++;
          break ;
        }

        if (packets_received == 0) /* first packet  contain file information*/
        {
          if (packet_data[PACKET_HEADER] == 0) /* Filename packet is empty, end session */
          {
            YmodemSendByte(CA);
            YmodemSendByte(CA);
            return RESULT_FILENAME_EMPTY;
          }

          /* Filename packet has valid data */
          for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
          {
            FileName[i++] = *file_ptr++;
          }
          FileName[i++] = '\0';
          for (i = 0, file_ptr ++; (*file_ptr != 0) && (i < FILE_SIZE_LENGTH);)
          {
            file_size[i++] = *file_ptr++;
          }
          file_size[i++] = '\0';
          Str2Int(file_size, &filesize);

          /* Test the size of the image to be sent */
          if (filesize > HAL_SB_IMG_SIZE)
          {
            /* End session */
            YmodemSendByte(CA);
            YmodemSendByte(CA);
            return RESULT_FILESIZE_ERROR;
          }
          while(time--);
          YmodemSendBytes(imgInfoAck, 2);
        }
        else  /* Data packet */
        {
          /* save image data */
          if (YmodemSaveImage(flashdestination, packet_data + PACKET_HEADER, packet_size) == -1)
          {
            YmodemSendByte(CRC16);
            errors++;
            break ;
          }
          else
          {
            /*crc check*/
            HalFlashRead(flashdestination / HAL_FLASH_PAGE_SIZE, flashdestination % HAL_FLASH_PAGE_SIZE, packet_data, packet_size);
            crc_cal = YmodemCalCRC(packet_data, packet_size);
            if (crc_source == crc_cal)
            {
              flashdestination += packet_size;
              YmodemSendByte(ACK);
            }
            else
            {
              YmodemSendByte(CRC16);
              errors++;
              break ;
            }
          }
        }
        packets_received++;
        break;

      case EOT:
        YmodemSendByte(ACK);
        return RESULT_SUCCESS;
        break;

      case CA:
        if ((YmodemReceiveBytes(&ch, 1, PACKET_TIMEOUT) == 1) && (ch == CA))
        {
          YmodemSendByte(ACK);
          return RESULT_USER_ABORT;
        }
        else
        {
          YmodemSendByte(CRC16);
          break;
        }

      case ABORT1:
      case ABORT2:
        YmodemSendByte(CA);
        YmodemSendByte(CA);
        return RESULT_USER_ABORT;

      default:
      {
          YmodemSendByte(CRC16);
          break;
      }
    }
  }
}


static void YModemStartDownload(void)
{
  int32 result = 0;
  uint32 time = ACK_DELAY;
  while(time--);
  YmodemSendByte(ACK);
  //Receiving the image file ...
  result = YmodemDownloading ();
  switch (result)
  {
    case RESULT_FILENAME_EMPTY:
      break;
    case RESULT_FILESIZE_ERROR:
      break;
    case RESULT_USER_ABORT:
      break;
    case RESULT_TOO_MUCH_ERROR:
      break;
    case RESULT_SUCCESS:
      //YmodemOutputString("\r\nUpgrade successfully.");
      //YmodemOutputString("\r\nImage filename: ");
      //YmodemOutputString(FileName);
      //YmodemOutputString("\r\nImage size: ");
      //YmodemOutputString(file_size);
      //YmodemOutputString(" Bytes.\r\n");
      break;
    case RESULT_VERIFY_FAIL:
      break;
    default:
      break;
  }

  return ;
}


static void YModemEnterUpgrade(void)
{
  uint8 key = 0;

  while (1)
  {
    /* '1' start downloading image */
    if (YmodemReceiveBytes (&key, 1, START_DOWNLOAD_TIMEOUT) == 1 && key == '1')
    {
      YModemStartDownload();
      return ;
    }
  }
}

void YModemUpgrade(void)
{
  uint8 cmd_data[2]={0};

  if(YmodemReceiveBytes(cmd_data, 2, ENTER_UPGRADE_TIMEOUT) == 2)
  {
    if(cmd_data[0]==0x41 && cmd_data[1]==0x35)
    {
        YModemEnterUpgrade ();
    }
  }
  return ;
}
