/*!
 * @file Adafruit_SPITFT.cpp
 *
 * @mainpage Adafruit SPI TFT Displays (and some others)
 *
 * @section intro_sec Introduction
 *
 * Part of Adafruit's GFX graphics library. Originally this class was
 * written to handle a range of color TFT displays connected via SPI,
 * but over time this library and some display-specific subclasses have
 * mutated to include some color OLEDs as well as parallel-interfaced
 * displays. The name's been kept for the sake of older code.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!

 * @section dependencies Dependencies
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * @section author Author
 *
 * Written by Limor "ladyada" Fried for Adafruit Industries,
 * with contributions from the open source community.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 */



#include "Adafruit_SPITFT.h"

#if defined(__AVR__)
#if defined(__AVR_XMEGA__)  //only tested with __AVR_ATmega4809__
#define AVR_WRITESPI(x) for(SPI0_DATA = (x); (!(SPI0_INTFLAGS & _BV(SPI_IF_bp))); )
#else
#define AVR_WRITESPI(x) for(SPDR = (x); (!(SPSR & _BV(SPIF))); )
#endif
#endif

#if defined(PORT_IOBUS)
// On SAMD21, redefine digitalPinToPort() to use the slightly-faster
// PORT_IOBUS rather than PORT (not needed on SAMD51).
#undef  digitalPinToPort
#define digitalPinToPort(P) (&(PORT_IOBUS->Group[g_APinDescription[P].ulPort]))
#endif // end PORT_IOBUS

#if defined(USE_SPI_DMA)
 #include <Adafruit_ZeroDMA.h>
 #include "wiring_private.h"  // pinPeripheral() function
 #include <malloc.h>          // memalign() function
 #define tcNum        2       // Timer/Counter for parallel write strobe PWM
 #define wrPeripheral PIO_CCL // Use CCL to invert write strobe

    // DMA transfer-in-progress indicator and callback
    static volatile bool dma_busy = false;
    static void dma_callback(Adafruit_ZeroDMA *dma) {
        dma_busy = false;
    }

 #if defined(__SAMD51__)
    // Timer/counter info by index #
    static const struct {
        Tc *tc;   // -> Timer/Counter base address
        int gclk; // GCLK ID
        int evu;  // EVSYS user ID
    } tcList[] = {
      { TC0, TC0_GCLK_ID, EVSYS_ID_USER_TC0_EVU },
      { TC1, TC1_GCLK_ID, EVSYS_ID_USER_TC1_EVU },
      { TC2, TC2_GCLK_ID, EVSYS_ID_USER_TC2_EVU },
      { TC3, TC3_GCLK_ID, EVSYS_ID_USER_TC3_EVU },
  #if defined(TC4)
      { TC4, TC4_GCLK_ID, EVSYS_ID_USER_TC4_EVU },
  #endif
  #if defined(TC5)
      { TC5, TC5_GCLK_ID, EVSYS_ID_USER_TC5_EVU },
  #endif
  #if defined(TC6)
      { TC6, TC6_GCLK_ID, EVSYS_ID_USER_TC6_EVU },
  #endif
  #if defined(TC7)
      { TC7, TC7_GCLK_ID, EVSYS_ID_USER_TC7_EVU }
  #endif
     };
  #define NUM_TIMERS (sizeof tcList / sizeof tcList[0]) ///< # timer/counters
 #endif // end __SAMD51__

#endif // end USE_SPI_DMA

// Possible values for Adafruit_SPITFT.connection:
#define TFT_HARD_SPI 0  ///< Display interface = hardware SPI
#define TFT_SOFT_SPI 1  ///< Display interface = software SPI
#define TFT_PARALLEL 2  ///< Display interface = 8- or 16-bit parallel


// CONSTRUCTORS ------------------------------------------------------------

/*!
    @brief   Adafruit_SPITFT constructor for software (bitbang) SPI.
    @param   w     Display width in pixels at default rotation setting (0).
    @param   h     Display height in pixels at default rotation setting (0).
    @param   cs    Arduino pin # for chip-select (-1 if unused, tie CS low).
    @param   dc    Arduino pin # for data/command select (required).
    @param   mosi  Arduino pin # for bitbang SPI MOSI signal (required).
    @param   sck   Arduino pin # for bitbang SPI SCK signal (required).
    @param   rst   Arduino pin # for display reset (optional, display reset
                   can be tied to MCU reset, default of -1 means unused).
    @param   miso  Arduino pin # for bitbang SPI MISO signal (optional,
                   -1 default, many displays don't support SPI read).
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized; application typically will
             need to call subclass' begin() function, which in turn calls
             this library's initSPI() function to initialize pins.
*/
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h,
  int8_t cs, int8_t dc, int8_t mosi, int8_t sck, int8_t rst, int8_t miso) :
  Adafruit_GFX(w, h), connection(TFT_SOFT_SPI), _rst(rst), _cs(cs), _dc(dc) {
    swspi._sck  = sck;
    swspi._mosi = mosi;
    swspi._miso = miso;
#if defined(USE_FAST_PINIO)
 #if defined(HAS_PORT_SET_CLR)
  #if defined(CORE_TEENSY)
   #if !defined(KINETISK)
    dcPinMask          = digitalPinToBitMask(dc);
    swspi.sckPinMask   = digitalPinToBitMask(sck);
    swspi.mosiPinMask  = digitalPinToBitMask(mosi);
   #endif
    dcPortSet          = portSetRegister(dc);
    dcPortClr          = portClearRegister(dc);
    swspi.sckPortSet   = portSetRegister(sck);
    swspi.sckPortClr   = portClearRegister(sck);
    swspi.mosiPortSet  = portSetRegister(mosi);
    swspi.mosiPortClr  = portClearRegister(mosi);
    if(cs >= 0) {
   #if !defined(KINETISK)
        csPinMask      = digitalPinToBitMask(cs);
   #endif
        csPortSet      = portSetRegister(cs);
        csPortClr      = portClearRegister(cs);
    } else {
   #if !defined(KINETISK)
        csPinMask      = 0;
   #endif
        csPortSet      = dcPortSet;
        csPortClr      = dcPortClr;
    }
    if(miso >= 0) {
        swspi.misoPort = portInputRegister(miso);
        #if !defined(KINETISK)
        swspi.misoPinMask = digitalPinToBitMask(miso);
        #endif
    } else {
        swspi.misoPort = portInputRegister(dc);
    }
  #else  // !CORE_TEENSY
    dcPinMask        =digitalPinToBitMask(dc);
    swspi.sckPinMask =digitalPinToBitMask(sck);
    swspi.mosiPinMask=digitalPinToBitMask(mosi);
    dcPortSet        =&(PORT->Group[g_APinDescription[dc].ulPort].OUTSET.reg);
    dcPortClr        =&(PORT->Group[g_APinDescription[dc].ulPort].OUTCLR.reg);
    swspi.sckPortSet =&(PORT->Group[g_APinDescription[sck].ulPort].OUTSET.reg);
    swspi.sckPortClr =&(PORT->Group[g_APinDescription[sck].ulPort].OUTCLR.reg);
    swspi.mosiPortSet=&(PORT->Group[g_APinDescription[mosi].ulPort].OUTSET.reg);
    swspi.mosiPortClr=&(PORT->Group[g_APinDescription[mosi].ulPort].OUTCLR.reg);
    if(cs >= 0) {
        csPinMask = digitalPinToBitMask(cs);
        csPortSet = &(PORT->Group[g_APinDescription[cs].ulPort].OUTSET.reg);
        csPortClr = &(PORT->Group[g_APinDescription[cs].ulPort].OUTCLR.reg);
    } else {
        // No chip-select line defined; might be permanently tied to GND.
        // Assign a valid GPIO register (though not used for CS), and an
        // empty pin bitmask...the nonsense bit-twiddling might be faster
        // than checking _cs and possibly branching.
        csPortSet = dcPortSet;
        csPortClr = dcPortClr;
        csPinMask = 0;
    }
    if(miso >= 0) {
        swspi.misoPinMask=digitalPinToBitMask(miso);
        swspi.misoPort   =(PORTreg_t)portInputRegister(digitalPinToPort(miso));
    } else {
        swspi.misoPinMask=0;
        swspi.misoPort   =(PORTreg_t)portInputRegister(digitalPinToPort(dc));
    }
  #endif // end !CORE_TEENSY
 #else  // !HAS_PORT_SET_CLR
    dcPort              =(PORTreg_t)portOutputRegister(digitalPinToPort(dc));
    dcPinMaskSet        =digitalPinToBitMask(dc);
    swspi.sckPort       =(PORTreg_t)portOutputRegister(digitalPinToPort(sck));
    swspi.sckPinMaskSet =digitalPinToBitMask(sck);
    swspi.mosiPort      =(PORTreg_t)portOutputRegister(digitalPinToPort(mosi));
    swspi.mosiPinMaskSet=digitalPinToBitMask(mosi);
    if(cs >= 0) {
        csPort       = (PORTreg_t)portOutputRegister(digitalPinToPort(cs));
        csPinMaskSet = digitalPinToBitMask(cs);
    } else {
        // No chip-select line defined; might be permanently tied to GND.
        // Assign a valid GPIO register (though not used for CS), and an
        // empty pin bitmask...the nonsense bit-twiddling might be faster
        // than checking _cs and possibly branching.
        csPort       = dcPort;
        csPinMaskSet = 0;
    }
    if(miso >= 0) {
        swspi.misoPort   =(PORTreg_t)portInputRegister(digitalPinToPort(miso));
        swspi.misoPinMask=digitalPinToBitMask(miso);
    } else {
        swspi.misoPort   =(PORTreg_t)portInputRegister(digitalPinToPort(dc));
        swspi.misoPinMask=0;
    }
    csPinMaskClr         = ~csPinMaskSet;
    dcPinMaskClr         = ~dcPinMaskSet;
    swspi.sckPinMaskClr  = ~swspi.sckPinMaskSet;
    swspi.mosiPinMaskClr = ~swspi.mosiPinMaskSet;
 #endif // !end HAS_PORT_SET_CLR
#endif // end USE_FAST_PINIO
}

