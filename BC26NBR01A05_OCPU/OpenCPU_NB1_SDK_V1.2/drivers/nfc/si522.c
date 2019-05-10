#include "si522.h"
#include "ql_gpio.h"
#include "ql_spi.h"
#include "ql_stdlib.h"
#include <stdint.h>
#include <string.h> 
#include "nfc.h"
#include "ql_uart.h"
#include "ql_trace.h"
#include "ql_uart.h"

static u32 spiChn = 1;
static uint8_t CT[2];
static uint8_t SN[6];
static u8 spi_type = 1;
static uint8_t nfc_tx_buf[MAXRLEN];

#define USR_SPI_CHANNAL	1

#define SI522_DEBUG(fmt, ...)	do{	\
	char buffer[64] = {0};											\
	Ql_sprintf(buffer, fmt, ##__VA_ARGS__);\	
	Ql_UART_Write(UART_PORT0, (u8*)(buffer), Ql_strlen((buffer)));\
	}while(0)
static uint16_t si522_crc(uint8_t *pdata, uint16_t len);
int8_t si522_communication(uint8_t Command, 
                 uint8_t *pInData, 
                 uint8_t InLenByte,
                 uint8_t *pOutData, 
                 u32  *pOutLenBit);

void si522_write(uint8_t addr, uint8_t value)
{	
	uint8_t buf[2] ;
  addr = (addr & 0x3f) << 1;   //code the first byte
  buf[0] = addr;
	buf[1] = value;
  Ql_SPI_Write(spiChn, buf, 2);
}

uint8_t si522_read(uint8_t addr)
{
  uint8_t value;
  addr = (addr & 0x3f)<<1 | 0x80;
	Ql_SPI_WriteRead(spiChn, &addr, 1, &value, 1);
  return value;
}

void si522_bit_set(uint8_t reg,uint8_t mask)  
{
  uint8_t tmp = 0x0;
  tmp = si522_read(reg);
  si522_write(reg, tmp | mask);  // set bit mask
}

void si522_bit_clear(uint8_t reg,uint8_t mask)  
{
  uint8_t tmp = 0x0;
  tmp = si522_read(reg);
  si522_write(reg, tmp & ~mask);  // clear bit mask
} 

static void spi_init(void)
{
	s32 ret;
	
	ret = Ql_SPI_Init(USR_SPI_CHANNAL,PINNAME_SPI_SCLK,PINNAME_SPI_MISO,PINNAME_SPI_MOSI,PINNAME_SPI_CS,spi_type);
	
	if(ret <0)
	{
//			APP_DEBUG("\r\n<-- Failed!! Ql_SPI_Init fail , ret =%d-->\r\n",ret)
	}
	else
	{
//			APP_DEBUG("\r\n<-- Ql_SPI_Init ret =%d -->\r\n",ret)	
	}
	ret = Ql_SPI_Config(USR_SPI_CHANNAL,1,0,0,30); //config sclk about 30kHz;
	if(ret <0)
	{
//			APP_DEBUG("\r\n<--Failed!! Ql_SPI_Config fail  ret=%d -->\r\n",ret)
	}
	else
	{
//			APP_DEBUG("\r\n<-- Ql_SPI_Config	=%d -->\r\n",ret)
	} 	
	
	
	if (!spi_type)
	{
		Ql_GPIO_Init(PINNAME_SPI_CS,PINDIRECTION_OUT,PINLEVEL_HIGH,PINPULLSEL_PULLUP);	 //CS high
	}

}

