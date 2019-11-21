/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "SD.h"
#include <stdarg.h>
#include <string>
#include <algorithm>
#include <JPEGDecoder.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ImageReader.h>
#include "ImageUtility.h"
#include "XPT2046.h"

#include "icon.h"

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef ILI9341_SPI_PORT;
SPI_HandleTypeDef HSPI_SDCARD;

UART_HandleTypeDef huart5;

Adafruit_ILI9341 Tft = Adafruit_ILI9341(0, 0);

TouchScreen ts = TouchScreen();

Adafruit_ImageReader reader(SD);

DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi1_rx;


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI3_Init(void);
static void MX_UART5_Init(void);
static void MX_DMA_Init(void);
/* USER CODE BEGIN PFP */

ImageReturnCode stat;     // Status from image-reading functions

Adafruit_GFX_Button btn_exit;
Adafruit_GFX_Button btn_clear;
Adafruit_GFX_Button btn_save;
Adafruit_GFX_Button btn_load;
Adafruit_GFX_Button btn_s1;
Adafruit_GFX_Button btn_s2;
Adafruit_GFX_Button btn_s3;
Adafruit_GFX_Button btn_s4;
Adafruit_GFX_Button buttons[10];

uint8_t SPI_Complete = 1;
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) { 
	SPI_Complete = 1;
}
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) { SPI_Complete = 1; }
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) { SPI_Complete = 1; }

// Parameters for the array of buttons
const int xstartButton[] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };                  // x-min for keypads
const int ystartButton[] = { 12, 36, 60, 84, 108, 132, 156, 180, 204, 228 };            // y-min for keypads

const int widthButton = 16;
const int heightButton = 24;

uint8_t mode;



#define modeMain	0
#define modeFoto	1
#define modeGame	2
#define modePaint	3
#define modeMP3		4

uint8_t modeSelMenu()
{
	mode = 0;
	Tft.fillScreen(ILI9341_WHITE);
	btn_s1.initButtonUL(&Tft, 12, 88, 64, 64, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "", 8);
	btn_s2.initButtonUL(&Tft, 89, 88, 64, 64, ILI9341_WHITE, ILI9341_GREEN, ILI9341_WHITE, "", 8);
	btn_s3.initButtonUL(&Tft, 165, 88, 64, 64, ILI9341_WHITE, ILI9341_YELLOW, ILI9341_WHITE, "", 8);
	btn_s4.initButtonUL(&Tft, 242, 88, 64, 64, ILI9341_WHITE, ILI9341_YELLOW, ILI9341_WHITE, "", 8);
	
	Tft.drawRGBBitmap(12, 88, (uint16_t *)image_data_Foto, 64, 64);
	Tft.drawRGBBitmap(89, 88, (uint16_t *)image_data_Game, 64, 64);
	Tft.drawRGBBitmap(165, 88, (uint16_t *)image_data_Paint, 64, 64);
	Tft.drawRGBBitmap(242, 88, (uint16_t *)image_data_MP3, 64, 64);
	
	int x, y;
	
	/* Infinite loop */
	for (;;)
	{
		if (TouchPressed())
		{
			ts.read_coordinates(&x, &y);
			int userInput = -1; 
			userInput = getButtonNumber(x, y);
			if (userInput >= 0)
			{
				switch (userInput)
				{
				case 1:
					return modeFoto;
					break;
				case 2:
					return modeGame;
					break;
				case 3:
					return modePaint;
					break;
				case 4:
					return modeMP3;
					break;
				}
			}
		}
	}
	
	
}

void paintMenu()
{
	
  
	// Sets the coordinates and sizes of the keypad buttons, and sets all the values for the buttons' text
	const int noRows = 4;
	const int noColumns = 3;
	Tft.fillScreen(ILI9341_BLACK);
	int y = 0; 
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE, "", 8);
	
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_GREENYELLOW, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_MAGENTA, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_CYAN, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_WHITE, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_YELLOW, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_GREEN, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_MAROON, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "", 8);
	y++;
	buttons[y].initButton(&Tft, xstartButton[y], ystartButton[y], widthButton, heightButton, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "", 8);
	

	/// Draw the array of buttons
	for(int i = 0 ; i < 10 ; i++) 
	  buttons[i].drawButton();
	
	btn_exit.initButtonUL(&Tft, 303, 2, 16, 16, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "", 8);
	btn_clear.initButtonUL(&Tft, 303, 26, 16, 16, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "", 8);
	btn_save.initButtonUL(&Tft, 303, 50, 16, 16, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "", 8);
	btn_load.initButtonUL(&Tft, 303, 74, 16, 16, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "", 8);
	//btn_exit.drawButton();
	Tft.drawRGBBitmap(303, 2, (uint16_t *)image_data_Cancel, 16, 16);
	Tft.drawRGBBitmap(303, 26, (uint16_t *)image_data_Trash, 16, 16);
	Tft.drawRGBBitmap(303, 50, (uint16_t *)image_data_Save, 16, 16);
	Tft.drawRGBBitmap(303, 74, (uint16_t *)image_data_Load, 16, 16);

	
}