/*!
    @brief   Adafruit_SPITFT constructor for hardware SPI using the board's
             default SPI peripheral.
    @param   w     Display width in pixels at default rotation setting (0).
    @param   h     Display height in pixels at default rotation setting (0).
    @param   cs    Arduino pin # for chip-select (-1 if unused, tie CS low).
    @param   dc    Arduino pin # for data/command select (required).
    @param   rst   Arduino pin # for display reset (optional, display reset
                   can be tied to MCU reset, default of -1 means unused).
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized; application typically will
             need to call subclass' begin() function, which in turn calls
             this library's initSPI() function to initialize pins.
*/
#if defined(ESP8266) // See notes below
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h, int8_t cs,
  int8_t dc, int8_t rst) : Adafruit_GFX(w, h),
  connection(TFT_HARD_SPI), _rst(rst), _cs(cs), _dc(dc) {
    hwspi._spi = &SPI;
}
#else  // !ESP8266
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h, int8_t cs,
	int8_t dc, int8_t rst) : Adafruit_SPITFT(w, h, &ILI9341_SPI_PORT, cs, dc, rst) {
    // This just invokes the hardware SPI constructor below,
    // passing the default SPI device (&SPI).
}
#endif // end !ESP8266

#if !defined(ESP8266)
// ESP8266 compiler freaks out at this constructor -- it can't disambiguate
// beteween the SPIClass pointer (argument #3) and a regular integer.
// Solution here it to just not offer this variant on the ESP8266. You can
// use the default hardware SPI peripheral, or you can use software SPI,
// but if there's any library out there that creates a 'virtual' SPIClass
// peripheral and drives it with software bitbanging, that's not supported.
/*!
    @brief   Adafruit_SPITFT constructor for hardware SPI using a specific
             SPI peripheral.
    @param   w         Display width in pixels at default rotation (0).
    @param   h         Display height in pixels at default rotation (0).
    @param   spiClass  Pointer to SPIClass type (e.g. &SPI or &SPI1).
    @param   cs        Arduino pin # for chip-select (-1 if unused, tie CS low).
    @param   dc        Arduino pin # for data/command select (required).
    @param   rst       Arduino pin # for display reset (optional, display reset
                       can be tied to MCU reset, default of -1 means unused).
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized in constructor; application
             typically will need to call subclass' begin() function, which
             in turn calls this library's initSPI() function to initialize
             pins. EXCEPT...if you have built your own SERCOM SPI peripheral
             (calling the SPIClass constructor) rather than one of the
             built-in SPI devices (e.g. &SPI, &SPI1 and so forth), you will
             need to call the begin() function for your object as well as
             pinPeripheral() for the MOSI, MISO and SCK pins to configure
             GPIO manually. Do this BEFORE calling the display-specific
             begin or init function. Unfortunate but unavoidable.
*/
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w,
	uint16_t h,
	SPI_HandleTypeDef *spiClass,
  int8_t cs, int8_t dc, int8_t rst) : Adafruit_GFX(w, h),
  connection(TFT_HARD_SPI), _rst(rst), _cs(cs), _dc(dc) {
    hwspi._spi = spiClass;
#if defined(USE_FAST_PINIO)
 #if defined(HAS_PORT_SET_CLR)
  #if defined(CORE_TEENSY)
   #if !defined(KINETISK)
    dcPinMask     = digitalPinToBitMask(dc);
   #endif
    dcPortSet     = portSetRegister(dc);
    dcPortClr     = portClearRegister(dc);
    if(cs >= 0) {
   #if !defined(KINETISK)
        csPinMask = digitalPinToBitMask(cs);
   #endif
        csPortSet = portSetRegister(cs);
        csPortClr = portClearRegister(cs);
    } else { // see comments below
   #if !defined(KINETISK)
        csPinMask = 0;
   #endif
        csPortSet = dcPortSet;
        csPortClr = dcPortClr;
    }
  #else  // !CORE_TEENSY
    dcPinMask     = digitalPinToBitMask(dc);
    dcPortSet     = &(PORT->Group[g_APinDescription[dc].ulPort].OUTSET.reg);
    dcPortClr     = &(PORT->Group[g_APinDescription[dc].ulPort].OUTCLR.reg);
    if(cs >= 0) {
        csPinMask = digitalPinToBitMask(cs);
        csPortSet = &(PORT->Group[g_APinDescription[cs].ulPort].OUTSET.reg);
        csPortClr = &(PORT->Group[g_APinDescription[cs].ulPort].OUTCLR.reg);
    } else {
        // No chip-select line defined; might be permanently tied to GND.
        // Assign a valid GPIO register (though not used for CS), and an
        // empty pin bitmask...the nonsense bit-twiddling might be faster
        // than checking _cs and possibly branching.
        csPortSet = dcPortSet;
        csPortClr = dcPortClr;
        csPinMask = 0;
    }
  #endif // end !CORE_TEENSY
 #else  // !HAS_PORT_SET_CLR
    dcPort           = (PORTreg_t)portOutputRegister(digitalPinToPort(dc));
    dcPinMaskSet     = digitalPinToBitMask(dc);
    if(cs >= 0) {
        csPort       = (PORTreg_t)portOutputRegister(digitalPinToPort(cs));
        csPinMaskSet = digitalPinToBitMask(cs);
    } else {
        // No chip-select line defined; might be permanently tied to GND.
        // Assign a valid GPIO register (though not used for CS), and an
        // empty pin bitmask...the nonsense bit-twiddling might be faster
        // than checking _cs and possibly branching.
        csPort       = dcPort;
        csPinMaskSet = 0;
    }
    csPinMaskClr = ~csPinMaskSet;
    dcPinMaskClr = ~dcPinMaskSet;
 #endif // end !HAS_PORT_SET_CLR
#endif // end USE_FAST_PINIO
}
#endif // end !ESP8266

