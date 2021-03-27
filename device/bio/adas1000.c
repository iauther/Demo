#include "ADAS1000.h"
#include "spi.h"
#include "gpio.h"


U32 frameSize 		 = 0; //ADAS1000 frame size in bytes
static U32 wordSize    = 4;
static U32 frameRate 		 = 0; //ADAS1000 frame rate
static U32 inactiveWordsNo = 0; //number of inactive words in a frame

static int SPI_Init(U8 lsb, U32 freq, U8 cpol, U8 cpha)
{
	SPI_InitTypeDef si;
    
    si.Mode = SPI_MODE_MASTER;
    si.Direction = SPI_DIRECTION_2LINES;
    si.DataSize = SPI_DATASIZE_8BIT;
    si.CLKPolarity = SPI_POLARITY_LOW;
    si.CLKPhase = SPI_PHASE_1EDGE;

    si.NSS = SPI_NSS_SOFT;
    //si.NSS = SPI_NSS_HARD_OUTPUT;
    si.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    si.FirstBit = SPI_FIRSTBIT_LSB;
    si.TIMode = SPI_TIMODE_DISABLE;
    si.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    si.CRCPolynomial = 10;
    spi_init(SPI_PORT, &si);

    return 0;
}
static int SPI_DataReady(void)
{
    return (gpio_get(GPIOB, GPIO_PIN_0)==GPIO_PIN_SET);
}
static void SPI_SetCs(U8 hl)
{
    if(hl>0) {
        gpio_set(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    }
    else {
        gpio_set(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    }
}
static int SPI_Write(void* data, int bytes)
{
    return spi_write(SPI_1, data, bytes);
}
static int SPI_Read(void* data, int bytes)
{
    return spi_read(SPI_1, data, bytes);
}
int SPI_WriteRead(void* wdata, void* rdata, int bytes)
{
	return spi_readwrite(SPI_1, wdata, rdata, bytes);
}
//////////////////////////////////////////////////////////////////////


value_reg_t reg_data[50]={0};

value_reg_t default_para[] = {
    {0x01,	0x00F800BE},
    {0x02,	0x0003C003},
    {0x03,	0x00000000},
    {0x04,	0x00000F88},
    {0x05,	0x0000000A},
    {0x06,	0x00000000},
    {0x07,	0x00242424},
    {0x08,	0x00000000},
    {0x09,	0x00002000},
    {0x0A,	0x00079000},
    {0x0B,	0x00000000},
    {0x0C,	0x00047851},
    {0x0D,	0x00040003},
    {0x0E,	0x00000000},
    {0x0F,	0x00000000},
    {0x20,	0x00000000},
    {0x21,	0x00000FE0},
    {0x22,	0x00000017},
    {0x23,	0x00000014},
    {0x24,	0x0000000B},
    {0x25,	0x00000FEB},
    {0x2A,	0x00000000},
    {0x2B,	0x00001200},
    {0x2C,	0x00FFF6FB},
    {0x2D,	0x00000000},
    {0x2E,	0x00000000},
    
    {0xff,	0x00000000},
};

value_reg_t ecg_capture[] = {
    {0x05,	0xE0000B},
    {0x0A,	0x079600},
    {0x01,	0xF804AE},
    //{0x40,	0x000000},
    
    {0xff,	0x000000},
};
value_reg_t dc_leadoff[] = {
    {0x02,	0x000015},
    
    {0xff,	0x000000},
};
value_reg_t ac_leadoff[] = {
    {0x02,	0x000015},
    {0x0c,	0x000015},
    {0x0d,	0x000015},
    
    {0xff,	0x000000},
};




value_reg_t* reg_paras[] = {
    default_para,
    ecg_capture,
    dc_leadoff,
    //ac_leadoff,
    NULL,
};


#define REGID   2   //dc_leadoff

static void hw_reset(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
}

void ADAS1000_WriteRegs(int id)
{
    int i;
    value_reg_t *rv=reg_paras[id];
    
    if(id >=sizeof(reg_paras)/sizeof(value_reg_t*)) {
        return;
    }
    
    for(i=0;;i++) {
        if(rv[i].addr==0xff) {
            break;
        }
        ADAS1000_SetRegisterValue(rv[i].addr, rv[i].value);
    }
}
void ADAS1000_ReadRegs(int id)
{
    int i;
    value_reg_t *rv=reg_paras[id];
    
    for(i=0;;i++) {
        if(rv[i].addr==0xff) {
            break;
        }
        
        reg_data[i].addr = rv[i].addr;
        ADAS1000_GetRegisterValue(reg_data[i].addr, &reg_data[i].value);
    }
}




U8 ADAS1000_Init(U32 rate)
{ 
	U32 minSpiFreq = 0;
	//U32 revId 	 = 0;
    
	//store the selected frame rate
    wordSize = 4;
	
	// Compute the SPI clock frequency.
	switch(rate)
	{
		case ADAS1000_16KHZ_FRAME_RATE:
			minSpiFreq = ADAS1000_16KHZ_FRAME_RATE * 
					  	 ADAS1000_16KHZ_WORD_SIZE * 
					  	 ADAS1000_16KHZ_FRAME_SIZE;
		break;
		case ADAS1000_128KHZ_FRAME_RATE:
            wordSize = 2;
			minSpiFreq = ADAS1000_128KHZ_FRAME_RATE * 
					  	 ADAS1000_128KHZ_WORD_SIZE * 
					  	 ADAS1000_128KHZ_FRAME_SIZE;
		break;
		case ADAS1000_31_25HZ_FRAME_RATE:
			minSpiFreq = (ADAS1000_31_25HZ_FRAME_RATE * 
					  	 ADAS1000_31_25HZ_WORD_SIZE * 
					  	 ADAS1000_31_25HZ_FRAME_SIZE) / 100;
		break;
		default: // ADAS1000_2KHZ__FRAME_RATE
			minSpiFreq = ADAS1000_2KHZ_FRAME_RATE * 
					  	 ADAS1000_2KHZ_WORD_SIZE * 
					  	 ADAS1000_2KHZ_FRAME_SIZE;
		break;
	}
	
	// Initialize the SPI controller. 
	// The SPI frequency must be greater or equal to minSpiFreq.
	if(SPI_Init(0, minSpiFreq, 1, 1)) {		
		return 0;
	}
    
    hw_reset();
    // Reset the ADAS1000.
	ADAS1000_SoftwareReset();   
	
    ADAS1000_WriteRegs(REGID);
    ADAS1000_ReadRegs(REGID);

	// Activate all the channels
	inactiveWordsNo = 0;
	ADAS1000_SetInactiveFrameWords(0);

	//Set the frame rate
	ADAS1000_SetFrameRate(frameRate);

	return 1;
}

/***************************************************************************//**
 * @brief Reads the value of the selected register
 *
 * @param regAddress - The address of the register to read.
 * @param regVal - Pointer to a variable where to store the read data.
 *
 * @return None.
*******************************************************************************/
void ADAS1000_GetRegisterValue(U8 addr, U32* value)
{
	U8 cmd[4]  = {0, 0, 0, 0};
	U8 data[4] = {0, 0, 0, 0};

	// Select the register (For register reads, data is shifted out
	// during the next word).
	cmd[0] = addr;	// Register address.
    
    SPI_SetCs(0);
	SPI_Write(cmd, 4);
    //SPI_SetCs(1);
	
	// Read the data from the device.
    //SPI_SetCs(0);
	SPI_Read(data, 4);
    SPI_SetCs(1);
    
    if(value) {
        *value = ((U32)data[1] << 16) +
                  ((U32)data[2] << 8) +
                  ((U32)data[3] << 0);
    }
}

/***************************************************************************//**
 * @brief Writes a value to the selected register
 *
 * @param regAddress - The address of the register to write to.
 * @param regValue - The value to write to the register.
 *
 * @return None.    
*******************************************************************************/
void ADAS1000_SetRegisterValue(U8 addr, U32 value)
{
	U8 cmd[4] = {0, 0, 0, 0};
	
	cmd[0] = 0x80 + addr;	// Write bit and register address.
	cmd[1] = (U8)((value & 0xFF0000) >> 16);
	cmd[2] = (U8)((value & 0x00FF00) >> 8);
	cmd[3] = (U8)((value & 0x0000FF) >> 0);
    
    SPI_SetCs(0);
	SPI_Write(cmd, 4);
    SPI_SetCs(1);
}


/***************************************************************************//**
 * @brief Resets the ADAS1000 part.
 *
 * @return None.
*******************************************************************************/
void ADAS1000_SoftwareReset(void)
{
	// Clear all registers to their reset value.
	ADAS1000_SetRegisterValue(ADAS1000_ECGCTL, ADAS1000_ECGCTL_SWRST);

	// The software reset requires a NOP command to complete the reset.
	ADAS1000_SetRegisterValue(ADAS1000_NOP, 0);
}

/***************************************************************************//**
 * @brief Selects which words are not included in a data frame.
 *
 * @param channelsMask - Specifies the words to be excluded from the data 
 * 						 frame using a bitwise or of the corresponding bits
 * 						 from the Frame Control Register.
 * 
 * @return None.
*******************************************************************************/
void ADAS1000_SetInactiveFrameWords(U32 wordsMask)
{
	U32 frmCtrlRegVal = 0;
	U8 i = 0;
	
	// Read the current value of the Frame Control Register
	ADAS1000_GetRegisterValue(ADAS1000_FRMCTL, &frmCtrlRegVal);

	//set the inactive channles
	frmCtrlRegVal &= ~ADAS1000_FRMCTL_WORD_MASK;
	frmCtrlRegVal |= wordsMask;

	// Write the new value to the Frame Coontrol register.
	ADAS1000_SetRegisterValue(ADAS1000_FRMCTL, frmCtrlRegVal);
	
	//compute the number of inactive words
	inactiveWordsNo = 0;
	for(i = 0; i < 32; i++)
	{
		if(wordsMask & 0x00000001ul)
		{
			inactiveWordsNo++;
		}
		wordsMask >>= 1;
	}
	
	//compute the new frame size
	switch(frameRate)
	{
		case ADAS1000_16KHZ_FRAME_RATE:
			frameSize = (ADAS1000_16KHZ_WORD_SIZE / 8) *
						(ADAS1000_16KHZ_FRAME_SIZE - inactiveWordsNo);
		break;
		case ADAS1000_128KHZ_FRAME_RATE:
			frameSize = (ADAS1000_128KHZ_WORD_SIZE / 8) *
						(ADAS1000_128KHZ_FRAME_SIZE - inactiveWordsNo);
		break;
		case ADAS1000_31_25HZ_FRAME_RATE:
			frameSize = ((ADAS1000_31_25HZ_WORD_SIZE / 8) *
						(ADAS1000_31_25HZ_FRAME_SIZE - inactiveWordsNo)) / 100;
		break;
		default: // ADAS1000_2KHZ__FRAME_RATE
			frameSize = (ADAS1000_2KHZ_WORD_SIZE / 8) *
						(ADAS1000_2KHZ_FRAME_SIZE - inactiveWordsNo);
		break;
	}
}

/***************************************************************************//**
 * @brief Sets the frame rate.
 *
 * @param rate - ADAS1000 frame rate.
 * 
 * @return None.
*******************************************************************************/
void ADAS1000_SetFrameRate(U32 rate)
{
	U32 frmCtrlRegVal = 0;
	
	// Store the selected frame rate
	frameRate = rate;
	
	// Read the current value of the Frame Control Register
	ADAS1000_GetRegisterValue(ADAS1000_FRMCTL, &frmCtrlRegVal);
	frmCtrlRegVal &= ~ADAS1000_FRMCTL_FRMRATE_MASK;
	
	// Compute the new frame size and update the Frame Control Register value
	switch(frameRate)
	{
		case ADAS1000_16KHZ_FRAME_RATE:
			frameSize = (ADAS1000_16KHZ_WORD_SIZE / 8) *
						(ADAS1000_16KHZ_FRAME_SIZE - inactiveWordsNo);
			frmCtrlRegVal |= ADAS1000_FRMCTL_FRMRATE_16KHZ;
		break;
		case ADAS1000_128KHZ_FRAME_RATE:
			frameSize = (ADAS1000_128KHZ_WORD_SIZE / 8) *
						(ADAS1000_128KHZ_FRAME_SIZE - inactiveWordsNo);
			frmCtrlRegVal |= ADAS1000_FRMCTL_FRMRATE_128KHZ;
		break;
		case ADAS1000_31_25HZ_FRAME_RATE:
			frameSize = ((ADAS1000_31_25HZ_WORD_SIZE / 8) *
						(ADAS1000_31_25HZ_FRAME_SIZE - inactiveWordsNo)) / 100;
			frmCtrlRegVal |= ADAS1000_FRMCTL_FRMRATE_31_25HZ;
		break;
		default: // ADAS1000_2KHZ__FRAME_RATE
			frameSize = (ADAS1000_2KHZ_WORD_SIZE / 8) *
						(ADAS1000_2KHZ_FRAME_SIZE - inactiveWordsNo);
			frmCtrlRegVal |= ADAS1000_FRMCTL_FRMRATE_2KHZ;
		break;
	}
	
	// Write the new Frame control Register value
	ADAS1000_SetRegisterValue(ADAS1000_FRMCTL, frmCtrlRegVal);
}


U8 NOP[100]={0};
int ADAS1000_ReadFrame(frame_t *frame, int frames)
{
    int n=0;
	U8 cmd[4] = {0, 0, 0, 0};
	U32 hdr=0;
    //U8 tmp[buflen];
    
    if(!frame || frames<1) {
        return -1;
    }
    while(SPI_DataReady());
    
    SPI_SetCs(0);
    cmd[0] = ADAS1000_FRAMES;	// frame register address.
    SPI_Write(cmd, 4);
    
    while(n<frames) {
        SPI_WriteRead(NOP, &hdr, 4);
        if(hdr&0x40) {
            frame[n].hdr = hdr;
            SPI_WriteRead(NOP, &frame[n].frame, frameSize-4);
            n++;
        }
        else {
            break;
        }
    }
    SPI_SetCs(1);
    
	// If the frames read sequence must be stopped read a register to stop the frames read.
	//if(stopRead)
	{
		//ADAS1000_GetRegisterValue(ADAS1000_FRMCTL, &tmp);
	}
    
    return n;
}



/***************************************************************************//**
 * @brief Computes the CRC for a frame.
 *
 * @param pBuf - Buffer holding the frame data.
 *
 * @return Returns the CRC value for the given frame.
*******************************************************************************/
U32 ADAS1000_ComputeFrameCrc(U8 *pBuf)
{
	U8 i = 0;
    U32 crc = 0xFFFFFFFFul;
	U32 poly = 0;
	U8 bitCnt = 0;	
	U32 frmSize = 0;
	
	// Select the CRC poly and word size based on the frame rate.
	if(frameRate == ADAS1000_128KHZ_FRAME_RATE)
	{
		poly = CRC_POLY_128KHZ;
		bitCnt = 16;
	}
	else
	{
		poly = CRC_POLY_2KHZ_16KHZ;
		bitCnt = 24;
	}

	frmSize = frameSize;

	// Compute the CRC.
	while(frmSize--)
	{
		crc ^= (((U32)*pBuf++) << (bitCnt - 8));
		for(i = 0; i < 8; i++)
		{
			if(crc & (1ul << (bitCnt - 1)))
				crc = (crc << 1) ^ poly;
            else
				crc <<= 1;
		}
	}
	
	return crc;
}



///////////////////////////////////////////////////////
U8   dataBuffer[24000] = {0, };   
U8   ascii[3] = {0, 0, 0};   
U16  frame = 0;   
U8   character = 0;   
U32   crcValue = 0; 

U32 regVal = 0;
void ADAS1000_Test(void)
{

  /* Initialize the communication with the ADAS1000, set the frame rate to 2KHz */   
    ADAS1000_Init(ADAS1000_2KHZ_FRAME_RATE);   

#if 0     
/* Example on how to read and write a register */   
    // Write the CMREFCTL register with the value 0x85E0000B   
    //ADAS1000_SetRegisterValue(ADAS1000_CMREFCTL, 0x85E0000B);      
    //Read back the value of the CMREFCTL register
	ADAS1000_SetRegisterValue(0xc0, 0xf4ab); 
    ADAS1000_GetRegisterValue(0xc0, &regVal);    
    //Write a NOP command to flush out the data on SDO   
    ADAS1000_SetRegisterValue(ADAS1000_NOP, 0);   
         
/* Example how to start the ADCs Converting and begin streaming ECG data from ADAS1000 at 2kHz rate */   
    //Power up device, enable all ECG channels in gain 1.4, low noise mode,    
    //Master device using XTAL input source   
    ADAS1000_SetRegisterValue(ADAS1000_ECGCTL, 0x81F800AE);   
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0);   
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* Example on how to Enable Respiration, read a data frame */   
    //Configures 56kHz respiration signal with Gain 1,    
    //internal respiration cap selected, LEAD I measure path   
    ADAS1000_SetRegisterValue(ADAS1000_RESPCTL, 0x8302099);   
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0);   
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* Example on how to enable DC Leads Off, read a data frame */   
    //DC Leads off enabled, leads off current 50nA   
    ADAS1000_SetRegisterValue(ADAS1000_LOFFCTL, 0x82000015);   
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0); 
	
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* Example on how to enable Shield Amplifier, read a data frame */   
    //DC Leads off enabled, leads off current 50nA   
    ADAS1000_SetRegisterValue(ADAS1000_CMREFCTL, 0x85E0000B);   
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0);   
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* Example on how to change the number of words in a frame, read a data frame */   
    //Disable the Respiration Magnitude and Respiration Phase words   
    ADAS1000_SetInactiveFrameWords(ADAS1000_FRMCTL_RESPMDIS |    
                                   ADAS1000_FRMCTL_RESPPHDIS);    
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0);   
	
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* From power up, configure device and start streaming ECG data at 2kHz rate   
   with Leads off enabled, respiration enabled (all of above for the following configuration) */                                           
    //Initialize the communication with the ADAS1000, set the frame rate to 2KHz   
    ADAS1000_Init(ADAS1000_2KHZ_FRAME_RATE);   
    //Configure VCM = WCT = (LA+LL+RA)/3. RLD is enabled onto RL, Shield amplifier enabled.    
    ADAS1000_SetRegisterValue(ADAS1000_CMREFCTL, 0x85E0000B);   
    //DC Leads off enabled, leads off current 50nA   
    ADAS1000_SetRegisterValue(ADAS1000_LOFFCTL, 0x82000015);   
    //Configures 56kHz respiration signal with Gain 1,    
    //internal respiration cap selected, LEAD I measure path   
    ADAS1000_SetRegisterValue(ADAS1000_RESPCTL, 0x002099);   
    //Powers up device, enables all ECG channels in gain 1.4,    
    //low noise mode, Master device using XTAL input source   
    ADAS1000_SetRegisterValue(ADAS1000_ECGCTL, 0x81F800AE);   
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0);   
	
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* Configure device and start streaming ECG data at 16kHz rate */   
    //Change the frame rate to 16KHz   
    ADAS1000_SetFrameRate(ADAS1000_16KHZ_FRAME_RATE);   
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0);  
	
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* Configure device and start streaming ECG data at 128kHz rate */   
    //Change the frame rate to 16KHz   
    ADAS1000_SetFrameRate(ADAS1000_128KHZ_FRAME_RATE);   
    //Read a frame of data from the ADAS1000   
    ADAS1000_ReadData(dataBuffer, 1, 1, 1, 0, 0);   
    //Compute the CRC of the frame   
    crcValue = ADAS1000_ComputeFrameCrc(dataBuffer);   
   
