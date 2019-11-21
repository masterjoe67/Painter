

#include "Adafruit_ILI9341.h"

#include <limits.h>



#define MADCTL_MY  0x80  ///< Bottom to top
#define MADCTL_MX  0x40  ///< Right to left
#define MADCTL_MV  0x20  ///< Reverse Mode
#define MADCTL_ML  0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00  ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08  ///< Blue-Green-Red pixel order
#define MADCTL_MH  0x04  ///< LCD refresh right to left

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit ILI9341 driver with software SPI
    @param    cs    Chip select pin #
    @param    dc    Data/Command pin #
    @param    mosi  SPI MOSI pin #
    @param    sclk  SPI Clock pin #
    @param    rst   Reset pin # (optional, pass -1 if unused)
    @param    miso  SPI MISO pin # (optional, pass -1 if unused)
*/
/**************************************************************************/
Adafruit_ILI9341::Adafruit_ILI9341(int8_t cs, int8_t dc, int8_t mosi,
        int8_t sclk, int8_t rst, int8_t miso) : Adafruit_SPITFT(ILI9341_TFTWIDTH, ILI9341_TFTHEIGHT, cs, dc, mosi, sclk, rst, miso) {
}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit ILI9341 driver with hardware SPI
    @param    cs    Chip select pin #
    @param    dc    Data/Command pin #
    @param    rst   Reset pin # (optional, pass -1 if unused)
*/
/**************************************************************************/
Adafruit_ILI9341::Adafruit_ILI9341(int8_t cs, int8_t dc, int8_t rst) : Adafruit_SPITFT(ILI9341_TFTWIDTH, ILI9341_TFTHEIGHT, cs, dc, rst) {
}

static const uint8_t initcmd[] = {
  0xEF, 3, 0x03, 0x80, 0x02,
  0xCF, 3, 0x00, 0xC1, 0x30,
  0xED, 4, 0x64, 0x03, 0x12, 0x81,
  0xE8, 3, 0x85, 0x00, 0x78,
  0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  0xF7, 1, 0x20,
  0xEA, 2, 0x00, 0x00,
  ILI9341_PWCTR1  , 1, 0x23,             // Power control VRH[5:0]
  ILI9341_PWCTR2  , 1, 0x10,             // Power control SAP[2:0];BT[3:0]
  ILI9341_VMCTR1  , 2, 0x3e, 0x28,       // VCM control
  ILI9341_VMCTR2  , 1, 0x86,             // VCM control2
  ILI9341_MADCTL  , 1, 0x48,             // Memory Access Control
  ILI9341_VSCRSADD, 1, 0x00,             // Vertical scroll zero
  ILI9341_PIXFMT  , 1, 0x55,
  ILI9341_FRMCTR1 , 2, 0x00, 0x18,
  ILI9341_DFUNCTR , 3, 0x08, 0x82, 0x27, // Display Function Control
  0xF2, 1, 0x00,                         // 3Gamma Function Disable
  ILI9341_GAMMASET , 1, 0x01,             // Gamma curve selected
  ILI9341_GMCTRP1 , 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
  ILI9341_GMCTRN1 , 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  ILI9341_SLPOUT  , 0x80,                // Exit Sleep
  ILI9341_DISPON  , 0x80,                // Display on
  0x00                                   // End of list
};