/*!
    @brief   Adafruit_SPITFT constructor for parallel display connection.
    @param   w         Display width in pixels at default rotation (0).
    @param   h         Display height in pixels at default rotation (0).
    @param   busWidth  If tft16 (enumeration in header file), is a 16-bit
                       parallel connection, else 8-bit.
                       16-bit isn't fully implemented or tested yet so
                       applications should pass "tft8bitbus" for now...needed to
                       stick a required enum argument in there to
                       disambiguate this constructor from the soft-SPI case.
                       Argument is ignored on 8-bit architectures (no 'wide'
                       support there since PORTs are 8 bits anyway).
    @param   d0        Arduino pin # for data bit 0 (1+ are extrapolated).
                       The 8 (or 16) data bits MUST be contiguous and byte-
                       aligned (or word-aligned for wide interface) within
                       the same PORT register (might not correspond to
                       Arduino pin sequence).
    @param   wr        Arduino pin # for write strobe (required).
    @param   dc        Arduino pin # for data/command select (required).
    @param   cs        Arduino pin # for chip-select (optional, -1 if unused,
                       tie CS low).
    @param   rst       Arduino pin # for display reset (optional, display reset
                       can be tied to MCU reset, default of -1 means unused).
    @param   rd        Arduino pin # for read strobe (optional, -1 if unused).
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized; application typically will need
             to call subclass' begin() function, which in turn calls this
             library's initSPI() function to initialize pins.
             Yes, the name is a misnomer...this library originally handled
             only SPI displays, parallel being a recent addition (but not
             wanting to break existing code).
*/
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h, tftBusWidth busWidth,
  int8_t d0, int8_t wr, int8_t dc, int8_t cs, int8_t rst, int8_t rd) :
  Adafruit_GFX(w, h), connection(TFT_PARALLEL), _rst(rst), _cs(cs), _dc(dc) {
    tft8._d0  = d0;
    tft8._wr  = wr;
    tft8._rd  = rd;
    tft8.wide = (busWidth == tft16bitbus);
#if defined(USE_FAST_PINIO)
 #if defined(HAS_PORT_SET_CLR)
  #if defined(CORE_TEENSY)
    tft8.wrPortSet = portSetRegister(wr);
    tft8.wrPortClr = portClearRegister(wr);
   #if !defined(KINETISK)
    dcPinMask      = digitalPinToBitMask(dc);
   #endif
    dcPortSet      = portSetRegister(dc);
    dcPortClr      = portClearRegister(dc);
    if(cs >= 0) {
   #if !defined(KINETISK)
        csPinMask  = digitalPinToBitMask(cs);
   #endif
        csPortSet  = portSetRegister(cs);
        csPortClr  = portClearRegister(cs);
    } else { // see comments below
   #if !defined(KINETISK)
        csPinMask  = 0;
   #endif
        csPortSet  = dcPortSet;
        csPortClr  = dcPortClr;
    }
    if(rd >= 0) { // if read-strobe pin specified...
   #if defined(KINETISK)
        tft8.rdPinMask = 1;
   #else  // !KINETISK
        tft8.rdPinMask = digitalPinToBitMask(rd);
   #endif
        tft8.rdPortSet = portSetRegister(rd);
        tft8.rdPortClr = portClearRegister(rd);
    } else {
        tft8.rdPinMask = 0;
        tft8.rdPortSet = dcPortSet;
        tft8.rdPortClr = dcPortClr;
    }
    // These are all uint8_t* pointers -- elsewhere they're recast
    // as necessary if a 'wide' 16-bit interface is in use.
    tft8.writePort = portOutputRegister(d0);
    tft8.readPort  = portInputRegister(d0);
    tft8.dirSet    = portModeRegister(d0);
    tft8.dirClr    = portModeRegister(d0);
  #else  // !CORE_TEENSY
    tft8.wrPinMask = digitalPinToBitMask(wr);
    tft8.wrPortSet = &(PORT->Group[g_APinDescription[wr].ulPort].OUTSET.reg);
    tft8.wrPortClr = &(PORT->Group[g_APinDescription[wr].ulPort].OUTCLR.reg);
    dcPinMask      = digitalPinToBitMask(dc);
    dcPortSet      = &(PORT->Group[g_APinDescription[dc].ulPort].OUTSET.reg);
    dcPortClr      = &(PORT->Group[g_APinDescription[dc].ulPort].OUTCLR.reg);
    if(cs >= 0) {
        csPinMask  = digitalPinToBitMask(cs);
        csPortSet  = &(PORT->Group[g_APinDescription[cs].ulPort].OUTSET.reg);
        csPortClr  = &(PORT->Group[g_APinDescription[cs].ulPort].OUTCLR.reg);
    } else {
        // No chip-select line defined; might be permanently tied to GND.
        // Assign a valid GPIO register (though not used for CS), and an
        // empty pin bitmask...the nonsense bit-twiddling might be faster
        // than checking _cs and possibly branching.
        csPortSet  = dcPortSet;
        csPortClr  = dcPortClr;
        csPinMask  = 0;
    }
    if(rd >= 0) { // if read-strobe pin specified...
        tft8.rdPinMask =digitalPinToBitMask(rd);
        tft8.rdPortSet =&(PORT->Group[g_APinDescription[rd].ulPort].OUTSET.reg);
        tft8.rdPortClr =&(PORT->Group[g_APinDescription[rd].ulPort].OUTCLR.reg);
    } else {
        tft8.rdPinMask = 0;
        tft8.rdPortSet = dcPortSet;
        tft8.rdPortClr = dcPortClr;
    }
    // Get pointers to PORT write/read/dir bytes within 32-bit PORT
    uint8_t       dBit    = g_APinDescription[d0].ulPin; // d0 bit # in PORT
    PortGroup    *p       = (&(PORT->Group[g_APinDescription[d0].ulPort]));
    uint8_t       offset  = dBit / 8; // d[7:0] byte # within PORT
    if(tft8.wide) offset &= ~1; // d[15:8] byte # within PORT
    // These are all uint8_t* pointers -- elsewhere they're recast
    // as necessary if a 'wide' 16-bit interface is in use.
    tft8.writePort    = (volatile uint8_t *)&(p->OUT.reg)    + offset;
    tft8.readPort     = (volatile uint8_t *)&(p->IN.reg)     + offset;
    tft8.dirSet       = (volatile uint8_t *)&(p->DIRSET.reg) + offset;
    tft8.dirClr       = (volatile uint8_t *)&(p->DIRCLR.reg) + offset;
  #endif // end !CORE_TEENSY
 #else  // !HAS_PORT_SET_CLR
    tft8.wrPort       = (PORTreg_t)portOutputRegister(digitalPinToPort(wr));
    tft8.wrPinMaskSet = digitalPinToBitMask(wr);
    dcPort            = (PORTreg_t)portOutputRegister(digitalPinToPort(dc));
    dcPinMaskSet      = digitalPinToBitMask(dc);
    if(cs >= 0) {
        csPort        = (PORTreg_t)portOutputRegister(digitalPinToPort(cs));
        csPinMaskSet  = digitalPinToBitMask(cs);
    } else {
        // No chip-select line defined; might be permanently tied to GND.
        // Assign a valid GPIO register (though not used for CS), and an
        // empty pin bitmask...the nonsense bit-twiddling might be faster
        // than checking _cs and possibly branching.
        csPort        = dcPort;
        csPinMaskSet  = 0;
    }
    if(rd >= 0) { // if read-strobe pin specified...
        tft8.rdPort       =(PORTreg_t)portOutputRegister(digitalPinToPort(rd));
        tft8.rdPinMaskSet =digitalPinToBitMask(rd);
    } else {
        tft8.rdPort       = dcPort;
        tft8.rdPinMaskSet = 0;
    }
    csPinMaskClr      = ~csPinMaskSet;
    dcPinMaskClr      = ~dcPinMaskSet;
    tft8.wrPinMaskClr = ~tft8.wrPinMaskSet;
    tft8.rdPinMaskClr = ~tft8.rdPinMaskSet;
    tft8.writePort    = (PORTreg_t)portOutputRegister(digitalPinToPort(d0));
    tft8.readPort     = (PORTreg_t)portInputRegister(digitalPinToPort(d0));
    tft8.portDir      = (PORTreg_t)portModeRegister(digitalPinToPort(d0));
 #endif // end !HAS_PORT_SET_CLR
#endif // end USE_FAST_PINIO
}