//=====================================================================================
//函数：Si522_Init()
//功能：配置Si522为读写器模式
//=====================================================================================
void si522_reset(void)
{
	Ql_GPIO_Init(PINNAME_GPIO4, PINDIRECTION_OUT, PINLEVEL_HIGH, PINPULLSEL_PULLUP);
	Ql_Sleep(1);
	Ql_GPIO_SetLevel(PINNAME_GPIO4, PINLEVEL_LOW);
	Ql_Sleep(1);
	Ql_GPIO_SetLevel(PINNAME_GPIO4, PINLEVEL_HIGH);

//  NZ3801_RST_LOW();
//  delay_us(1);
//
//  NZ3801_RST_HIGH();
//  delay_us(1);

  si522_write(CommandReg,PCD_RESETPHASE);
  si522_write(CommandReg,PCD_RESETPHASE);
//  delay_us(1);
	Ql_Sleep(1);

  si522_write(ModeReg,0x3D);            
  si522_write(TReloadRegL,30);
  si522_write(TReloadRegH,0);
  si522_write(TModeReg,0x8D);
  si522_write(TPrescalerReg,0x3E);
  si522_write(TxAutoReg,0x40);
	
  PcdAntennaOff();
	Ql_Sleep(1);
//  delay_us(1);
  PcdAntennaOn();

}
void si522_init(void)
{
	uint8_t count = 0;
	uint8_t value = 0;
//	spi_init();
//	si522_write(0x01, 0x0F);
//	value = si522_read(0x01);
//	si522_write(0x12, 0x00);
//	si522_write(0x13, 0x00);
//	si522_write(0x24, 0x26);
//	si522_write(0x2A, 0x80);			
//	si522_write(0x2B, 0xA9);		
//	si522_write(0x2C, 0x03);		
//	si522_write(0x2D, 0xE8);
//	si522_write(0x15, 0x40);		
//	si522_write(0x11, 0x3D);	
//
//	si522_write(0x26, 0x68);
//	si522_write(0x03, 0xc0);
//
//	value = si522_read(0x03);
//	value = si522_read(0x14);  // 00001 0100
//	if ((value & 0x03) != 0x03){
//		si522_write(0x14, value | 0x03);
//	}						
////////////////////////////////////////
//	Ql_UART_Write(UART_PORT0, "si522 init...\r\n", strlen("si522 init...\r\n"));

	si522_reset();

	si522_bit_clear(Status2Reg,0x08);
	si522_write(ModeReg,0x3D);
	si522_write(RxSelReg,0x86);
	si522_write(RFCfgReg,0x7F);
	si522_write(TReloadRegL,30);
	si522_write(TReloadRegH,0);
	si522_write(TModeReg,0x8D);
	si522_write(TPrescalerReg,0x3E);

}

//函  数 : Si522ACD_EdgeTriggerMode
//功  能 : 初始化并使能Si522的ACD的边沿触发模式
//入  参 : 无
//返回值 : 无
void si522_edge_trigger_mode(void)
{			
	si522_write(0x20, 0x00);	// 访问ACDConfigA
	si522_write(0x0f, 0x04);	// 设置定时唤醒寻卡时间间隔为500ms	
	si522_write(0x20, 0x01);	// 访问ACDConfigB
	si522_write(0x0f, 0x04);	// 0000 0100b，设置边沿触发模式，配置B为04h(VREFIN = 0.31875V)	
	si522_write(0x20, 0x03);	// 访问ACDConfigD
	si522_write(0x0f, 0x04);	// 0000 0100b，检测差值为4	
	si522_write(0x20, 0x04);	// 访问ACDConfigE
	si522_write(0x0f, 0xA7);	// 1010 0111b，配置E为A7h(打开运放，vr_s=0表明参考电压范围:0至0.31875V，时间间隔为7)                 		
	si522_write(0x02, 0x80);	// 置位ComIEnReg.IRqInv位
	si522_write(0x03, 0x40);	// 使能ACDIRq	 
	si522_write(0x03, 0xc0);	// 使能ACDIRq//1100 0000  		         
	si522_write(0x01, 0xB0);	// 使能ACD
}

