
#include "main.h"
#include "ImageUtility.h"
#include "stm32f3xx_hal_def.h"
extern void SPI_DMAHalfTransmitCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMATransmitCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMAReceiveCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMAError(DMA_HandleTypeDef *hdma);

void jpegInfo(Adafruit_ILI9341 lcd) {
	lcd.fillScreen(ILI9341_BLACK);
	lcd.setCursor(0, 0);
	lcd.setTextSize(2);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.println("===============");
	lcd.println("===============");
	lcd.println("JPEG image info");
	lcd.println("===============");
	lcd.print("Width      :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.width);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.print("Height     :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.height);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.print("Components :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.comps);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.print("MCU / row  :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.MCUSPerRow);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.print("MCU / col  :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.MCUSPerCol);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.print("Scan type  :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.scanType);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.print("MCU width  :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.MCUWidth);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.print("MCU height :"); lcd.setTextColor(ILI9341_YELLOW); lcd.println(JpegDec.MCUHeight);
	lcd.setTextColor(ILI9341_GREEN);
	lcd.println("===============");
}

void SPI_WaitForComplete(void)
{
	uint32_t tick, tickstart = HAL_GetTick();
	do tick = HAL_GetTick(); while ((!SPI_Complete) && (tick - tickstart < 2));
	//SPI_Complete = 0;

}

HAL_StatusTypeDef HAL_SPI_Transmit_DMA16(SPI_HandleTypeDef *hspi, uint16_t *pData, uint16_t Size)
{
	HAL_StatusTypeDef errorcode = HAL_OK;

	/* check tx dma handle */
	assert_param(IS_SPI_DMA_HANDLE(hspi->hdmatx));

	/* Check Direction parameter */
	assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE(hspi->Init.Direction));

	/* Process Locked */
	__HAL_LOCK(hspi);

	if (hspi->State != HAL_SPI_STATE_READY)
	{
		errorcode = HAL_BUSY;
		goto error;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		errorcode = HAL_ERROR;
		goto error;
	}

	/* Set the transaction information */
	hspi->State       = HAL_SPI_STATE_BUSY_TX;
	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
	hspi->pTxBuffPtr  = (uint8_t *)pData;
	hspi->TxXferSize  = Size;
	hspi->TxXferCount = Size;

	/* Init field not used in handle to zero */
	hspi->pRxBuffPtr  = (uint8_t *)NULL;
	hspi->TxISR       = NULL;
	hspi->RxISR       = NULL;
	hspi->RxXferSize  = 0U;
	hspi->RxXferCount = 0U;

	/* Configure communication direction : 1Line */
	if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
	{
		SPI_1LINE_TX(hspi);
	}

#if (USE_SPI_CRC != 0U)
	/* Reset CRC Calculation */
	if (hspi->Init.CRCCalculation == SPI_CRCCALCULATION_ENABLE)
	{
		SPI_RESET_CRC(hspi);
	}
#endif /* USE_SPI_CRC */

	/* Set the SPI TxDMA Half transfer complete callback */
	hspi->hdmatx->XferHalfCpltCallback = SPI_DMAHalfTransmitCplt;

	/* Set the SPI TxDMA transfer complete callback */
	hspi->hdmatx->XferCpltCallback = SPI_DMATransmitCplt;

	/* Set the DMA error callback */
	hspi->hdmatx->XferErrorCallback = SPI_DMAError;

	/* Set the DMA AbortCpltCallback */
	hspi->hdmatx->XferAbortCallback = NULL;

	CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_LDMATX);
	/* Packing mode is enabled only if the DMA setting is HALWORD */
	if ((hspi->Init.DataSize <= SPI_DATASIZE_8BIT) && (hspi->hdmatx->Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD))
	{
		/* Check the even/odd of the data size + crc if enabled */
		if ((hspi->TxXferCount & 0x1U) == 0U)
		{
			CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_LDMATX);
			hspi->TxXferCount = (hspi->TxXferCount >> 1U);
		}
		else
		{
			SET_BIT(hspi->Instance->CR2, SPI_CR2_LDMATX);
			hspi->TxXferCount = (hspi->TxXferCount >> 1U) + 1U;
		}
	}

	/* Enable the Tx DMA Stream/Channel */
	HAL_DMA_Start_IT(hspi->hdmatx, (uint32_t)hspi->pTxBuffPtr, (uint32_t)&hspi->Instance->DR, hspi->TxXferCount);

	/* Check if the SPI is already enabled */
	if ((hspi->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(hspi);
	}

	/* Enable the SPI Error Interrupt Bit */
	__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_ERR));

	/* Enable Tx DMA Request */
	SET_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);

error :
	/* Process Unlocked */
	__HAL_UNLOCK(hspi);
	return errorcode;
}

void renderJPEG(Adafruit_ILI9341 lcd, int xpos, int ypos) {

	// retrieve infomration about the image
	uint16_t *pImg;
	uint16_t mcu_w = JpegDec.MCUWidth;
	uint16_t mcu_h = JpegDec.MCUHeight;
	uint32_t max_x = JpegDec.width;
	uint32_t max_y = JpegDec.height;
	uint32_t ofs_x = (lcd.width() - max_x) / 2; 
	uint32_t ofs_y = (lcd.height() - max_y) / 2; 
	if (ofs_x < 0) ofs_x = 0;
	if (ofs_y < 0) ofs_y = 0;
	xpos += ofs_x;
	ypos += ofs_y;

	// Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
	// Typically these MCUs are 16x16 pixel blocks
	// Determine the width and height of the right and bottom edge image blocks
	uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
	uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

	// save the current image block size
	uint32_t win_w = mcu_w;
	uint32_t win_h = mcu_h;

	// record the current time so we can measure how long it takes to draw an image
	uint32_t drawTime = HAL_GetTick();

	// save the coordinate of the right and bottom edges to assist image cropping
	// to the screen size
	max_x += xpos;
	max_y += ypos;
	lcd.startWrite();
	// read each MCU block until there are no more
	while(JpegDec.read()) {

		// save a pointer to the image block
		pImg = JpegDec.pImage;

		// calculate where the image block should be drawn on the screen
		int mcu_x = JpegDec.MCUx * mcu_w + xpos;
		int mcu_y = JpegDec.MCUy * mcu_h + ypos;

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
			
			lcd.setAddrBlock(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
			HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_SET);
			// push all the image block pixels to the screen
				if(SPI_Complete == 1)
				{
					SPI_Complete = 0;
					HAL_SPI_Transmit_DMA(&ILI9341_SPI_PORT, (uint8_t*)(pImg), mcu_pixels * 2);
					SPI_WaitForComplete();
				}
			//}
			int dummy = 12;
		}

		// stop drawing blocks if the bottom of the screen has been reached
		// the abort function will close the file
		else if((mcu_y + win_h) >= ILI9341_HEIGHT) JpegDec.abort();

	}
	lcd.endWrite();



}