/*
 * SmartCoffee Controller with ESP8266 NodeMCU
 */

// Rotary Encode LIbrary nodemcu.readthedocs.io/en/master/modules/rotary
// namespace SmartCoffee {

// #include <Arduino.h>
// #include <FunctionalInterrupt.h> //std::bind

#include <EEPROM.h>
#include <globals.h>

#define HAS_INTERFACE

/************************/
/******** Digital Output Pins ********/
#include <digitalPin.h>

#ifdef ESP32
//ESP32
typedef simplePin<2>                    LEDType;        // LED
typedef simplePin<4,true,false,2730,0>  BuzzerType;     // Buzzer
typedef simplePin<26>                   PowerType;      // OnOff
typedef simplePin<0,false,true>         PowerSensType;  // OnOff Detection
typedef simplePin<27>                   PumpType;       // Pump
typedef simplePin<15,false,true>        PumpSensType;   // Bewing Detection
typedef simplePin<12>                   ValveType;      // Valve
typedef simplePin<13>                   HeaterType;     // Heater

#define TEMPERATUR_SENSOR_PIN      32
#else
//ESP8266
typedef simplePin<2>              LEDType;        // LED
typedef simplePin<15>             PowerType;      // OnOff
typedef simplePin<16,false,true>  PowerSensType;  // OnOff Detection
typedef simplePin<13>             PumpType;       // Pump
typedef simplePin<10,false,true>  PumpSensType;   // Bewing Detection
typedef simplePin<12>             ValveType;      // Valve
typedef simplePin<14>             HeaterType;     // Heater

#define TEMPERATUR_SENSOR_PIN      0
#endif

// #ifdef HAS_INTERFACE
#include <encoder.h>
typedef ClassEncoder<19, 18, 5>   encoderType;

#include <display.h>
typedef ClassDisplay              displayType;
// #endif

 /************************/


/***************************************************/
/******** Temperature Sensor Implementation ********/
#include <tempSens.h>

// typedef ClassDallas<TEMPERATUR_SENSOR_PIN> SensType;
typedef ClassTSIC<TEMPERATUR_SENSOR_PIN> SensType;
typedef ClassTempSens<SensType> TempSensType;
TempSensType Temp;

#define DEFAULT_TEMP_SETPOINT   0   //set to 0. PID 0% in case off temperature sensor error=0
#define MAX_TEMP_SETPOINT       100
/***************************************************/


/*******************************************/
/******** OutPut PWM Implementation ********/
#include <pwm.h>
#define PWM_PERIOD            1000  // [ms] Periodendauer

ClassPWM<HeaterType, PWM_PERIOD> HeaterPWM;
/*******************************************/

/************************************/
/******** PID Implementation ********/
#include <pid.h>

double  Pid_Output    = 0;
// double  *Pid_Input    = &globalValues.TempValue;      // DEFAULT_TEMP_SETPOINT;
// double  *Pid_SetPoint = &globalValues.TempSetValue;   // DEFAULT_TEMP_SETPOINT;

ClassPID Pid(/*Pid_SetPoint*/&globalValues.TempSetValue, /*Pid_Input*/&globalValues.TempValue, &Pid_Output);

#define PID_MAX_SETPOINT      MAX_TEMP_SETPOINT // Setpoint Limitation
#define PID_OUTPUT_LIMIT      PWM_PERIOD        // PID Output 0-1000
#define PID_CALC_PERIOD       PWM_PERIOD        // [ms] calculation Period of PID values
/************************************/

/*********************************************************/
/******** User Interface (Display, RotaryEncoder) ********/
#ifdef HAS_INTERFACE
namespace UserInterface {
  encoderType cEncoder;
  displayType cDisplay;
};
#include <interface.h>
typedef UserInterface::ClassMenu    MenuType;       // Display, RotaryEncoder
#else
namespace UserInterface {
  noClassMenu cEncoder;
  noClassMenu cDisplay;
}
#include <interface.h>
typedef UserInterface::noClassMenu    MenuType;       // Display, RotaryEncoder
#endif
/*********************************************************/


/*******************************/
/******** State Machine ********/
/*******************************/
#include <statemachine.h>
ClassStateMachine<MenuType,LEDType,BuzzerType,PowerType,PowerSensType,PumpType,PumpSensType,ValveType> StateMachine;

//==============================================================
//                  Web Interface Functions
//==============================================================

#define HAS_WEB
#ifdef HAS_WEB
#include <web.h>
typedef ClassWebServer WebConnection;
// typedef ClassMQTT WebConnection;
ClassWeb</*noWeb*/WebConnection> Web;

// String getValueForInterface(String name) {
//
//     if(name == "resetGraph") { //startBrewing
//       return String((StateMachine.newBrewingDetected()?"true":"false"));
//     } else if(name == "OnOffState") {
//       return (StateMachine.isPowerOn() ? "ON" : "OFF");
//     } else if(name == "SetPoint") {
//       return String(configValues.TempSetValue);
//     } else if(name == "TempSens") {
//       return String(Temp.getValue());
//     } else if(name == "TempAvgSens") {
//       return String(globalValues.TempValue/*Temp.getAvgValue()*/);
//     } else if(name == "PID") {
//       return String(globalValues.Pid_Percent);
//     } else if(name == "StateMachine") {
//       return (StateMachine.getState());
//     } else if(name == "BrewingTimer") {
//       return String(StateMachine.getBrewingTime());
//     }
// }
//
void setValueByInterface(String name, String value) {
  Serial.print("remote set ");Serial.print(name);Serial.print(" to ");Serial.println(value);
  if(name == "Power") {
    StateMachine.PowerSwitch(value == "on" ? true : false);
  }
}

void pushValueToInterface (String name, String value) {
  if(name == "Power") {
    Web.postPowerState(value == "on" ? true : false);
  }
}

#endif

//==============================================================
//                  Global Values
//==============================================================
#include <globals.h>

//==============================================================
//                  SETUP
//==============================================================
void setup(void){

  // Serial Debug Output
  Serial.begin(115200);
  // while (!Serial); //if just the the basic function, must connect to a computer
  Serial.println(); Serial.println("Booting Sketch...");

  getConfiguration();

  // LED.initialize();
  StateMachine.initialize();
  HeaterPWM.initialize();

  if(Temp.initialize(&globalValues.TempValue)) { Serial.println("Temperature sensor initialized"); } else { Serial.println("Temperature sensor failed"); }
  Temp.setAutoSampling(true);

#ifdef HAS_WEB
  // Web Interface
  Web.initialize();
#endif

  // PID
  Pid.initialize(PID_CALC_PERIOD, PID_OUTPUT_LIMIT);
  // Web.send("test","hallo");

}

//==============================================================
//                     LOOP
//==============================================================
void loop(void){

  StateMachine.poll();

  // Pid_Input = Temp.getAvgValue(); //interrupt driven
  Pid.poll(); //poll for Tuning K values to aggressive ones
  globalValues.Pid_Percent = (Pid_Output/PID_OUTPUT_LIMIT);
  HeaterPWM.poll(Pid_Output);

#ifdef HAS_WEB
  Web.poll();
#endif
}