void clearPad(uint16_t color)
{
	Tft.fillRect(widthButton + 1, 0, ILI9341_WIDTH - (widthButton * 2) - 2, ILI9341_HEIGHT, color);
}

void paint()
{
	mode = 3;
	uint16_t currentColor = ILI9341_WHITE;
	paintMenu();
	int x, y;
	
	/* Infinite loop */
	for (;;)
	{
		if (TouchPressed())
		{
			ts.read_coordinates(&x, &y);
			
			
			// Determine which button was pressed
			int userInput = -1;      // -1 means no button is pressed
			int w = widthButton;
			
			if ((x < w) || (x > 320 - w))
			{
				userInput = getButtonNumber(x, y);
			}
			
			if (userInput >= 0)
			{
				switch (userInput)
				{
				case 9:
					currentColor = ILI9341_BLUE;
					break;
				case 8:
					currentColor = ILI9341_RED;
					break;
				case 7:
					currentColor = ILI9341_MAROON;
					break;
				case 6:
					currentColor = ILI9341_GREEN;
					break;
				case 5:
					currentColor = ILI9341_YELLOW;
					break;
				case 4:
					currentColor = ILI9341_WHITE;
					break;
				case 3:
					currentColor = ILI9341_CYAN;
					break;
				case 2:
					currentColor = ILI9341_MAGENTA;
					break;
				case 1:
					currentColor = ILI9341_GREENYELLOW;
					break;
				case 0:
					currentColor = ILI9341_BLACK;
					break;
				case 11:
					return;
					break;
				case 12:
					clearPad(currentColor);
					break;
				case 13:
				
					save();
					break;
				case 14:
				
					load();
					break;
				}
			}
			if (x > (widthButton + 2) && x < (ILI9341_WIDTH - widthButton - 3))
			{
				if (x >= 0 && y >= 0)
					Tft.fillCircle(x, y, 2, currentColor);
			
				//Tft.setPixel(x, 240 - y, currentColor);
			}
			//UART_Printf("raw_x = %i    raw_y = %i\r\n", x, y);
			//HAL_Delay(5);
		}	
		//osDelay(1);
	}
	
}

void convertToUpperCase(char *sPtr)
{
	while (*sPtr != '\0')
	{
		if (islower(*sPtr))
			*sPtr = toupper(*sPtr);
		sPtr++;
	}
}

int slideshowMenu()
{
	int x, y;
	int userInput = -1;
	Tft.drawRGBBitmap(303, 2, (uint16_t *)image_data_Cancel, 16, 16);
	uint32_t curTime = HAL_GetTick();
	uint32_t endTime = curTime + 5000;
	while (HAL_GetTick() < endTime)
	{
		
	
		if (TouchPressed())
		{
			ts.read_coordinates(&x, &y);
			//userInput = -1;
			userInput = getButtonNumber(x, y);
			if (userInput == 11) return userInput;
		}
	}
	return userInput;
}

void slideshow()
{
	btn_exit.initButtonUL(&Tft, 303, 2, 16, 16, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "", 8);
	mode = modeFoto;
	File dir = SD.open("/");
	dir.rewindDirectory();
	Tft.fillScreen(ILI9341_BLACK);
	while (true) {
		File entry =  dir.openNextFile();
		if (!entry) {
			break;
		}
		// Recurse for directories, otherwise print the file size
		if(!entry.isDirectory()) {
			String str(entry.name());
			String extension; 
			char ext[10];
			(str.substring(str.lastIndexOf(".") + 1).toCharArray(ext, 10, 0));
			convertToUpperCase(ext);
			String s(ext);
			if (s == "JPG") {
				JpegDec.decodeSdFile(entry);
				renderJPEG(Tft, 0, 0);
				if (slideshowMenu() == 11) return;
			}
			if (s == "BMP")
			{
				reader.drawBMP(entry.name(), Tft, 0, 0);
				if (slideshowMenu() == 11) return;
			}
			Tft.fillScreen(ILI9341_BLACK);
		}
		entry.close();
	}
}