// end constructors -------


// CLASS MEMBER FUNCTIONS --------------------------------------------------

// begin() and setAddrWindow() MUST be declared by any subclass.

/*!
    @brief  Configure microcontroller pins for TFT interfacing. Typically
            called by a subclass' begin() function.
    @param  freq     SPI frequency when using hardware SPI. If default (0)
                     is passed, will fall back on a device-specific value.
                     Value is ignored when using software SPI or parallel
                     connection.
    @param  spiMode  SPI mode when using hardware SPI. MUST be one of the
                     values SPI_MODE0, SPI_MODE1, SPI_MODE2 or SPI_MODE3
                     defined in SPI.h. Do NOT attempt to pass '0' for
                     SPI_MODE0 and so forth...the values are NOT the same!
                     Use ONLY the defines! (Pity it's not an enum.)
    @note   Another anachronistically-named function; this is called even
            when the display connection is parallel (not SPI). Also, this
            could probably be made private...quite a few class functions
            were generously put in the public section.
*/
void Adafruit_SPITFT::initSPI(uint32_t freq, uint8_t spiMode) {

    if(!freq) freq = DEFAULT_SPI_FREQ; // If no freq specified, use default

    // Init basic control pins common to all connection types
    /*if(_cs >= 0) {
        pinMode(_cs, OUTPUT);
        digitalWrite(_cs, HIGH); // Deselect
    }
    pinMode(_dc, OUTPUT);
    digitalWrite(_dc, HIGH); // Data mode*/

    

    if(_rst >= 0) {
        // Toggle _rst low to reset
        HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_SET);
	    HAL_Delay(100);
		HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_RESET);
	    HAL_Delay(100);
	    HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_SET);
	    HAL_Delay(100);
    }


}

/*!
    @brief  Call before issuing command(s) or data to display. Performs
            chip-select (if required) and starts an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
void Adafruit_SPITFT::startWrite(void) {
    SPI_BEGIN_TRANSACTION();
    if(_cs >= 0) SPI_CS_LOW();
}

/*!
    @brief  Call after issuing command(s) or data to display. Performs
            chip-deselect (if required) and ends an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
void Adafruit_SPITFT::endWrite(void) {
    if(_cs >= 0) SPI_CS_HIGH();
    SPI_END_TRANSACTION();
}


// -------------------------------------------------------------------------
// Lower-level graphics operations. These functions require a chip-select
// and/or SPI transaction around them (via startWrite(), endWrite() above).
// Higher-level graphics primitives might start a single transaction and
// then make multiple calls to these functions (e.g. circle or text
// rendering might make repeated lines or rects) before ending the
// transaction. It's more efficient than starting a transaction every time.

/*!
    @brief  Draw a single pixel to the display at requested coordinates.
            Not self-contained; should follow a startWrite() call.
    @param  x      Horizontal position (0 = left).
    @param  y      Vertical position   (0 = top).
    @param  color  16-bit pixel color in '565' RGB format.
*/
void Adafruit_SPITFT::writePixel(int16_t x, int16_t y, uint16_t color) {
    if((x >= 0) && (x < _width) && (y >= 0) && (y < _height)) {
        setAddrWindow(x, y, 1, 1);
        SPI_WRITE16(color);
    }
}

