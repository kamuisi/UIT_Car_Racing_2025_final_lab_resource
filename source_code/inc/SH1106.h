#ifndef SH1106_H_
#define SH1106_H_

#include <iostream>
#include "SH1106Fonts.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <string.h>
#include <linux/i2c-dev.h>

extern "C" 
{
   #include <i2c/smbus.h>
}

#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define WIDTH_POS 0
#define HEIGHT_POS 1
#define FIRST_CHAR_POS 2
#define CHAR_NUM_POS 3
#define CHAR_WIDTH_START_POS 4

#define TEXT_ALIGN_LEFT 0
#define TEXT_ALIGN_CENTER 1
#define TEXT_ALIGN_RIGHT 2

#define CHARGEPUMP 0x8D
#define COLUMNADDR 0x21
#define COMSCANDEC 0xC8
#define COMSCANINC 0xC0
#define DISPLAYALLON 0xA5
#define DISPLAYALLON_RESUME 0xA4
#define DISPLAYOFF 0xAE
#define DISPLAYON 0xAF
#define EXTERNALVCC 0x1
#define INVERTDISPLAY 0xA7
#define MEMORYMODE 0x20
#define NORMALDISPLAY 0xA6
#define PAGEADDR 0x22
#define PAGESTARTADDRESS 0xB0
#define SEGREMAP 0xA1
#define SETCOMPINS 0xDA
#define SETCONTRAST 0x81
#define SETDISPLAYCLOCKDIV 0xD5
#define SETDISPLAYOFFSET 0xD3
#define SETHIGHCOLUMN 0x10
#define SETLOWCOLUMN 0x00
#define SETMULTIPLEX 0xA8
#define SETPRECHARGE 0xD9
#define SETSEGMENTREMAP 0xA1
#define SETSTARTLINE 0x40
#define SETVCOMDETECT 0xDB
#define SWITCHCAPVCC 0x2

class SH1106 {

private:
   uint16_t myI2cAddress;
   int _device;

   uint8_t buffer[128 * 64 / 8];
   int myTextAlignment = TEXT_ALIGN_LEFT;
   int myColor = WHITE;
   uint8_t lastChar;
   const char *myFontData = ArialMT_Plain_10;

public:
   // For I2C display: create the display object connected to pin SDA and SDC
   SH1106(uint16_t i2cAddress = 0x3C) : myI2cAddress(i2cAddress), _device(-1){}

   ~SH1106();
   
   // Initialize the display
   bool Init();

   // Cycle through the initialization
   void resetDisplay(void);

   // Turn the display on
   void displayOn(void);

   // Turn the display offs
   void displayOff(void);

   // Clear the local pixel buffer
   void clear(void);

   // Write the buffer to the display memory
   void display(void);

   // Set display contrast
   void setContrast(char contrast);

   // Turn the display upside down
   void flipScreenVertically();

   // Send a command to the display (low level function)
   void sendCommand(uint8_t cmd);

   // Send all the init commands
   void sendInitCommands(void);

   // Draw a pixel at given position
   void setPixel(int x, int y);

   // Draw 8 bits at the given position
   void setChar(int x, int y, unsigned char data);

   // Draw the border of a rectangle at the given location
   void drawRect(int x, int y, int width, int height);

   // Fill the rectangle
   void fillRect(int x, int y, int width, int height);

   // Draw a bitmap with the given dimensions
   void drawBitmap(int x, int y, int width, int height, const char *bitmap);

   // Draw an XBM image with the given dimensions
   void drawXbm(int x, int y, int width, int height, const char *xbm);

   // Sets the color of all pixel operations
   void setColor(int color);

   // converts utf8 characters to extended ascii
   // taken from http://playground.arduino.cc/Main/Utf8ascii
   uint8_t utf8ascii(uint8_t ascii);

   // converts utf8 string to extended ascii
   // taken from http://playground.arduino.cc/Main/Utf8ascii
   std::string utf8ascii(std::string s);

   // Draws a string at the given location
   void drawString(int x, int y, std::string text);

   // Draws a String with a maximum width at the given location.
   // If the given String is wider than the specified width
   // The text will be wrapped to the next line at a space or dash
   void drawStringMaxWidth(int x, int y, int maxLineWidth, std::string text);

   // Returns the width of the String with the current
   // font settings
   int getStringWidth(std::string text);

   // Specifies relative to which anchor point
   // the text is rendered. Available constants:
   // TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT
   void setTextAlignment(int textAlignment);

   // Sets the current font. Available default fonts
   // defined in SH1106Fonts.h:
   // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
   void setFont(const char *fontData);

};

#endif