#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor, sc, v = 0, pv = 0, i = 0, j = 0;
void bird(int n)
{
	Tft.fillCircle(Tft.width() / 3, Tft.height() / 2 - 45 + v, 18, ILI9341_YELLOW);
	Tft.drawCircle(Tft.width() / 3, Tft.height() / 2 - 45 + v, 18, ILI9341_BLACK);
	Tft.fillRect(Tft.width() / 3, Tft.height() / 2 - 40 + v, 20, 7, ILI9341_RED);
	Tft.drawRect(Tft.width() / 3, Tft.height() / 2 - 40 + v, 20, 7, ILI9341_BLACK);
	Tft.fillRect(Tft.width() / 3, Tft.height() / 2 - 37 + v, 20, 1, ILI9341_BLACK);
	Tft.fillCircle(Tft.width() / 3 + 7, Tft.height() / 2 - 54 + v, 7, ILI9341_WHITE);
	Tft.drawCircle(Tft.width() / 3 + 7, Tft.height() / 2 - 54 + v, 7, ILI9341_BLACK);
	Tft.fillCircle(Tft.width() / 3 + 7, Tft.height() / 2 - 54 + v, 2, ILI9341_BLACK);
	Tft.fillRect(Tft.width() / 3 - 23, Tft.height() / 2 - 45 + v, 18, 7, ILI9341_WHITE);
	Tft.drawRect(Tft.width() / 3 - 23, Tft.height() / 2 - 45 + v, 18, 7, ILI9341_BLACK);
}
void birdcyan(int pv)
{
	Tft.fillCircle(Tft.width() / 3, Tft.height() / 2 - 45 + pv, 18, ILI9341_CYAN);
	Tft.drawCircle(Tft.width() / 3, Tft.height() / 2 - 45 + pv, 18, ILI9341_CYAN);
	Tft.fillRect(Tft.width() / 3, Tft.height() / 2 - 40 + pv, 20, 7, ILI9341_CYAN);
	Tft.drawRect(Tft.width() / 3, Tft.height() / 2 - 40 + pv, 20, 7, ILI9341_CYAN);
	Tft.fillRect(Tft.width() / 3, Tft.height() / 2 - 37 + pv, 20, 1, ILI9341_CYAN);
	Tft.fillCircle(Tft.width() / 3 + 7, Tft.height() / 2 - 54 + pv, 7, ILI9341_CYAN);
	Tft.drawCircle(Tft.width() / 3 + 7, Tft.height() / 2 - 54 + pv, 7, ILI9341_CYAN);
	Tft.fillCircle(Tft.width() / 3 + 7, Tft.height() / 2 - 54 + pv, 2, ILI9341_CYAN);
	Tft.fillRect(Tft.width() / 3 - 23, Tft.height() / 2 - 45 + pv, 18, 7, ILI9341_CYAN);
	Tft.drawRect(Tft.width() / 3 - 23, Tft.height() / 2 - 45 + pv, 18, 7, ILI9341_CYAN);
}


float a = 0.5;
void Gameloop()
{
	while (true)
	{
	         
		
	



		if (TouchPressed()) {
			TSPoint p = ts.getPoint();
			if (p.x > 0 && p.x<320 && p.y>0 && p.y < 240)
			{

				j = -8;

			}
		}

		//for(int i=0;i<282;i++){

  
		Tft.fillRect(Tft.width() - 40 - (i * 2), 20, 20, 50, 0xD78A);
		Tft.fillRect(Tft.width() - 40 - (i * 2), 165, 20, 50, 0xD78A);
   
		Tft.fillRect(Tft.width() - 20 - (i * 2), 20, 20, 50, ILI9341_CYAN);
		Tft.fillRect(Tft.width() - 20 - (i * 2), 165, 20, 50, ILI9341_CYAN);
   
		//delay(1);
		Tft.fillRect(0, 0, 20, Tft.height(), 0xD78A);
		//for(int i1=0;i1<4;i1++){
		 pv = v;
		v = v + (4*a*j);
		birdcyan(pv);
		bird(v);
		j++;
		// }
		  i++;
   
		//Serial.println(v);
		//Serial.println(p.x);
		//Serial.print(" \t    ");
		//Serial.print(p.y);
		  if(i == 141 &&(v <= 120 || v > -34 || (280 - (2*i)<106 && 280 - (2*i)>70 && 57 + v>73 && 93 + v < 165)))
		  i = 0;
		else if((v >= 122   || v < (-34) || (280 - (2*i)<106 && 280 - (2*i)>70 &&(57 + v<73 || 93 + v>165))  && i < 143))
		{
			Tft.fillScreen(ILI9341_CYAN);
			Tft.setCursor(Tft.width() / 3 + 36, Tft.height() / 2);
			Tft.setTextSize(3);
			Tft.println("Game over");
			//Serial.println("Game over");
			i = 500;
		}
		else {}//a=0.4;
		//}
	}
  
}

