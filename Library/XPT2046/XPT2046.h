

#ifndef __XPT2046_TOUCH_H__
#define __XPT2046_TOUCH_H__

#include "stm32f3xx_hal.h"
#include <stdbool.h>

#define TP_INT_port																		GPIOC
#define TP_INT_pin                                                                      GPIO_PIN_9
#define TP_CS_port                                                                       GPIOC
#define TP_CS_pin                                                                       GPIO_PIN_8

#define SPI_port																		GPIOB
#define MISO_port																		GPIOB
#define MISO_pin                                                                        GPIO_PIN_14
#define MOSI_port                                                                       GPIOB
#define MOSI_pin                                                                        GPIO_PIN_15
#define SCK_port                                                                        GPIOB
#define SCK_pin                                                                         GPIO_PIN_13

#define TP_CS_SET	HAL_GPIO_WritePin(TP_CS_port, TP_CS_pin, GPIO_PIN_SET)
#define TP_CS_RESET	HAL_GPIO_WritePin(TP_CS_port, TP_CS_pin, GPIO_PIN_RESET)

#define MOSI_SET	HAL_GPIO_WritePin(MOSI_port, MOSI_pin, GPIO_PIN_SET)
#define MOSI_RESET	HAL_GPIO_WritePin(MOSI_port, MOSI_pin, GPIO_PIN_RESET)

#define SCK_SET		HAL_GPIO_WritePin(SCK_port, SCK_pin, GPIO_PIN_SET)
#define SCK_RESET	HAL_GPIO_WritePin(SCK_port, SCK_pin, GPIO_PIN_RESET)

#define MISO		HAL_GPIO_ReadPin(MISO_port, MISO_pin)

#define CMD_RDY                                                                         0X90
#define CMD_RDX                                                                         0XD0

#define error                                                                           50.0

#define RL_min                                                                          450.0
#define RL_max                                                                          3950.0
#define RW_min                                                                          3800.0
#define RW_max                                                                          300.0

#define res_l                                                                           319.0
#define res_w                                                                           239.0

class TSPoint {
public:
	TSPoint(void);
	TSPoint(int16_t x, int16_t y, int16_t z);
  
	bool operator==(TSPoint);
	bool operator!=(TSPoint);

	int16_t x, y, z;
};

class TouchScreen {
public:
	TouchScreen();
	void init(uint8_t rot);
	bool isTouching(void);
	uint16_t pressure(void);
	int readTouchY();
	int readTouchX();
	TSPoint getPoint();
	int16_t pressureThreshhold;
	uint8_t rotation;
	void read_coordinates(int *x_pos, int *y_pos);	
private:
	uint8_t _yp, _ym, _xm, _xp;
	uint16_t _rxplate;



};


extern unsigned int tpx;
extern unsigned int tpy;


void tp_init();
void tp_GPIO_init();
unsigned int TP_read();
void tp_write(unsigned char value);

int filter_data(unsigned int value, unsigned char axis);
float map(float value, float x_min, float x_max, float y_min, float y_max);
float constrain(float value, float value_min, float value_max);

bool TouchPressed();

#endif
