#include "XPT2046.h"
#include <math.h>

unsigned int tpx = 0x0000;
unsigned int tpy = 0x0000;

#define NUMSAMPLES 2

TSPoint::TSPoint(void) {
	x = y = 0;
}

TSPoint::TSPoint(int16_t x0, int16_t y0, int16_t z0) {
	x = x0;
	y = y0;
	z = z0;
}

bool TSPoint::operator==(TSPoint p1) {
	return ((p1.x == x) && (p1.y == y) && (p1.z == z));
}

bool TSPoint::operator!=(TSPoint p1) {
	return ((p1.x != x) || (p1.y != y) || (p1.z != z));
}

#if (NUMSAMPLES > 2)
static void insert_sort(int array[], uint8_t size) {
	uint8_t j;
	int save;
  
	for (int i = 1; i < size; i++) {
		save = array[i];
		for (j = i; j >= 1 && save < array[j - 1]; j--)
			array[j] = array[j - 1];
		array[j] = save; 
	}
}
#endif

TSPoint TouchScreen::getPoint(void) {
	int x, y, z;
	read_coordinates(&x, &y);

	return TSPoint(x, y, z);
}

TouchScreen::TouchScreen() {

	
}

void TouchScreen::init(uint8_t rot) {
	rotation = rot;
	tp_init();
	pressureThreshhold = 10;
}


void tp_init()
{
    tp_GPIO_init();
	TP_CS_SET;
	MOSI_SET;
	SCK_SET;
}


void tp_GPIO_init()
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	
	/*Configure GPIO pins as OUTPUT: SCK MOSI */
	GPIO_InitStruct.Pin = SCK_pin | MOSI_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SPI_port, &GPIO_InitStruct);
	/*Configure GPIO pins as INPUT: MISO */
	GPIO_InitStruct.Pin = MISO_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(SPI_port, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = TP_INT_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(TP_INT_port, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = TP_CS_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(TP_CS_port, &GPIO_InitStruct);

	
    /*GPIO_Clk_Enable(&GPIOA_BASE);
    GPIO_Config(&GPIOA_BASE, (_GPIO_PINMASK_4 | _GPIO_PINMASK_6), (_GPIO_CFG_MODE_INPUT | _GPIO_CFG_PULL_UP));
    GPIO_Config(&GPIOA_BASE, (_GPIO_PINMASK_2 | _GPIO_PINMASK_5 | _GPIO_PINMASK_7), (_GPIO_CFG_MODE_OUTPUT | _GPIO_CFG_SPEED_MAX | _GPIO_CFG_OTYPE_PP));*/
}

bool TouchPressed() {
	return HAL_GPIO_ReadPin(TP_INT_port, TP_INT_pin) == GPIO_PIN_RESET;
}

unsigned int TP_read()
{
    unsigned char i = 0x0C;
    unsigned int value = 0x0000;

    while(i > 0x00)
    {
        value <<= 1;

	    SCK_SET;
	    SCK_RESET;

        if(MISO == 1)
        {
            value++;
        }

        i--;
    };

    return value;
}


void tp_write(unsigned char value)
{
    unsigned char i = 0x08;

	SCK_RESET;

    while(i > 0)
    {
        if((value & 0x80) != 0x00)
        {
	        MOSI_SET;
        }
        else
        {
	        MOSI_RESET;
        }

        value <<= 1;

	    SCK_RESET;
	    SCK_SET;
        
        i--;
    };
}


void TouchScreen::read_coordinates(int *x_pos, int *y_pos)
{
    unsigned int tempx = 0x0000;
	unsigned int tempy = 0x0000;
    unsigned int avg_x = 0x0000;
    unsigned int avg_y = 0x0000;

    unsigned char samples = 0x10;
    
	TP_CS_RESET;
    while(samples > 0)
    {
        tp_write(CMD_RDY);
	    SCK_SET;
	    SCK_RESET;
        avg_x += TP_read();

        tp_write(CMD_RDX);
	    SCK_SET;
	    SCK_RESET;
        avg_y += TP_read();
        
        samples--;
    };
	TP_CS_SET;
    
    tempx = (avg_x >> 4);
    tempx = filter_data(tempx, 1);
    
    
    tempy = (avg_y >> 4);
    tempy = filter_data(tempy, 0);
	switch (rotation)
	{
	case 0:
		*x_pos = ((uint16_t)res_w + 1) - tempy;
		*y_pos = ((uint16_t)res_l + 1) -  tempx;
		break;
	case 1:
		*x_pos =  ((uint16_t)res_l + 1) - tempx;
		*y_pos =  tempy;
		break;
	case 2:
		*x_pos = tempy;
		*y_pos = tempx;
		break;
	case 3:
		*x_pos = tempx;
		*y_pos = ((uint16_t)res_w + 1) - tempy;
		break;
	}
	
    
	
}

uint32_t lastTimeX;
uint32_t lastTimeY;

unsigned int lastX, lastY;

uint32_t windowTime = 5;
uint8_t maxDistance = 10;

int filter_data(unsigned int value, unsigned char axis)
{
    float temp[3];
    
    float sum = 0.0;
    float min_R = 0.0;
    float max_R = 0.0;
    float axis_max = 0.0;
    
    unsigned int res = 0x0000;
    
    unsigned char i = 0x00;
    
    switch(axis)
    {
        case 1:
        {
            min_R = RL_min;
            max_R = RL_max;
            axis_max = res_l;
            break;
        }
        default:
        {
            min_R = RW_min;
            max_R = RW_max;
            axis_max = res_w;
            break;
        }
    }
    
    temp[0] = map((((float)value) - error), min_R, max_R, 0.0, axis_max);
    temp[1] = map((((float)value) + error), min_R, max_R, 0.0, axis_max);
    temp[2] = map(((float)value), min_R, max_R, 0.0, axis_max);
    
    for(i = 0; i < 3; i++)
    {
        sum += temp[i];
    }
    
    sum /= 3.0;
    res = constrain(sum, 0.0, axis_max);
	
	//Antinoise
	uint32_t currentTime = HAL_GetTick();
	switch(axis)
	{
	case 1:{
		double dist = abs((double)lastX - (double)res);
			if (((currentTime - lastTimeX) < windowTime) && (dist > maxDistance)){
				res = -1;
			}
			else lastX = res;
			lastTimeX = HAL_GetTick();
			break;
		}
	default:{
		double dist = abs((double)lastY - (double)res);
		if (((currentTime - lastTimeY) < windowTime) && (dist > maxDistance)){
			res = -1;
		}
		else lastY = res;
		lastTimeY = HAL_GetTick();
			break;
		}
	}
    return res;
}


float map(float value, float x_min, float x_max, float y_min, float y_max)
{
    return (y_min + (((y_max - y_min) / (x_max - x_min)) * (value - x_min)));
}


float constrain(float value, float value_min, float value_max)
{
      if(value >= value_max)
      {
           return value_max;
      }
      else if(value <= value_min)
      {
           return value_min;
      }
      else
      {
           return value;
      }
}