char buffer[512];
void save()
{
	if (SD.exists("buffer.bin"))
	{
		SD.remove("buffer.bin");
	}
	char buf[2];
	File myFile = SD.open("buffer.bin", FILE_WRITE);
	if (myFile) {

		uint16_t mcu_w = 16;
		uint16_t mcu_h = 16;
		uint32_t max_x = 288;
		uint32_t max_y = 240;
		
		uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
		uint32_t min_h = minimum(mcu_h, max_y % mcu_h);
		
		uint32_t win_w = mcu_w;
		uint32_t win_h = mcu_h;
		
		max_x += 16;
		max_y += 0;
		
		for (uint8_t y_block = 0; y_block < 185; y_block++)
		{
			for (uint8_t x_block = 0; x_block < 18; x_block++) {


				// calculate where the image block should be drawn on the screen
				int mcu_x = x_block * 16 + 16;
				int mcu_y = y_block * 16 + 0;

				// check if the image block size needs to be changed for the right and bottom edges
				if(mcu_x + mcu_w <= max_x) win_w = mcu_w;
				else win_w = min_w;
				if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
				else win_h = min_h;

				// calculate how many pixels must be drawn
				uint32_t mcu_pixels = win_w * win_h;

				// draw image block if it will fit on the screen
		
				if((mcu_x + win_w) <= ILI9341_WIDTH && (mcu_y + win_h) <= ILI9341_HEIGHT) {
					// open a window onto the screen to paint the pixels into
					//TFTscreen.setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
			
					Tft.setAddrBlock(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1, 1);
					Tft.readMemory(buffer, mcu_pixels);
					uint16_t index = 0;
					
					while (mcu_pixels--)
					{
						uint16_t color = buffer[index++];
						buf[0] = color & 0x00FF;
						buf[1] = color >> 8 & 0x00FF;
						myFile.write(buf, 2);
					}
				
				}

				// stop drawing blocks if the bottom of the screen has been reached
				// the abort function will close the file
				else if((mcu_y + win_h) >= ILI9341_HEIGHT) break;

			}
		}
		
		
		
		
		// close the file:
		myFile.close();
		UART_Printf("done.");
	}
	else {
		// if the file didn't open, print an error:
		UART_Printf("error opening test.txt");
	}
	
	
	
}

