
#ifndef __MENU_H__
#define __MENU_H__

#ifndef HAS_INTERFACE
#include <encoder.h>

namespace UserInterface {

class noClassMenu {
public:
  void initialize() { }
  enum EncoderEvent poll() {}
  void flush() {}
  void activateTextScreen(const char* header, char *text) { }
  void activateValueScreen(const char* header, double *value = NULL, const char *unit = NULL, double *setpoint = NULL, double *percent = NULL) { }
  void activateConfigScreen() {}
};
}

#else

#include <encoder.h>
#include <display.h>

#include <menu.h>//menu macros and objects
#include <menuIO/u8g2Out.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/chainStream.h>

namespace UserInterface {
  using namespace Menu;

  result cleaning(eventMask e,navNode& nav, prompt &item);
  result exitMenu(eventMask e, navNode& nav, prompt &item);

  // ------------------- MENU ----------------------
  #define MAX_DEPTH 2

  const colorDef<uint8_t> colors[] MEMMODE={ //  {{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
    {{0,0},{0,1,0}},//bgColor
    {{0,0},{1,1,1}},//fgColor
    {{1,1},{1,0,0}},//valColor
    {{1,1},{1,0,0}},//unitColor
    {{0,0},{0,0,1}},//cursorColor
    {{1,1},{1,0,0}},//titleColor
    // {{0,0},{0,1,1}},//bgColor
    // {{1,1},{1,0,0}},//fgColor
    // {{1,1},{1,0,0}},//valColor
    // {{1,1},{1,0,0}},//unitColor
    // {{0,1},{0,0,1}},//cursorColor
    // {{1,1},{1,0,0}},//titleColor
  };

  // MENU(subMenu,"Sub-Menu",showEvent,anyEvent,noStyle
  //   ,OP("Sub1",showEvent,anyEvent)
  //   ,OP("Sub2",showEvent,anyEvent)
  //   ,OP("Sub3",showEvent,anyEvent)
  //   ,EXIT("<Back")
  // );

  MENU(subMenu_PID,"PID",doNothing,anyEvent,noStyle
    ,FIELD(configValues.PID.consKp,"Kp","",0,300,1,1,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.PID.consKi,"Ki","",0,10,0.1,0.01,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.PID.consKd,"Kd","",0,10,0.1,0.01,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.PID.Gap4BoostParameter,"boostGap","",0,50,1,1,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.PID.boostKp,"boostKp","",0,300,1,1,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.PID.boostKi,"boostKi","",0,10,0.1,0.01,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.PID.boostKd,"boostKd","",0,10,0.1,0.01,doNothing,noEvent,wrapStyle)
    ,EXIT("<Back")
  );

  MENU(subMenu_StateMaschine,"StateMaschine",doNothing,anyEvent,noStyle
    ,FIELD(configValues.stateMaschine.TempMinError,"TempThreshold","C",0,20,1,0,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.stateMaschine.TempStabilizingTime,"Stabilize","s",0,200,1,0,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.stateMaschine.Preinfusion_Duration,"Preinfusion","s",0,60,1,0,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.stateMaschine.Preinfusion_PumpDuration,"Preinf.Pump","s",0,10,1,0,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.stateMaschine.Brewing_Duration,"Brewing","s",0,60,1,0,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.stateMaschine.BrewingTempSetpointIncrease,"BrewingInc","C",0,100,1,0,doNothing,noEvent,wrapStyle)
    ,FIELD(configValues.stateMaschine.Flush_Duration,"Flush","s",0,20,1,0,doNothing,noEvent,wrapStyle)
    ,EXIT("<Back")
  );

  MENU(mainMenu,"Configuration",doNothing,noEvent,wrapStyle
    ,OP("Cleaning        ",cleaning,enterEvent)
    ,FIELD(globalValues.TempSetValue,"Temperature","C",0,100,1,0,doNothing,noEvent,wrapStyle)
    ,SUBMENU(subMenu_PID)
    ,SUBMENU(subMenu_StateMaschine)
    ,OP("Safe Config",saveConfiguration,enterEvent)
    ,OP("reset Config",removeConfiguration,enterEvent)
    ,OP("<Back", exitMenu,enterEvent)
  );