/*!
    @brief  Issue a series of pixels from memory to the display. Not self-
            contained; should follow startWrite() and setAddrWindow() calls.
    @param  colors     Pointer to array of 16-bit pixel values in '565' RGB
                       format.
    @param  len        Number of elements in 'colors' array.
    @param  block      If true (default case if unspecified), function blocks
                       until DMA transfer is complete. This is simply IGNORED
                       if DMA is not enabled. If false, the function returns
                       immediately after the last DMA transfer is started,
                       and one should use the dmaWait() function before
                       doing ANY other display-related activities (or even
                       any SPI-related activities, if using an SPI display
                       that shares the bus with other devices).
    @param  bigEndian  If using DMA, and if set true, bitmap in memory is in
                       big-endian order (most significant byte first). By
                       default this is false, as most microcontrollers seem
                       to be little-endian and 16-bit pixel values must be
                       byte-swapped before issuing to the display (which tend
                       to be big-endian when using SPI or 8-bit parallel).
                       If an application can optimize around this -- for
                       example, a bitmap in a uint16_t array having the byte
                       values already reordered big-endian, this can save
                       some processing time here, ESPECIALLY if using this
                       function's non-blocking DMA mode. Not all cases are
                       covered...this is really here only for SAMD DMA and
                       much forethought on the application side.
*/
void Adafruit_SPITFT::writePixels(uint16_t *colors, uint32_t len,
  bool block, bool bigEndian) {

    if(!len) return; // Avoid 0-byte transfers

#if defined(ESP32) // ESP32 has a special SPI pixel-writing function...
    if(connection == TFT_HARD_SPI) {
        hwspi._spi->writePixels(colors, len * 2);
        return;
    }
#elif defined(USE_SPI_DMA)
    if((connection == TFT_HARD_SPI) || (connection == TFT_PARALLEL)) {
        int     maxSpan     = maxFillLen / 2; // One scanline max
        uint8_t pixelBufIdx = 0;              // Active pixel buffer number
 #if defined(__SAMD51__)
        if(connection == TFT_PARALLEL) {
            // Switch WR pin to PWM or CCL
            pinPeripheral(tft8._wr, wrPeripheral);
        }
 #endif // end __SAMD51__
        if(!bigEndian) { // Normal little-endian situation...
            while(len) {
                int count = (len < maxSpan) ? len : maxSpan;

                // Because TFT and SAMD endianisms are different, must swap
                // bytes from the 'colors' array passed into a DMA working
                // buffer. This can take place while the prior DMA transfer
                // is in progress, hence the need for two pixelBufs.
                for(int i=0; i<count; i++) {
                    pixelBuf[pixelBufIdx][i] = __builtin_bswap16(*colors++);
                }
                // The transfers themselves are relatively small, so we don't
                // need a long descriptor list. We just alternate between the
                // first two, sharing pixelBufIdx for that purpose.
                descriptor[pixelBufIdx].SRCADDR.reg       =
                  (uint32_t)pixelBuf[pixelBufIdx] + count * 2;
                descriptor[pixelBufIdx].BTCTRL.bit.SRCINC = 1;
                descriptor[pixelBufIdx].BTCNT.reg         = count * 2;
                descriptor[pixelBufIdx].DESCADDR.reg      = 0;

                while(dma_busy); // Wait for prior line to finish

                // Move new descriptor into place...
                memcpy(dptr, &descriptor[pixelBufIdx], sizeof(DmacDescriptor));
                dma_busy = true;
                dma.startJob();                // Trigger SPI DMA transfer
                if(connection == TFT_PARALLEL) dma.trigger();
                pixelBufIdx = 1 - pixelBufIdx; // Swap DMA pixel buffers

                len -= count;
            }
        } else { // bigEndian == true
            // With big-endian pixel data, this can be handled as a single
            // DMA transfer using chained descriptors. Even full screen, this
            // needs only a relatively short descriptor list, each
            // transferring a max of 32,767 (not 32,768) pixels. The list
            // was allocated large enough to accommodate a full screen's
            // worth of data, so this won't run past the end of the list.
            int d, numDescriptors = (len + 32766) / 32767;
            for(d=0; d<numDescriptors; d++) {
                int count = (len < 32767) ? len : 32767;
                descriptor[d].SRCADDR.reg       = (uint32_t)colors + count * 2;
                descriptor[d].BTCTRL.bit.SRCINC = 1;
                descriptor[d].BTCNT.reg         = count * 2;
                descriptor[d].DESCADDR.reg      = (uint32_t)&descriptor[d+1];
                len    -= count;
                colors += count;
            }
            descriptor[d-1].DESCADDR.reg        = 0;

            while(dma_busy); // Wait for prior transfer (if any) to finish

            // Move first descriptor into place and start transfer...
            memcpy(dptr, &descriptor[0], sizeof(DmacDescriptor));
            dma_busy = true;
            dma.startJob();                // Trigger SPI DMA transfer
            if(connection == TFT_PARALLEL) dma.trigger();
        } // end bigEndian

        lastFillColor = 0x0000; // pixelBuf has been sullied
        lastFillLen   = 0;
        if(block) {
            while(dma_busy);    // Wait for last line to complete
 #if defined(__SAMD51__) || defined(_SAMD21_)
            if(connection == TFT_HARD_SPI) {
                // See SAMD51/21 note in writeColor()
                hwspi._spi->setDataMode(hwspi._mode);
            } else {
                pinPeripheral(tft8._wr, PIO_OUTPUT); // Switch WR back to GPIO
            }
 #endif // end __SAMD51__ || _SAMD21_
        }
        return;
    }
#endif // end USE_SPI_DMA

    // All other cases (bitbang SPI or non-DMA hard SPI or parallel),
    // use a loop with the normal 16-bit data write function:
    while(len--) {
        SPI_WRITE16(*colors++);
    }
}

/*!
    @brief  Wait for the last DMA transfer in a prior non-blocking
            writePixels() call to complete. This does nothing if DMA
            is not enabled, and is not needed if blocking writePixels()
            was used (as is the default case).
*/
void Adafruit_SPITFT::dmaWait(void) {
#if defined(USE_SPI_DMA)
    while(dma_busy);
 #if defined(__SAMD51__) || defined(_SAMD21_)
    if(connection == TFT_HARD_SPI) {
        // See SAMD51/21 note in writeColor()
        hwspi._spi->setDataMode(hwspi._mode);
    } else {
        pinPeripheral(tft8._wr, PIO_OUTPUT); // Switch WR back to GPIO
    }
 #endif // end __SAMD51__ || _SAMD21_
#endif
}

void Adafruit_SPITFT::WriteData(uint8_t* buff, size_t buff_size) {
	HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_SET);

	// split data in small chunks because HAL can't send more then 64K at once
	while(buff_size > 0) {
		uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
		HAL_SPI_Transmit(&ILI9341_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
		buff += chunk_size;
		buff_size -= chunk_size;
	}
}

/*!
    @brief  Issue a series of pixels, all the same color. Not self-
            contained; should follow startWrite() and setAddrWindow() calls.
    @param  color  16-bit pixel color in '565' RGB format.
    @param  len    Number of pixels to draw.
*/
void Adafruit_SPITFT::writeColor(uint16_t color, uint32_t len) {

    if(!len) return; // Avoid 0-byte transfers

    uint8_t hi = color >> 8, lo = color;

        while(len--) {
 
	        uint8_t data[] = { color >> 8, color & 0xFF };
	        WriteData(data, sizeof(data));
	        


        }

    
}

/*!
    @brief  Draw a filled rectangle to the display. Not self-contained;
            should follow startWrite(). Typically used by higher-level
            graphics primitives; user code shouldn't need to call this and
            is likely to use the self-contained fillRect() instead.
            writeFillRect() performs its own edge clipping and rejection;
            see writeFillRectPreclipped() for a more 'raw' implementation.
    @param  x      Horizontal position of first corner.
    @param  y      Vertical position of first corner.
    @param  w      Rectangle width in pixels (positive = right of first
                   corner, negative = left of first corner).
    @param  h      Rectangle height in pixels (positive = below first
                   corner, negative = above first corner).
    @param  color  16-bit fill color in '565' RGB format.
    @note   Written in this deep-nested way because C by definition will
            optimize for the 'if' case, not the 'else' -- avoids branches
            and rejects clipped rectangles at the least-work possibility.
*/
void Adafruit_SPITFT::writeFillRect(int16_t x, int16_t y,
  int16_t w, int16_t h, uint16_t color) {
    if(w && h) {                            // Nonzero width and height?
        if(w < 0) {                         // If negative width...
            x +=  w + 1;                    //   Move X to left edge
            w  = -w;                        //   Use positive width
        }
        if(x < _width) {                    // Not off right
            if(h < 0) {                     // If negative height...
                y +=  h + 1;                //   Move Y to top edge
                h  = -h;                    //   Use positive height
            }
            if(y < _height) {               // Not off bottom
                int16_t x2 = x + w - 1;
                if(x2 >= 0) {               // Not off left
                    int16_t y2 = y + h - 1;
                    if(y2 >= 0) {           // Not off top
                        // Rectangle partly or fully overlaps screen
                        if(x  <  0)       { x = 0; w = x2 + 1; } // Clip left
                        if(y  <  0)       { y = 0; h = y2 + 1; } // Clip top
                        if(x2 >= _width)  { w = _width  - x;   } // Clip right
                        if(y2 >= _height) { h = _height - y;   } // Clip bottom
                        writeFillRectPreclipped(x, y, w, h, color);
                    }
                }
            }
        }
    }
}