void load()
{
	File myFile = SD.open("buffer.bin", FILE_READ);
	if (myFile) {
		uint16_t mcu_w = 16;
		uint16_t mcu_h = 16;
		uint32_t max_x = 288;
		uint32_t max_y = 240;
		
		uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
		uint32_t min_h = minimum(mcu_h, max_y % mcu_h);
		
		uint32_t win_w = mcu_w;
		uint32_t win_h = mcu_h;
		
		max_x += 16;
		max_y += 0;
		Tft.startWrite();
		for (uint8_t y_block = 0; y_block < 185; y_block++)
		{
			for (uint8_t x_block = 0; x_block < 18; x_block++) {


				// calculate where the image block should be drawn on the screen
				int mcu_x = x_block * 16 + 16;
				int mcu_y = y_block * 16 + 0;

				// check if the image block size needs to be changed for the right and bottom edges
				if(mcu_x + mcu_w <= max_x) win_w = mcu_w;
				else win_w = min_w;
				if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
				else win_h = min_h;

				// calculate how many pixels must be drawn
				uint32_t mcu_pixels = win_w * win_h;

				// draw image block if it will fit on the screen
		
				if((mcu_x + win_w) <= ILI9341_WIDTH && (mcu_y + win_h) <= ILI9341_HEIGHT) {
					// open a window onto the screen to paint the pixels into
					//TFTscreen.setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
			
					myFile.read(buffer, mcu_pixels);
					
					Tft.setAddrBlock(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1, 0);
					
					int i = 0;
					while (mcu_pixels--) {
						uint16_t co = buffer[i++]  + buffer[i++] << 8 & 0xFF00;
						Tft.pushColor(co);
						//Tft.spiWrite(buffer[i++]);
						//Tft.WriteData((uint8_t*)buffer, mcu_pixels);
					}
					i += 1;  //dummy
				}

				// stop drawing blocks if the bottom of the screen has been reached
				// the abort function will close the file
				else if((mcu_y + win_h) >= ILI9341_HEIGHT) goto exit;

			}
			Tft.endWrite();
		}
exit:		
		
		
		
		// close the file:
		myFile.close();
		UART_Printf("done.");
	}
	else {
		// if the file didn't open, print an error:
		UART_Printf("error opening test.txt");
	}
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_SPI1_Init();
	MX_SPI3_Init();
	MX_UART5_Init();
	
	Tft.begin();
	Tft.setRotation(3);
	ts.init(3);
	//End initialization
	UART_Printf("Initializing SD card...");

	if (!SD.begin(4)) {
		UART_Printf("initialization failed!");
		while (1) ;
	}
	UART_Printf("initialization done.");
	while (1)
	{
		switch (modeSelMenu())
		{
		case modeGame:
			Gameloop();
			break;
		case modeFoto:
			slideshow();
			break;
		case modePaint:
			paint();
			break;
		}
	}
	
	 
	
	//////////////////////////////////////////////////////
	if(SD.exists("Baboon20.jpg"))
		UART_Printf("File exist.");
	
	// open the image file
	File jpgFile = SD.open("Baboon20.jpg", FILE_READ);

	// initialise the decoder to give access to image information
	JpegDec.decodeSdFile(jpgFile);
	
	jpegInfo(Tft);
	
	HAL_Delay(1000);
	Tft.fillScreen(ILI9341_BLACK);
	// render the image onto the screen at coordinate 0,0
	  renderJPEG(Tft, 0, 0);
	jpgFile.close();
	
	///////////////////////////////////////////////////
		if(SD.exists("/purple.bmp"))
		UART_Printf("File exist.");
	
	stat = reader.drawBMP("/purple.bmp", Tft, 0, 0);
	
	
	/////////////////////////////////////////////////////


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while(1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the CPU, AHB and APB busses clocks 
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks 
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_UART5;
	PeriphClkInit.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{

	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
	/* DMA1_Channel3_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
	/* SPI1 parameter configuration*/
	ILI9341_SPI_PORT.Instance = SPI1;
	ILI9341_SPI_PORT.Init.Mode = SPI_MODE_MASTER;
	ILI9341_SPI_PORT.Init.Direction = SPI_DIRECTION_2LINES;
	ILI9341_SPI_PORT.Init.DataSize = SPI_DATASIZE_8BIT;
	ILI9341_SPI_PORT.Init.CLKPolarity = SPI_POLARITY_LOW;
	ILI9341_SPI_PORT.Init.CLKPhase = SPI_PHASE_1EDGE;
	ILI9341_SPI_PORT.Init.NSS = SPI_NSS_SOFT;
	ILI9341_SPI_PORT.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	ILI9341_SPI_PORT.Init.FirstBit = SPI_FIRSTBIT_MSB;
	ILI9341_SPI_PORT.Init.TIMode = SPI_TIMODE_DISABLE;
	ILI9341_SPI_PORT.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	ILI9341_SPI_PORT.Init.CRCPolynomial = 7;
	ILI9341_SPI_PORT.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	ILI9341_SPI_PORT.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	if (HAL_SPI_Init(&ILI9341_SPI_PORT) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{
	/* SPI3 parameter configuration*/
	HSPI_SDCARD.Instance = SPI3;
	HSPI_SDCARD.Init.Mode = SPI_MODE_MASTER;
	HSPI_SDCARD.Init.Direction = SPI_DIRECTION_2LINES;
	HSPI_SDCARD.Init.DataSize = SPI_DATASIZE_8BIT;
	HSPI_SDCARD.Init.CLKPolarity = SPI_POLARITY_LOW;
	HSPI_SDCARD.Init.CLKPhase = SPI_PHASE_1EDGE;
	HSPI_SDCARD.Init.NSS = SPI_NSS_SOFT;
	HSPI_SDCARD.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	HSPI_SDCARD.Init.FirstBit = SPI_FIRSTBIT_MSB;
	HSPI_SDCARD.Init.TIMode = SPI_TIMODE_DISABLE;
	HSPI_SDCARD.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	HSPI_SDCARD.Init.CRCPolynomial = 7;
	HSPI_SDCARD.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	HSPI_SDCARD.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	if (HAL_SPI_Init(&HSPI_SDCARD) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI3_Init 2 */

	/* USER CODE END SPI3_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{
	huart5.Instance = UART5;
	huart5.Init.BaudRate = 115200;
	huart5.Init.WordLength = UART_WORDLENGTH_8B;
	huart5.Init.StopBits = UART_STOPBITS_1;
	huart5.Init.Parity = UART_PARITY_NONE;
	huart5.Init.Mode = UART_MODE_TX_RX;
	huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart5.Init.OverSampling = UART_OVERSAMPLING_16;
	huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart5) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN UART5_Init 2 */

	/* USER CODE END UART5_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(Giro_Block_GPIO_Port, Giro_Block_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC,
		ILI9341_RS_Pin|ILI9341_RST_Pin|ILI9341_CS_Pin|SD_CS_Pin 
	                        |TOUCH_CS_Pin,
		GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, TOUCH_SCK_Pin | TOUCH_MOSI_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : Giro_Block_Pin */
	GPIO_InitStruct.Pin = Giro_Block_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(Giro_Block_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : ILI9341_RS_Pin ILI9341_RST_Pin ILI9341_CS_Pin SD_CS_Pin 
	                         TOUCH_CS_Pin */
	GPIO_InitStruct.Pin = ILI9341_RS_Pin | ILI9341_RST_Pin | ILI9341_CS_Pin | SD_CS_Pin 
	                        | TOUCH_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : TOUCH_SCK_Pin TOUCH_MOSI_Pin */
	GPIO_InitStruct.Pin = TOUCH_SCK_Pin | TOUCH_MOSI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : TOUCH_MISO_Pin */
	GPIO_InitStruct.Pin = TOUCH_MISO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(TOUCH_MISO_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : TOUCH_IRQ_Pin */
	GPIO_InitStruct.Pin = TOUCH_IRQ_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(TOUCH_IRQ_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

	/* USER CODE END Callback 1 */
}

void UART_Printf(const char* fmt, ...) {
	char buff[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buff, sizeof(buff), fmt, args);
	HAL_UART_Transmit(&huart5,
		(uint8_t*)buff,
		strlen(buff),
		HAL_MAX_DELAY);
	va_end(args);
}

// Gets which button was pressed.  If no button is pressed, -1 is returned.
int getButtonNumber(int xInput, int yInput)
{
	if (mode == modeMain)
	{
		if (btn_s1.isPressed(xInput, yInput)) {
			return 1; // Signifies big button was pressed
		}
		if (btn_s2.isPressed(xInput, yInput)) {
			return 2; // Signifies big button was pressed
		}
		if (btn_s3.isPressed(xInput, yInput)) {
			return 3; // Signifies big button was pressed
		}
		if (btn_s4.isPressed(xInput, yInput)) {
			return 4; // Signifies big button was pressed
		}
	}
	if (mode == modePaint)
	{
		for (int i = 0; i < 10; i++) {
			if (buttons[i].isPressed(xInput, yInput)) 
				return i; // Returns the button number that was pressed
		}
		if (btn_exit.isPressed(xInput, yInput)) {
			return 11; // Signifies big button was pressed
		}
		if (btn_clear.isPressed(xInput, yInput)) {
			return 12; // Signifies big button was pressed
		}
		if (btn_save.isPressed(xInput, yInput)) {
			return 13; // Signifies big button was pressed
		}
		if (btn_load.isPressed(xInput, yInput)) {
			return 14; // Signifies big button was pressed
		}
	}
	if (mode == modeFoto)
	{
		if (btn_exit.isPressed(xInput, yInput)) {
			return 11; // Signifies big button was pressed
		}
	}
	return -1;
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	 /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