/**************************************************************************/
/*!
    @brief   Initialize ILI9341 chip
    Connects to the ILI9341 over SPI and sends initialization procedure commands
    @param    freq  Desired SPI clock frequency
*/
/**************************************************************************/
void Adafruit_ILI9341::begin(uint32_t freq) {

    startWrite();
	Reset();
	// SOFTWARE RESET
	writeCommand(0x01);
	HAL_Delay(1000);
	// POWER CONTROL A
	writeCommand(0xCB);
	{
		uint8_t data[] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
		WriteData(data, sizeof(data));
	}

	// POWER CONTROL B
	writeCommand(0xCF);
	{
		uint8_t data[] = { 0x00, 0xC1, 0x30 };
		WriteData(data, sizeof(data));
	}

	// DRIVER TIMING CONTROL A
	writeCommand(0xE8);
	{
		uint8_t data[] = { 0x85, 0x00, 0x78 };
		WriteData(data, sizeof(data));
	}

	// DRIVER TIMING CONTROL B
	writeCommand(0xEA);
	{
		uint8_t data[] = { 0x00, 0x00 };
		WriteData(data, sizeof(data));
	}

	// POWER ON SEQUENCE CONTROL
	writeCommand(0xED);
	{
		uint8_t data[] = { 0x64, 0x03, 0x12, 0x81 };
		WriteData(data, sizeof(data));
	}

	// PUMP RATIO CONTROL
	writeCommand(0xF7);
	{
		uint8_t data[] = { 0x20 };
		WriteData(data, sizeof(data));
	}

	// POWER CONTROL,VRH[5:0]
	writeCommand(0xC0);
	{
		uint8_t data[] = { 0x23 };
		WriteData(data, sizeof(data));
	}

	// POWER CONTROL,SAP[2:0];BT[3:0]
	writeCommand(0xC1);
	{
		uint8_t data[] = { 0x10 };
		WriteData(data, sizeof(data));
	}

	// VCM CONTROL
	writeCommand(0xC5);
	{
		uint8_t data[] = { 0x3E, 0x28 };
		WriteData(data, sizeof(data));
	}

	// VCM CONTROL 2
	writeCommand(0xC7);
	{
		uint8_t data[] = { 0x86 };
		WriteData(data, sizeof(data));
	}

	// MEMORY ACCESS CONTROL
	writeCommand(0x36);
	{
		uint8_t data[] = { 0x48 };
		WriteData(data, sizeof(data));
	}

	// PIXEL FORMAT
	writeCommand(0x3A);
	{
		uint8_t data[] = { 0x55 };
		WriteData(data, sizeof(data));
	}

	// FRAME RATIO CONTROL, STANDARD RGB COLOR
	writeCommand(0xB1);
	{
		uint8_t data[] = { 0x00, 0x18 };
		WriteData(data, sizeof(data));
	}

	// DISPLAY FUNCTION CONTROL
	writeCommand(0xB6);
	{
		uint8_t data[] = { 0x08, 0x82, 0x27 };
		WriteData(data, sizeof(data));
	}

	// 3GAMMA FUNCTION DISABLE
	writeCommand(0xF2);
	{
		uint8_t data[] = { 0x00 };
		WriteData(data, sizeof(data));
	}

	// GAMMA CURVE SELECTED
	writeCommand(0x26);
	{
		uint8_t data[] = { 0x01 };
		WriteData(data, sizeof(data));
	}
	// POSITIVE GAMMA CORRECTION
	writeCommand(0xE0);
	{
		uint8_t data[] = {
			0x0F,
			0x31,
			0x2B,
			0x0C,
			0x0E,
			0x08,
			0x4E,
			0xF1,
			0x37,
			0x07,
			0x10,
			0x03,
			0x0E,
			0x09,
			0x00
		};
		WriteData(data, sizeof(data));
	}

	// NEGATIVE GAMMA CORRECTION
	writeCommand(0xE1);
	{
		uint8_t data[] = {
			0x00,
			0x0E,
			0x14,
			0x03,
			0x11,
			0x07,
			0x31,
			0xC1,
			0x48,
			0x08,
			0x0F,
			0x0C,
			0x31,
			0x36,
			0x0F
		};
		WriteData(data, sizeof(data));
	}

	// EXIT SLEEP
	writeCommand(0x11);
	HAL_Delay(120);

	// TURN ON DISPLAY
	writeCommand(0x29);
/*#define ILI9341_ROTATION (MADCTL_MX | MADCTL_BGR)
	// MADCTL
	writeCommand(0x36);
	{
		
		uint8_t data[] = { ILI9341_ROTATION };
		WriteData(data, sizeof(data));
	}*/
    /*uint8_t        cmd, x, numArgs;
    uint8_t addr = 0;
    while((cmd = initcmd[addr++]) > 0) {
        writeCommand(cmd);
	    x = initcmd[addr++];
        numArgs = x & 0x7F;
	    while (numArgs--) spiWrite(initcmd[addr++]);
	    if (x & 0x80) HAL_Delay(120);
    }*/

    endWrite();

    _width  = ILI9341_TFTWIDTH;
    _height = ILI9341_TFTHEIGHT;
}


/**************************************************************************/
/*!
    @brief   Set origin of (0,0) and orientation of TFT display
    @param   m  The index for rotation, from 0-3 inclusive
*/
/**************************************************************************/
void Adafruit_ILI9341::setRotation(uint8_t m) {
    rotation = m % 4; // can't be higher than 3
    switch (rotation) {
        case 0:
            m = (MADCTL_MX | MADCTL_BGR);
            _width  = ILI9341_TFTWIDTH;
            _height = ILI9341_TFTHEIGHT;
            break;
        case 1:
            m = (MADCTL_MV | MADCTL_BGR);
            _width  = ILI9341_TFTHEIGHT;
            _height = ILI9341_TFTWIDTH;
            break;
        case 2:
            m = (MADCTL_MY | MADCTL_BGR);
            _width  = ILI9341_TFTWIDTH;
            _height = ILI9341_TFTHEIGHT;
            break;
        case 3:
            m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            _width  = ILI9341_TFTHEIGHT;
            _height = ILI9341_TFTWIDTH;
            break;
    }

    startWrite();
    writeCommand(ILI9341_MADCTL);
    spiWrite(m);
    endWrite();
}

