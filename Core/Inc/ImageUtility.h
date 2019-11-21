#ifndef __IMAGEUTILITY_H
#define __IMAGEUTILITY_H
#include <Adafruit_ILI9341.h>
#include <JPEGDecoder.h>
#ifdef __cplusplus
extern "C" {
#endif

	// this function determines the minimum of two numbers
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))
	
	void jpegInfo(Adafruit_ILI9341 lcd);
	void renderJPEG(Adafruit_ILI9341 lcd, int xpos, int ypos);

#ifdef __cplusplus
}
#endif

#endif 