/*!
    @brief  Draw a horizontal line on the display. Performs edge clipping
            and rejection. Not self-contained; should follow startWrite().
            Typically used by higher-level graphics primitives; user code
            shouldn't need to call this and is likely to use the self-
            contained drawFastHLine() instead.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  w      Line width in pixels (positive = right of first point,
                   negative = point of first corner).
    @param  color  16-bit line color in '565' RGB format.
*/
void inline Adafruit_SPITFT::writeFastHLine(int16_t x, int16_t y, int16_t w,
  uint16_t color) {
    if((y >= 0) && (y < _height) && w) { // Y on screen, nonzero width
        if(w < 0) {                      // If negative width...
            x +=  w + 1;                 //   Move X to left edge
            w  = -w;                     //   Use positive width
        }
        if(x < _width) {                 // Not off right
            int16_t x2 = x + w - 1;
            if(x2 >= 0) {                // Not off left
                // Line partly or fully overlaps screen
                if(x  <  0)       { x = 0; w = x2 + 1; } // Clip left
                if(x2 >= _width)  { w = _width  - x;   } // Clip right
                writeFillRectPreclipped(x, y, w, 1, color);
            }
        }
    }
}

/*!
    @brief  Draw a vertical line on the display. Performs edge clipping and
            rejection. Not self-contained; should follow startWrite().
            Typically used by higher-level graphics primitives; user code
            shouldn't need to call this and is likely to use the self-
            contained drawFastVLine() instead.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  h      Line height in pixels (positive = below first point,
                   negative = above first point).
    @param  color  16-bit line color in '565' RGB format.
*/
void inline Adafruit_SPITFT::writeFastVLine(int16_t x, int16_t y, int16_t h,
  uint16_t color) {
    if((x >= 0) && (x < _width) && h) { // X on screen, nonzero height
        if(h < 0) {                     // If negative height...
            y +=  h + 1;                //   Move Y to top edge
            h  = -h;                    //   Use positive height
        }
        if(y < _height) {               // Not off bottom
            int16_t y2 = y + h - 1;
            if(y2 >= 0) {               // Not off top
                // Line partly or fully overlaps screen
                if(y  <  0)       { y = 0; h = y2 + 1; } // Clip top
                if(y2 >= _height) { h = _height - y;   } // Clip bottom
                writeFillRectPreclipped(x, y, 1, h, color);
            }
        }
    }
}

/*!
    @brief  A lower-level version of writeFillRect(). This version requires
            all inputs are in-bounds, that width and height are positive,
            and no part extends offscreen. NO EDGE CLIPPING OR REJECTION IS
            PERFORMED. If higher-level graphics primitives are written to
            handle their own clipping earlier in the drawing process, this
            can avoid unnecessary function calls and repeated clipping
            operations in the lower-level functions.
    @param  x      Horizontal position of first corner. MUST BE WITHIN
                   SCREEN BOUNDS.
    @param  y      Vertical position of first corner. MUST BE WITHIN SCREEN
                   BOUNDS.
    @param  w      Rectangle width in pixels. MUST BE POSITIVE AND NOT
                   EXTEND OFF SCREEN.
    @param  h      Rectangle height in pixels. MUST BE POSITIVE AND NOT
                   EXTEND OFF SCREEN.
    @param  color  16-bit fill color in '565' RGB format.
    @note   This is a new function, no graphics primitives besides rects
            and horizontal/vertical lines are written to best use this yet.
*/
inline void Adafruit_SPITFT::writeFillRectPreclipped(int16_t x, int16_t y,
  int16_t w, int16_t h, uint16_t color) {
    setAddrWindow(x, y, w, h);
    writeColor(color, (uint32_t)w * h);
}


// -------------------------------------------------------------------------
// Ever-so-slightly higher-level graphics operations. Similar to the 'write'
// functions above, but these contain their own chip-select and SPI
// transactions as needed (via startWrite(), endWrite()). They're typically
// used solo -- as graphics primitives in themselves, not invoked by higher-
// level primitives (which should use the functions above for better
// performance).

/*!
    @brief  Draw a single pixel to the display at requested coordinates.
            Self-contained and provides its own transaction as needed
            (see writePixel(x,y,color) for a lower-level variant).
            Edge clipping is performed here.
    @param  x      Horizontal position (0 = left).
    @param  y      Vertical position   (0 = top).
    @param  color  16-bit pixel color in '565' RGB format.
*/
void Adafruit_SPITFT::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Clip first...
    if((x >= 0) && (x < _width) && (y >= 0) && (y < _height)) {
        // THEN set up transaction (if needed) and draw...
        startWrite();
        setAddrWindow(x, y, 1, 1);
        SPI_WRITE16(color);
        endWrite();
    }
}

/*!
    @brief  Draw a filled rectangle to the display. Self-contained and
            provides its own transaction as needed (see writeFillRect() or
            writeFillRectPreclipped() for lower-level variants). Edge
            clipping and rejection is performed here.
    @param  x      Horizontal position of first corner.
    @param  y      Vertical position of first corner.
    @param  w      Rectangle width in pixels (positive = right of first
                   corner, negative = left of first corner).
    @param  h      Rectangle height in pixels (positive = below first
                   corner, negative = above first corner).
    @param  color  16-bit fill color in '565' RGB format.
    @note   This repeats the writeFillRect() function almost in its entirety,
            with the addition of a transaction start/end. It's done this way
            (rather than starting the transaction and calling writeFillRect()
            to handle clipping and so forth) so that the transaction isn't
            performed at all if the rectangle is rejected. It's really not
            that much code.
*/
void Adafruit_SPITFT::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
  uint16_t color) {
    if(w && h) {                            // Nonzero width and height?
        if(w < 0) {                         // If negative width...
            x +=  w + 1;                    //   Move X to left edge
            w  = -w;                        //   Use positive width
        }
        if(x < _width) {                    // Not off right
            if(h < 0) {                     // If negative height...
                y +=  h + 1;                //   Move Y to top edge
                h  = -h;                    //   Use positive height
            }
            if(y < _height) {               // Not off bottom
                int16_t x2 = x + w - 1;
                if(x2 >= 0) {               // Not off left
                    int16_t y2 = y + h - 1;
                    if(y2 >= 0) {           // Not off top
                        // Rectangle partly or fully overlaps screen
                        if(x  <  0)       { x = 0; w = x2 + 1; } // Clip left
                        if(y  <  0)       { y = 0; h = y2 + 1; } // Clip top
                        if(x2 >= _width)  { w = _width  - x;   } // Clip right
                        if(y2 >= _height) { h = _height - y;   } // Clip bottom
                        startWrite();
                        writeFillRectPreclipped(x, y, w, h, color);
                        endWrite();
                    }
                }
            }
        }
    }
}