/**************************************************************************/
/*!
    @brief   Enable/Disable display color inversion
    @param   invert True to invert, False to have normal color
*/
/**************************************************************************/
void Adafruit_ILI9341::invertDisplay(bool invert) {
    startWrite();
    writeCommand(invert ? ILI9341_INVON : ILI9341_INVOFF);
    endWrite();
}

/**************************************************************************/
/*!
    @brief   Scroll display memory
    @param   y How many pixels to scroll display by
*/
/**************************************************************************/
void Adafruit_ILI9341::scrollTo(uint16_t y) {
    startWrite();
    writeCommand(ILI9341_VSCRSADD);
    SPI_WRITE16(y);
    endWrite();
}

/**************************************************************************/

void Adafruit_ILI9341::setScrollMargins(uint16_t top, uint16_t bottom)
{
  uint16_t height = _height - (top + bottom);

  startWrite();
  writeCommand(0x33);
  SPI_WRITE16(top);
  SPI_WRITE16(height);
  SPI_WRITE16(bottom);
  endWrite();
}

/**************************************************************************/
/*!
    @brief   Set the "address window" - the rectangle we will write to RAM with the next chunk of SPI data writes. The ILI9341 will automatically wrap the data as each row is filled
    @param   x  TFT memory 'x' origin
    @param   y  TFT memory 'y' origin
    @param   w  Width of rectangle
    @param   h  Height of rectangle
*/
/**************************************************************************/
void Adafruit_ILI9341::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint32_t xa = ((uint32_t)x << 16) | (x+w-1);
    uint32_t ya = ((uint32_t)y << 16) | (y+h-1);
    writeCommand(ILI9341_CASET); // Column addr set
    SPI_WRITE32(xa);
    writeCommand(ILI9341_PASET); // Row addr set
    SPI_WRITE32(ya);
    writeCommand(ILI9341_RAMWR); // write to RAM
}

void Adafruit_ILI9341::setAddrBlock(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t RW) {
	startWrite();
	// column address set
	writeCommand(0x2A);    // CASET
	{
		uint8_t data[] = { (x >> 8) & 0xFF, x & 0xFF, (w >> 8) & 0xFF, w & 0xFF };
		WriteData(data, sizeof(data));
	}

	// row address set
	writeCommand(0x2B);    // RASET
	{
		uint8_t data[] = { (y >> 8) & 0xFF, y & 0xFF, (h >> 8) & 0xFF, h & 0xFF };
		WriteData(data, sizeof(data));
	}
	if(RW == 0)
		// write to RAM
		writeCommand(0x2C);    // RAMWR
	else
	{
		// read from RAM
		writeCommand(0x2E);      // RAMRD
		uint8_t dummy = ReadData8();     // dummy read
	}
		
}

uint8_t Adafruit_ILI9341::ReadData8()
{
	uint8_t buff[2];
	HAL_SPI_Receive(&ILI9341_SPI_PORT, buff, 1, HAL_MAX_DELAY);
	return buff[0];
}

void Adafruit_ILI9341::readMemory(char *buf, uint16_t n)
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint16_t color;
	int i, index = 0;
	
	for(i = 0 ; i < n ; i++)
	{
		
		red = ReadData8();
	
		green= ReadData8();
	
		blue = ReadData8();
		
		color = color565(red, green, blue);
		
		buf[index] = (color >> 8) & 0xFF;
		index++;
		buf[index] = color & 0xFF;
		index++;
		
		
	}

}

/**************************************************************************/
/*!
   @brief  Read 8 bits of data from ILI9341 configuration memory. NOT from RAM!
           This is highly undocumented/supported, it's really a hack but kinda works?
    @param    command  The command register to read data from
    @param    index  The byte index into the command to read from
    @return   Unsigned 8-bit data read from ILI9341 register
*/
/**************************************************************************/
uint8_t Adafruit_ILI9341::readcommand8(uint8_t command, uint8_t index) {
    uint32_t freq = _freq;
    if(_freq > 24000000) _freq = 24000000;
    startWrite();
    writeCommand(0xD9);  // woo sekret command?
    spiWrite(0x10 + index);
    writeCommand(command);
    uint8_t r = spiRead();
    endWrite();
    _freq = freq;
    return r;
}