//函  数 : Si522ACD_EdgeTriggerMode
//功  能 : 初始化并使能Si522的ACD的边沿触发模式
//入  参 : 无
//返回值 : 无
void si522_edge_trigger_with_timer(void)
{			
	si522_write(0x20, 0x00);	// 访问ACDConfigA
	si522_write(0x0f, 0x04);	// 设置定时唤醒寻卡时间间隔为500ms	
	si522_write(0x20, 0x01);	// 访问ACDConfigB
	si522_write(0x0f, 0x04);	// 0000 0100b，设置边沿触发模式，配置B为04h(VREFIN = 0.31875V)
	si522_write(0x20, 0x03);	// 访问ACDConfigD
	si522_write(0x0f, 0x04);	// 0000 0100b，检测差值为4	
	si522_write(0x20, 0x04);	// 访问ACDConfigE
	si522_write(0x0f, 0xA7);	// 1010 0111b，配置E为A7h(打开运放，vr_s=0表明参考电压范围:0至0.31875V，时间间隔为7)  
	si522_write(0x20, 0x07);	// 访问ACDConfigH
	si522_write(0x0f, 0x08);	// 8个定时唤醒周期		
	si522_write(0x02, 0x80);	// 置位ComIEnReg.IRqInv位
	si522_write(0x03, 0x60);	// 使能ACDIRq			
	si522_write(0x01, 0xB0);	// 使能ACD
}

int8_t PcdRequest(uint8_t req_code,uint8_t *pTagType)
{
	int8_t status;
	u32  len;
	
	si522_write(BitFramingReg,0x07);
	si522_bit_clear(Status2Reg,0x08);
	si522_bit_set(TxControlReg,0x03);

	nfc_tx_buf[0] = req_code;
	
	status = si522_communication(PCD_TRANSCEIVE, nfc_tx_buf, 1, nfc_tx_buf, &len);
	if ((status == NFC_SUCCESS) && (len == 0x10)){    
		*pTagType = nfc_tx_buf[0];
		*(pTagType+1) = nfc_tx_buf[1];
		SI522_DEBUG("card type:%x%x\r\n", nfc_tx_buf[1], nfc_tx_buf[0]);
	}
	
	return status;
}

int8_t PcdAnticoll(uint8_t *pSnr)
{
	int8_t  status;
  uint8_t i,snr_check=0;
  u32  len;
	
  si522_bit_clear(Status2Reg,0x08);
  si522_write(BitFramingReg,0x00);
  si522_bit_clear(CollReg,0x80);

  nfc_tx_buf[0] = PICC_ANTICOLL1;
  nfc_tx_buf[1] = 0x20;

  status = si522_communication(PCD_TRANSCEIVE,nfc_tx_buf,2,nfc_tx_buf,&len);

  if (status == NFC_SUCCESS){
		 for (i=0; i<4; i++){   
       *(pSnr+i)  = nfc_tx_buf[i];
       snr_check ^= nfc_tx_buf[i];
	   }
	   if (snr_check != nfc_tx_buf[i]){   
	     status = NFC_FAIL;    
		 }
		 SI522_DEBUG("card sn:%x%x%x%x\r\n", pSnr[0], pSnr[1], pSnr[2], pSnr[3]);
  }
  
  si522_bit_set(CollReg,0x80);
  return status;
}

int8_t PcdSelect(uint8_t *pSnr)
{
  uint8_t i;
  u32  len;
 int8_t status;
  uint16_t *pcrc = (uint16_t *)&nfc_tx_buf[7];
  nfc_tx_buf[0] = PICC_ANTICOLL1;
  nfc_tx_buf[1] = 0x70;
  nfc_tx_buf[6] = 0;
  for (i=0; i<4; i++){
  	nfc_tx_buf[i+2] = *(pSnr+i);
  	nfc_tx_buf[6]  ^= *(pSnr+i);
  }
  *pcrc = si522_crc(nfc_tx_buf,7);

  si522_bit_clear(Status2Reg,0x08);

  status = si522_communication(PCD_TRANSCEIVE, nfc_tx_buf, 9, nfc_tx_buf, &len);
	status = (len == 0x18 ? status : NFC_FAIL);
	SI522_DEBUG("%s\t%d status:%d len:%d\r\n", __func__, __LINE__, status, len);

  return status;
}

 int8_t si522_card_search(uint8_t *puid)
{
	int8_t status = NFC_ERR;

	status = PcdRequest(PICC_REQALL,CT);
	if(status == NFC_SUCCESS){
		status = PcdAnticoll(SN);
		if(status != NFC_SUCCESS){
			return status;
		}
		status = PcdSelect(SN);
		if(status == NFC_SUCCESS){
			memcpy(puid, SN, 6);
		}
	}

	return status;
}
static int8_t si522_card_authent(uint8_t *puid, uint8_t key_type, uint8_t *key,  uint8_t block_nr)
{
  uint8_t   len;
	int8_t status = NFC_SUCCESS;

  nfc_tx_buf[0] = 0x60;
  nfc_tx_buf[1] = block_nr;

  memcpy(&nfc_tx_buf[2], key, 6);
  memcpy(&nfc_tx_buf[8], puid, 4);

  status = si522_communication(PCD_AUTHENT, nfc_tx_buf, 12, nfc_tx_buf, &len);
  if ((status != NFC_SUCCESS) || (!(si522_read(Status2Reg) & 0x08))){
    status = NFC_FAIL;
  }
  return status;
}