/*!
    @brief  Draw a horizontal line on the display. Self-contained and
            provides its own transaction as needed (see writeFastHLine() for
            a lower-level variant). Edge clipping and rejection is performed
            here.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  w      Line width in pixels (positive = right of first point,
                   negative = point of first corner).
    @param  color  16-bit line color in '565' RGB format.
    @note   This repeats the writeFastHLine() function almost in its
            entirety, with the addition of a transaction start/end. It's
            done this way (rather than starting the transaction and calling
            writeFastHLine() to handle clipping and so forth) so that the
            transaction isn't performed at all if the line is rejected.
*/
void Adafruit_SPITFT::drawFastHLine(int16_t x, int16_t y, int16_t w,
  uint16_t color) {
    if((y >= 0) && (y < _height) && w) { // Y on screen, nonzero width
        if(w < 0) {                      // If negative width...
            x +=  w + 1;                 //   Move X to left edge
            w  = -w;                     //   Use positive width
        }
        if(x < _width) {                 // Not off right
            int16_t x2 = x + w - 1;
            if(x2 >= 0) {                // Not off left
                // Line partly or fully overlaps screen
                if(x  <  0)       { x = 0; w = x2 + 1; } // Clip left
                if(x2 >= _width)  { w = _width  - x;   } // Clip right
                startWrite();
                writeFillRectPreclipped(x, y, w, 1, color);
                endWrite();
            }
        }
    }
}

/*!
    @brief  Draw a vertical line on the display. Self-contained and provides
            its own transaction as needed (see writeFastHLine() for a lower-
            level variant). Edge clipping and rejection is performed here.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  h      Line height in pixels (positive = below first point,
                   negative = above first point).
    @param  color  16-bit line color in '565' RGB format.
    @note   This repeats the writeFastVLine() function almost in its
            entirety, with the addition of a transaction start/end. It's
            done this way (rather than starting the transaction and calling
            writeFastVLine() to handle clipping and so forth) so that the
            transaction isn't performed at all if the line is rejected.
*/
void Adafruit_SPITFT::drawFastVLine(int16_t x, int16_t y, int16_t h,
  uint16_t color) {
    if((x >= 0) && (x < _width) && h) { // X on screen, nonzero height
        if(h < 0) {                     // If negative height...
            y +=  h + 1;                //   Move Y to top edge
            h  = -h;                    //   Use positive height
        }
        if(y < _height) {               // Not off bottom
            int16_t y2 = y + h - 1;
            if(y2 >= 0) {               // Not off top
                // Line partly or fully overlaps screen
                if(y  <  0)       { y = 0; h = y2 + 1; } // Clip top
                if(y2 >= _height) { h = _height - y;   } // Clip bottom
                startWrite();
                writeFillRectPreclipped(x, y, 1, h, color);
                endWrite();
            }
        }
    }
}

/*!
    @brief  Essentially writePixel() with a transaction around it. I don't
            think this is in use by any of our code anymore (believe it was
            for some older BMP-reading examples), but is kept here in case
            any user code relies on it. Consider it DEPRECATED.
    @param  color  16-bit pixel color in '565' RGB format.
*/
void Adafruit_SPITFT::pushColor(uint16_t color) {
    //startWrite();
    SPI_WRITE16(color);
    //endWrite();
}



/*!
    @brief  Draw a 16-bit image (565 RGB) at the specified (x,y) position.
            For 16-bit display devices; no color reduction performed.
            Adapted from https://github.com/PaulStoffregen/ILI9341_t3
            by Marc MERLIN. See examples/pictureEmbed to use this.
            5/6/2017: function name and arguments have changed for
            compatibility with current GFX library and to avoid naming
            problems in prior implementation.  Formerly drawBitmap() with
            arguments in different order. Handles its own transaction and
            edge clipping/rejection.
    @param  x        Top left corner horizontal coordinate.
    @param  y        Top left corner vertical coordinate.
    @param  pcolors  Pointer to 16-bit array of pixel values.
    @param  w        Width of bitmap in pixels.
    @param  h        Height of bitmap in pixels.
*/
void Adafruit_SPITFT::drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *pcolors, int16_t w, int16_t h) {

    int16_t x2, y2; // Lower-right coord
    if(( x             >= _width ) ||      // Off-edge right
       ( y             >= _height) ||      // " top
       ((x2 = (x+w-1)) <  0      ) ||      // " left
       ((y2 = (y+h-1)) <  0)     ) return; // " bottom

    int16_t bx1=0, by1=0, // Clipped top-left within bitmap
            saveW=w;      // Save original bitmap width value
    if(x < 0) { // Clip left
        w  +=  x;
        bx1 = -x;
        x   =  0;
    }
    if(y < 0) { // Clip top
        h  +=  y;
        by1 = -y;
        y   =  0;
    }
    if(x2 >= _width ) w = _width  - x; // Clip right
    if(y2 >= _height) h = _height - y; // Clip bottom

    pcolors += by1 * saveW + bx1; // Offset bitmap ptr to clipped top-left
    startWrite();
    setAddrWindow(x, y, w, h); // Clipped area
    while(h--) { // For each (clipped) scanline...
      writePixels(pcolors, w); // Push one (clipped) row
      pcolors += saveW; // Advance pointer by one full (unclipped) line
    }
    endWrite();
}


// -------------------------------------------------------------------------
// Miscellaneous class member functions that don't draw anything.

/*!
    @brief  Invert the colors of the display (if supported by hardware).
            Self-contained, no transaction setup required.
    @param  i  true = inverted display, false = normal display.
*/
void Adafruit_SPITFT::invertDisplay(bool i) {
    startWrite();
    writeCommand(i ? invertOnCommand : invertOffCommand);
    endWrite();
}

/*!
    @brief   Given 8-bit red, green and blue values, return a 'packed'
             16-bit color value in '565' RGB format (5 bits red, 6 bits
             green, 5 bits blue). This is just a mathematical operation,
             no hardware is touched.
    @param   red    8-bit red brightnesss (0 = off, 255 = max).
    @param   green  8-bit green brightnesss (0 = off, 255 = max).
    @param   blue   8-bit blue brightnesss (0 = off, 255 = max).
    @return  'Packed' 16-bit color value (565 format).
*/
uint16_t Adafruit_SPITFT::color565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

/*!
 @brief   Adafruit_SPITFT Send Command handles complete sending of commands and data
 @param   commandByte       The Command Byte
 @param   dataBytes         A pointer to the Data bytes to send
 @param   numDataBytes      The number of bytes we should send
 */
void Adafruit_SPITFT::sendCommand(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes) {
    SPI_BEGIN_TRANSACTION();
    if(_cs >= 0) SPI_CS_LOW();
  
    SPI_DC_LOW(); // Command mode
    spiWrite(commandByte); // Send the command byte
  
    SPI_DC_HIGH();
    for (int i=0; i<numDataBytes; i++) {
      spiWrite(*dataBytes); // Send the data bytes
      dataBytes++;
    }
  
    if(_cs >= 0) SPI_CS_HIGH();
    SPI_END_TRANSACTION();
}

