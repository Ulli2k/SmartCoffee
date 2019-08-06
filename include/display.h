
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <Wire.h>
#include <U8g2lib.h>

#define DISPLAY_NO_ROTATION       U8G2_R0
#define DISPLAY_90_ROTATION       U8G2_R1
#define DISPLAY_180_ROTATION      U8G2_R2
#define DISPLAY_270_ROTATION      U8G2_R3
/******************************************** Display ********************************************/
// Wiki: https://github.com/olikraus/u8g2/wiki/u8g2reference#setfont
// Bilder einfügen https://sandhansblog.wordpress.com/2017/04/16/interfacing-displaying-a-custom-graphic-on-an-0-96-i2c-oled/

  class ClassDisplay {

  public:
    U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2;
    uint8_t fontX;
    uint8_t fontY;
    const uint8_t width;
    const uint8_t height;
    const uint8_t *fontName;

    // const struct fontname;  //https://github.com/olikraus/u8g2/wiki/fntlistall
  	ClassDisplay() : u8g2(DISPLAY_180_ROTATION), width(128), height(64) {
      setFontSize();
    }

    void initialize() {
      Wire.begin();
      // u8x8.begin();
      u8g2.begin();
      setFontSize();
    }

    void firstPage() { u8g2.firstPage(); }
    bool nextPage() { return u8g2.nextPage(); }

    void setFontSize(uint8_t size=0) {
      switch(size) {
        case 0:
          fontX = 6;
          fontY = 13;
          fontName = u8g2_font_7x13_mf;
          break;
        case 1:
          break;
        case 3:
          // fontX = 17;
          // fontY = 34;
          // fontName = u8g2_font_logisoso34_tf;
          fontX = 31; //12;
          fontY = 38; //24;
          fontName = u8g2_font_helvR24_tf;
          break;
      }
      u8g2.setFont(fontName);
      u8g2.home();
      u8g2.setFontPosBaseline();
    }

    bool poll() {
      return true;
    }


  };

#endif