static int8_t si522_card_block_write(const void *pbuf, uint8_t block_nr)
{
  int8_t   status = NFC_ERR;
  uint8_t   len;
  uint8_t   i;
	uint16_t *pcrc = (uint16_t *)&nfc_tx_buf[2];

  nfc_tx_buf[0] = PICC_WRITE;
  nfc_tx_buf[1] = block_nr;
  *pcrc = si522_crc(nfc_tx_buf, 2);

  status = si522_communication(PCD_TRANSCEIVE,nfc_tx_buf,4,nfc_tx_buf,&len);

  if ((status != NFC_SUCCESS) || (len != 4) || ((nfc_tx_buf[0] & 0x0F) != 0x0A)){
    status = NFC_FAIL;
  }
  if (status == NFC_SUCCESS){
    for (i = 0; i < 16; i ++){
      nfc_tx_buf[i] = *((char *)pbuf +i);
    }
		pcrc = (uint16_t *)&nfc_tx_buf[16];
    *pcrc = si522_crc(nfc_tx_buf, 16);

    status = si522_communication(PCD_TRANSCEIVE,nfc_tx_buf,18,nfc_tx_buf,&len);
    if ((status != NFC_SUCCESS) || (len != 4) || ((nfc_tx_buf[0] & 0x0F) != 0x0A)){
      status = NFC_FAIL;
    }
  }
  return status;

}

static int8_t si522_card_block_read(void *pbuf, uint8_t block_nr)
{
  uint8_t   len;
  int8_t   status = NFC_FAIL;
	uint16_t *pcrc = (uint16_t *)&nfc_tx_buf[2];

  nfc_tx_buf[0] = PICC_READ;
  nfc_tx_buf[1] = block_nr;
  *pcrc = si522_crc(nfc_tx_buf, 2);

  status = si522_communication(PCD_TRANSCEIVE,nfc_tx_buf,4,nfc_tx_buf,&len);
  if ((status == NFC_SUCCESS) && (len == 0x90)){
		memcpy(pbuf, nfc_tx_buf, 16);
  }else{
    status = NFC_FAIL;
  }
  return status;	
}

static uint16_t si522_crc(uint8_t *pdata, uint16_t len)
{
	uint16_t crc = 0x00;
	uint8_t 	i,n;

	si522_bit_clear(DivIrqReg, 0x04);
	si522_write(CommandReg, PCD_IDLE);
	si522_bit_set(FIFOLevelReg, 0x80);
	for (i=0; i<len; i++){
		si522_write(FIFODataReg, *(pdata +i));
	}
	si522_write(CommandReg, PCD_CALCCRC);
	i = 0xFF;
	do{
		n = si522_read(DivIrqReg);
	}while (( --i != 0) && !( n & 0x04));
	crc = si522_read(CRCResultRegL);
	crc |= (si522_read(CRCResultRegM) << 8);
	return crc;
}