/*!
 @brief   Adafruit_SPITFT Send Command handles complete sending of commands and const data
 @param   commandByte       The Command Byte
 @param   dataBytes         A pointer to the Data bytes to send
 @param   numDataBytes      The number of bytes we should send
 */
void Adafruit_SPITFT::sendCommand(uint8_t commandByte, const uint8_t *dataBytes, uint8_t numDataBytes) {
    SPI_BEGIN_TRANSACTION();
    if(_cs >= 0) SPI_CS_LOW();
  
    SPI_DC_LOW(); // Command mode
    spiWrite(commandByte); // Send the command byte
  
    SPI_DC_HIGH();
    for (int i=0; i<numDataBytes; i++) {
      //spiWrite(pgm_read_byte(dataBytes++)); // Send the data bytes
    }
  
    if(_cs >= 0) SPI_CS_HIGH();
    SPI_END_TRANSACTION();
}

/*!
 @brief   Read 8 bits of data from display configuration memory (not RAM).
 This is highly undocumented/supported and should be avoided,
 function is only included because some of the examples use it.
 @param   commandByte
 The command register to read data from.
 @param   index
 The byte index into the command to read from.
 @return  Unsigned 8-bit data read from display register.
 */
/**************************************************************************/
uint8_t Adafruit_SPITFT::readcommand8(uint8_t commandByte, uint8_t index) {
  uint8_t result;
  startWrite();
  SPI_DC_LOW();     // Command mode
  spiWrite(commandByte);
  SPI_DC_HIGH();    // Data mode
  do {
    result = spiRead();
  } while(index--); // Discard bytes up to index'th
  endWrite();
  return result;
}

// -------------------------------------------------------------------------
// Lowest-level hardware-interfacing functions. Many of these are inline and
// compile to different things based on #defines -- typically just a few
// instructions. Others, not so much, those are not inlined.

/*!
    @brief  Start an SPI transaction if using the hardware SPI interface to
            the display. If using an earlier version of the Arduino platform
            (before the addition of SPI transactions), this instead attempts
            to set up the SPI clock and mode. No action is taken if the
            connection is not hardware SPI-based. This does NOT include a
            chip-select operation -- see startWrite() for a function that
            encapsulated both actions.
*/
inline void Adafruit_SPITFT::SPI_BEGIN_TRANSACTION(void) {
   /* if(connection == TFT_HARD_SPI) {
#if defined(SPI_HAS_TRANSACTION)
        hwspi._spi->beginTransaction(hwspi.settings);
#else // No transactions, configure SPI manually...
 #if defined(__AVR__) || defined(TEENSYDUINO) || defined(ARDUINO_ARCH_STM32F1)
        hwspi._spi->setClockDivider(SPI_CLOCK_DIV2);
 #elif defined(__arm__)
        hwspi._spi->setClockDivider(11);
 #elif defined(ESP8266) || defined(ESP32)
        hwspi._spi->setFrequency(hwspi._freq);
 #elif defined(RASPI) || defined(ARDUINO_ARCH_STM32F1)
        hwspi._spi->setClock(hwspi._freq);
 #endif
        hwspi._spi->setBitOrder(MSBFIRST);
        hwspi._spi->setDataMode(hwspi._mode);
#endif // end !SPI_HAS_TRANSACTION
    }*/
}

/*!
    @brief  End an SPI transaction if using the hardware SPI interface to
            the display. No action is taken if the connection is not
            hardware SPI-based or if using an earlier version of the Arduino
            platform (before the addition of SPI transactions). This does
            NOT include a chip-deselect operation -- see endWrite() for a
            function that encapsulated both actions.
*/
inline void Adafruit_SPITFT::SPI_END_TRANSACTION(void) {
#if defined(SPI_HAS_TRANSACTION)
    if(connection == TFT_HARD_SPI) {
        hwspi._spi->endTransaction();
    }
#endif
}

/*!
    @brief  Issue a single 8-bit value to the display. Chip-select,
            transaction and data/command selection must have been
            previously set -- this ONLY issues the byte. This is another of
            those functions in the library with a now-not-accurate name
            that's being maintained for compatibility with outside code.
            This function is used even if display connection is parallel.
    @param  b  8-bit value to write.
*/
void Adafruit_SPITFT::spiWrite(uint8_t b) {

       
		HAL_SPI_Transmit(&ILI9341_SPI_PORT, &b, sizeof(b), HAL_MAX_DELAY);
}

/*!
    @brief  Write a single command byte to the display. Chip-select and
            transaction must have been previously set -- this ONLY sets
            the device to COMMAND mode, issues the byte and then restores
            DATA mode. There is no corresponding explicit writeData()
            function -- just use spiWrite().
    @param  cmd  8-bit command to write.
*/
void Adafruit_SPITFT::writeCommand(uint8_t cmd) {
    SPI_DC_LOW();
    spiWrite(cmd);
    SPI_DC_HIGH();
}

/*!
    @brief   Read a single 8-bit value from the display. Chip-select and
             transaction must have been previously set -- this ONLY reads
             the byte. This is another of those functions in the library
             with a now-not-accurate name that's being maintained for
             compatibility with outside code. This function is used even if
             display connection is parallel.
    @return  Unsigned 8-bit value read (always zero if USE_FAST_PINIO is
             not supported by the MCU architecture).
*/
uint8_t Adafruit_SPITFT::spiRead(void) {
	uint8_t buff[2];
	HAL_SPI_Receive(&ILI9341_SPI_PORT, buff, 1, HAL_MAX_DELAY);
	return buff[0];
}






/*!
    @brief  Issue a single 16-bit value to the display. Chip-select,
            transaction and data/command selection must have been
            previously set -- this ONLY issues the word. Despite the name,
            this function is used even if display connection is parallel;
            name was maintaned for backward compatibility. Naming is also
            not consistent with the 8-bit version, spiWrite(). Sorry about
            that. Again, staying compatible with outside code.
    @param  w  16-bit value to write.
*/
void Adafruit_SPITFT::SPI_WRITE16(uint16_t w) {

	uint8_t data[] = { w >> 8, w & 0xFF };
	WriteData(data, sizeof(data));
        //hwspi._spi->transfer(w >> 8);
        //hwspi._spi->transfer(w);

}

/*!
    @brief  Issue a single 32-bit value to the display. Chip-select,
            transaction and data/command selection must have been
            previously set -- this ONLY issues the longword. Despite the
            name, this function is used even if display connection is
            parallel; name was maintaned for backward compatibility. Naming
            is also not consistent with the 8-bit version, spiWrite().
            Sorry about that. Again, staying compatible with outside code.
    @param  l  32-bit value to write.
*/
void Adafruit_SPITFT::SPI_WRITE32(uint32_t l) {
 
	    uint8_t data[] = { l >> 24, l >> 16, l >> 8, l };
	    WriteData(data, sizeof(data));
	    
       // hwspi._spi->transfer(l >> 24);
       // hwspi._spi->transfer(l >> 16);
       // hwspi._spi->transfer(l >> 8);
       // hwspi._spi->transfer(l);

}