  ClickEncoderStream encStream(cEncoder.encoder,1);
  MENU_INPUTS(in,&encStream);
  MENU_OUTPUTS(out,MAX_DEPTH
    // ,U8X8_OUT(u8x8,{0,1,10,6})
    ,U8G2_OUT(cDisplay.u8g2,colors,cDisplay.fontX,cDisplay.fontY, 0, /* Headersize*/ 2 /*cDisplay.fontY+3*/ ,{0,1,cDisplay.width/cDisplay.fontX,cDisplay.height/cDisplay.fontY-1})
    ,NONE
  );
  NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);//the navigation root object


/****************************************** Interface Class ******************************************/

  #define DRAW(CMD) { cDisplay.firstPage(); do { CMD; } while(cDisplay.nextPage()); }

  #define GET_CENTER_POSITION_X(STR) ((cDisplay.width-cDisplay.u8g2.getStrWidth(STR))/2)
  #define GET_FONT_WIDTH(STR)        (cDisplay.u8g2.getStrWidth(STR))

  enum InterfaceScreen {
    CONFIG=0,
    TEXT,
    VALUE
  };

// #define convertEvents(label) (InterfaceEvent::#label);

  typedef struct {
    char header[20];
    double *value;
    char text[20];
    char valueUnit[5];
    double *setpoint;
    double *percent;
    bool setpointEditMode;
  } draw_ScreenValues;

  class ScreenTypes {

  private:

    void drawHeader(char *name) {
      cDisplay.setFontSize();
      cDisplay.u8g2.drawStr(0,cDisplay.fontY, name);
      cDisplay.u8g2.drawHLine(0,cDisplay.fontY+1,128);
    }

    void drawSetPoint(double *setpoint, bool *editMode) {
      if(!setpoint) return;
      cDisplay.setFontSize();
      char str[6];
      uint8_t i = (uint8_t)(*setpoint);
      if(editMode != NULL && *editMode) {
        sprintf(str, (i>9 ? ">%d" : "> %d"), i);
      } else {
        sprintf(str, (i>9 ? "%d" : " %d"), i);
      }
      cDisplay.u8g2.drawStr(cDisplay.width-GET_FONT_WIDTH(str) , cDisplay.fontY, str);
    }

    void drawProgressBar(double *percent, uint8_t x, uint8_t y, uint8_t h) {
      if(!percent) return;
      cDisplay.setFontSize();
      cDisplay.u8g2.drawFrame(x, y, cDisplay.width-(x*2), h); \
      cDisplay.u8g2.drawBox(x+2, y+2, (uint8_t)((cDisplay.width-(x*2)-4) * (*percent)), h-4);
    }

    void drawMainValue(double *value, char *unit, uint8_t y) {
      if(!value) return;
      cDisplay.setFontSize();
      char str[5];
      uint8_t u = 0;
      if(unit) u = GET_FONT_WIDTH(unit);
      cDisplay.setFontSize(3);
      uint8_t v = (uint8_t)(*value);
      sprintf(str, (v>9 ? "%d" : " %d"), v);
      v = GET_FONT_WIDTH(str);
      v += 2; //Abstand
      cDisplay.u8g2.drawStr( (cDisplay.width-(u+v))/2 , y, str);
      if(unit) {
        cDisplay.setFontSize();
        cDisplay.u8g2.drawStr( (cDisplay.width-(u+v))/2+v/*Abstand*/ , y, unit);
      }
    }

    void drawMainText(char *text, uint8_t y) {
      cDisplay.setFontSize(2);
      uint8_t v = GET_FONT_WIDTH(text);
      cDisplay.u8g2.drawStr( (cDisplay.width-(v))/2 , y, text);
    }

  public:
    draw_ScreenValues drawValues;

    ScreenTypes() { clear(); }

    void clear() {
      drawValues.header[0]        = 0x0;
      drawValues.value            = NULL;
      drawValues.valueUnit[0]     = 0x0;
      drawValues.setpoint         = NULL;
      drawValues.setpointEditMode = false;
      drawValues.percent          = NULL;
    }

    void drawTextScreen() {
      if(!drawValues.header[0]) return;
      drawHeader(drawValues.header);

      drawMainText(drawValues.text, cDisplay.height - 17);
    }

    void drawValueScreen() {
      char str[5];
      uint8_t i;
      if(!drawValues.header[0]) return;
      drawHeader(drawValues.header);

      drawSetPoint(drawValues.setpoint, &drawValues.setpointEditMode);

      drawProgressBar(drawValues.percent,0,cDisplay.height-10,10);

      drawMainValue(drawValues.value, drawValues.valueUnit, cDisplay.height - 17);

    }

    void drawConfigScreen() {
      drawHeader("Configuration");
      nav.doOutput();
    }
  };

  class ClassMenu : public ScreenTypes {

  private:
    InterfaceScreen screen;
    bool forceUpdateScreen;

  public:
	  ClassMenu()  {}
    static bool exitConfigMenu;

  	void initialize() {
      cDisplay.initialize();
      cEncoder.initialize();

      // nav.idleTask=idle;//point a function to be used when menu is suspended
      // mainMenu[1].enabled=disabledStatus;
      nav.showTitle=false;
      // nav.idleTask=idle;//point a function to be used when menu is suspended
      screen = CONFIG;
      forceUpdateScreen = true;
      exitConfigMenu = false;
    }

    void flush() {
      switch(screen) {
        case CONFIG:
          if (nav.changed(0) || forceUpdateScreen) {//only draw if menu changed for gfx device
            //change checking leaves more time for other tasks
            DRAW( drawConfigScreen(); )
          }
        break;

        case TEXT:
          DRAW( drawTextScreen(); )
        break;

        case VALUE:
          DRAW( drawValueScreen(); )
        break;
      }
    }

    enum EncoderEvent poll() {

      EncoderEvent retVal = NOTHING;

      if(exitConfigMenu) {
        exitConfigMenu = false;
        return EXIT;
      }

      switch(screen) {
        case CONFIG:
          nav.doInput();
          break;

        case TEXT:
          retVal = cEncoder.poll();
          if(retVal == BUTTON_HELD) {
            activateConfigScreen();
            return retVal;
          }
          break;

        case VALUE:
          if(retVal = cEncoder.poll()) {
            if(retVal == BUTTON_HELD) {
              activateConfigScreen();
              return retVal;
            }
            if(drawValues.setpoint) {
              switch(retVal) {
                case BUTTON_PRESSED:
                  drawValues.setpointEditMode = !drawValues.setpointEditMode;
                  break;
                case ROTATE_LEFT:
                  if(drawValues.setpointEditMode)
                    (*drawValues.setpoint)++;
                  break;
                case ROTATE_RIGHT:
                  if(drawValues.setpointEditMode)
                    (*drawValues.setpoint)--;
                  break;
              }
            }
            return retVal;
          }
          break;
      }

      flush();

      if(retVal != NOTHING) forceUpdateScreen = false;
      return retVal;
    }

    void changeScreen(InterfaceScreen s) {
      screen = s;
      forceUpdateScreen = true;
    }

    void activateTextScreen(const char* header, char *text) {
      clear();
      strcpy(drawValues.header,header);
      strcpy(drawValues.text,text);
      changeScreen(TEXT);
    }

    void activateValueScreen(const char* header, double *value = NULL, const char *unit = NULL, double *setpoint = NULL, double *percent = NULL) {
      clear();
      strcpy(drawValues.header,header);
      drawValues.value = value;
      if(unit) {
        strcpy(drawValues.valueUnit,unit);
      }
      drawValues.setpoint = setpoint;
      drawValues.percent = percent;
      changeScreen(VALUE);
    }

    void activateConfigScreen() {
      changeScreen(CONFIG);
    }
  };


  result showEvent(eventMask e,navNode& nav,prompt& item) {
    Serial.print("event: ");
    Serial.println(e);
    return proceed;
  }

  result cleaning(eventMask e, navNode& nav, prompt &item) {
    Serial.print("action1 event: ");
    Serial.print(e);
    Serial.println(", proceed menu");
    Serial.flush();

    // strcpy(Interface.drawValues.header,"Brewing");
    // Interface.drawValues.value = &Interface.v;
    // Interface.drawValues.setpoint = &Interface.s;
    // Interface.changeScreen(VALUE);
    return proceed;
  }

  result exitMenu(eventMask e, navNode& nav, prompt &item) {
    ClassMenu::exitConfigMenu = true;
    return proceed;
  }

  bool ClassMenu::exitConfigMenu = false; //static variable
}
#endif
#endif