int8_t PcdReset(void)
{

	si522_write(CommandReg,PCD_RESETPHASE);
//	delay_ms(10);

	si522_write(ModeReg,0x3D);    // 和Mifare卡通讯，CRC初始值0x6363
	si522_write(TReloadRegL,30);           
	si522_write(TReloadRegH,0);
	si522_write(TModeReg,0x8D);
	si522_write(TPrescalerReg,0x3E);	
	
	si522_write(TxAutoReg,0x40);  // 不可少
	
  return NFC_SUCCESS;
}

int8_t PcdConfigISOType(uint8_t type)
{
   if (type == 'A'){ 
		si522_bit_clear(Status2Reg,0x08);
		si522_write(ModeReg,0x3D);//3F
		si522_write(RxSelReg,0x86);//84

		si522_write(RFCfgReg,0x78);   //4F

		si522_write(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
		si522_write(TReloadRegH,0);
		si522_write(TModeReg,0x8D);
		si522_write(TPrescalerReg,0x3E);

//		delay_ms(10);
		PcdAntennaOn();
   }else{ 
		return 1;
	 }	//有改动
   
   return NFC_SUCCESS;
}

int8_t si522_communication(uint8_t Command, 
                 uint8_t *pInData, 
                 uint8_t InLenByte,
                 uint8_t *pOutData, 
                 u32  *pOutLenBit)
{
  int8_t status = NFC_ERR;
  uint8_t irqEn   = 0x00;
  uint8_t waitFor = 0x00;
  uint8_t lastBits;
  uint8_t n;
  u32 i;
  switch (Command){
   case PCD_AUTHENT:
    irqEn   = 0x12;
    waitFor = 0x10;
    break;
   case PCD_TRANSCEIVE:
    irqEn   = 0x77;
    waitFor = 0x30;
    break;
   default:
		break;
  }
 
  si522_write(ComIEnReg, irqEn | 0x80);
  si522_bit_clear(ComIrqReg, 0x80);
  si522_write(CommandReg, PCD_IDLE);
  si522_bit_set(FIFOLevelReg, 0x80);
  
  for (i=0; i<InLenByte; i++){   
    si522_write(FIFODataReg, pInData[i]);  
	}
  si522_write(CommandReg, Command);
 
  if (Command == PCD_TRANSCEIVE){    
    si522_bit_set(BitFramingReg,0x80);  
	}
  
  i = 5000;//根据时钟频率调整，操作M1卡最大等待时间25ms
  do {
     n = si522_read(ComIrqReg);
  }while ((-- i != 0) && !(n & 0x01) && !(n & waitFor));
	
  si522_bit_clear(BitFramingReg,0x80);
  if (i != 0){    
   if(!(si522_read(ErrorReg) & 0x1B))  {
     status = NFC_SUCCESS;
     if (n & irqEn & 0x01){  
		 	status = NFC_FAIL;   
		 }
     if (Command == PCD_TRANSCEIVE){
     	n = si522_read(FIFOLevelReg);
    	lastBits = si522_read(ControlReg) & 0x07;
      if (lastBits){   
      	*pOutLenBit = (n-1)*8 + lastBits;  
			}
      else{   
				*pOutLenBit = n*8;
			}
			n = n ? n : 1;
			n = n > MAXRLEN ? MAXRLEN : n;
      for (i=0; i<n; i++){   
        pOutData[i] = si522_read(FIFODataReg);
			}
    }
   }
 }
 
 si522_bit_set(ControlReg, 0x80);           // stop timer now
 si522_write(CommandReg, PCD_IDLE); 
 return status;
}

void PcdAntennaOn()
{
  uint8_t i;
  i = si522_read(TxControlReg);
  if (!(i & 0x03)){
    si522_bit_set(TxControlReg, 0x03);
  }
}

void PcdAntennaOff()
{
  si522_bit_clear(TxControlReg, 0x03);
}

nfc_ops_t si522 = {
	si522_init,
	si522_card_search,
	si522_card_authent,
	si522_card_block_write,
	si522_card_block_read,
	NULL,
};