/* Configure 150Hz test tone Sinewave on each ECG channel and stream data */                     
    //Initialize the communication with the ADAS1000, set the frame rate to 16KHz   
    ADAS1000_Init(ADAS1000_16KHZ_FRAME_RATE);      
    //Reduce the frame size to the minimum   
    ADAS1000_SetInactiveFrameWords(ADAS1000_FRMCTL_PACEDIS | ADAS1000_FRMCTL_RESPMDIS |    
                                   ADAS1000_FRMCTL_RESPPHDIS |  ADAS1000_FRMCTL_LOFFDIS |    
                                   ADAS1000_FRMCTL_GPIODIS | ADAS1000_FRMCTL_CRCDIS);      
    //Enable the electrode mode in the FRMCTL register   
    ADAS1000_GetRegisterValue(ADAS1000_FRMCTL, &regVal);   
    regVal |= ADAS1000_FRMCTL_DATAFMT;   
    ADAS1000_SetRegisterValue(ADAS1000_FRMCTL, regVal);    
    //Configure VCM = WCT = (LA+LL+RA)/3. RLD is enabled onto RL, Shield amplifier enabled.    
    ADAS1000_SetRegisterValue(ADAS1000_CMREFCTL, 0x8500000B);   
    //150Hz sinewave put onto each electrode.    
    ADAS1000_SetRegisterValue(ADAS1000_TESTTONE, 0x88F8000D);   
    //Change internal LPF to 250Hz   
    ADAS1000_SetRegisterValue(ADAS1000_FILTCTL, 0x8B000008);       
    //Powers up device, enables all ECG channels in gain 1.4,    
    //low noise mode, Master device using XTAL input source   
    ADAS1000_SetRegisterValue(ADAS1000_ECGCTL,   0x81F800AE);   
   
    //Start the data streaming   
    ADAS1000_ReadData(dataBuffer, 1000, 1, 1, 1, 0); 
#endif

    for (;;) {}
}


#ifdef ADAS10XX_TEST
int n=0;
U32 reg=0;
#define FMAX  10
frame_t frames[FMAX]={0};
void ADAS1000_Test(void)
{
    ADAS1000_Init(ADAS1000_2KHZ_FRAME_RATE);
    while (1) {
        n = ADAS1000_ReadFrame(frames, FMAX);
        //ADAS1000_GetRegisterValue(0x1d, &reg);
        ADAS1000_GetRegisterValue(0x1e, &reg);
        //HAL_Delay(1);
    }
}
#endif