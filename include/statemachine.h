#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__

#include <globals.h>

#define DEFAULT_POWERON_MAX_TIME                  60 // [min]

#define DEFAULT_STANDBY_TEMPERATURE                0 // [°C]
#define DEFAULT_BREWING_TEMPERATURE               94 // [°C] 92

#define TEMP_STABELIZING_TIME                     60 // [seconds]
#define TEMP_MIN_ERROR                             1 // [°C]

#define SUPPLY_TEMP_SETPOINT_INCREASE             31 // [°C], ggf. gleich PID_GAP_4_BOOST_PARAMETER ??
#define SUPPLY_DURATION                           23 // [s]
#define PREINFUSION_DURATION                       2 // [s]
#define PREINFUSION_PUMP_DURATION                2.3 // [s]
#define FLUSH_DURATION                           1.4 // [s]

#define CLEANING_FLUSH_DURATION                  4.0 // [s]
#define CLEANING_WAIT_DURATION                    30 // [s]
#define CLEANING_NUMFLUSES                         5 // [-]


enum State {
  STANDBY = 0,
  STARTUP,
  HEATING, READY,
  SUPPLY,
  PREINFUSION, PREINFUSION_WAIT,
  BREWING,
  DONE,
  FLUSH,
  CLEANING_PREPARE, CLEANING, CLEANING_WAIT, CLEANING_FLUSH, CLEANING_DONE
};

class ClassTiming {

public:
  bool active;
  long unsigned int ms;
  long unsigned int time_ms;

  ClassTiming() : active(0), ms(0), time_ms(0) {}

  double leftSeconds;
  double setSeconds;
  double percent;
  void startTimer(double sec) {
    setSeconds = sec;
    ms = millis();
    active=true;
  }
  bool checking() {
    long delta = (millis()-ms)/1000;

    leftSeconds = (double)(setSeconds-delta);
    percent = (double)(leftSeconds/(double)setSeconds);
    active = (leftSeconds > 0);
    return !active;
  }
  bool isActive() {
    return active;
  }
};

template<typename InterfaceType, typename LedType, typename BuzzerType, typename PowerType,typename PowerSensType,typename PumpType,typename PumpSensType,typename ValveType>
class ClassStateMachine {

private:

  uint16_t  PowerOnTime;    //[s] safe Time on Powering On the Machine (hour*60+min)
  bool      PowerOnOff;     //[on, off] Power State

  ClassTiming Timer;

  InterfaceType Interface;
  LedType       PowerLed;
  BuzzerType    Buzzer;
  PowerType     Power;
  PowerSensType PowerSens;
  PumpType      Pump;
  PumpSensType  PumpSens;
  ValveType     Valve;

  bool TempWithinThreshold() {
    if(abs(globalValues.TempSetValue - globalValues.TempValue) <= configValues.stateMaschine.TempMinError) {
      return true;
    }
    return false;
  }

public:
  uint8_t   _state;         //current State of StateMachine
  int8_t    memory;         //general global Variable
  bool      StartupFlush;   //[false,true] flush before first Supply

  ClassStateMachine() : _state(STANDBY), memory(0), StartupFlush(true) {
    configValues.stateMaschine.MaxPowerOnTime               = DEFAULT_POWERON_MAX_TIME;
    globalValues.standby                                    = true;
    globalValues.TempSetValue                               = DEFAULT_STANDBY_TEMPERATURE;
    configValues.TempSetValue                               = DEFAULT_BREWING_TEMPERATURE;
    configValues.stateMaschine.TempMinError                 = TEMP_MIN_ERROR;
    configValues.stateMaschine.TempStabilizingTime          = TEMP_STABELIZING_TIME;
    configValues.stateMaschine.Preinfusion_Duration         = PREINFUSION_DURATION;
    configValues.stateMaschine.Preinfusion_PumpDuration     = PREINFUSION_PUMP_DURATION;
    configValues.stateMaschine.Brewing_Duration             = SUPPLY_DURATION;
    configValues.stateMaschine.BrewingTempSetpointIncrease  = SUPPLY_TEMP_SETPOINT_INCREASE;
    configValues.stateMaschine.Flush_Duration               = FLUSH_DURATION;
    configValues.stateMaschine.Cleaning_FlushDuration       = CLEANING_FLUSH_DURATION;
    configValues.stateMaschine.Cleaning_WaitDuration        = CLEANING_WAIT_DURATION;
    configValues.stateMaschine.Cleaning_NumFlushes          = CLEANING_NUMFLUSES;
  }

  void initialize() {

    // Display & RotaryEncoder
    Interface.initialize();

    // Power LED
    PowerLed.initialize();
    // Buzzer
    Buzzer.initialize();

    // OnOff Function
    PowerOnOff = false;
    Power.initialize();
    PowerSens.initialize();
    // Brewing/Pump
    Pump.initialize();
    PumpSens.initialize();
    // Valve
    Valve.initialize();

    changeState(STANDBY);
    checkPowerOnTime(false);
  }

  void poll() {

    uint8_t newState = _state;

    if(_state != STANDBY && checkPowerOnTime()) { changeState(STANDBY); return; }

    enum MenuFunctions event = Interface.poll();
    if(event == EXIT) { changeState(_state); return; }

    if(PowerSens.updateState(&PowerOnOff)) { changeState(PowerOnOff ? STARTUP : STANDBY); return; }

    if(event == FKT_CLEANING) { changeState(PowerOnOff ? CLEANING_PREPARE : STANDBY); return; }

    switch(_state) {

      case STANDBY:
        break;

      case STARTUP:
        newState=HEATING;
        break;

      case HEATING:
        if(TempWithinThreshold()) {
          if(!Timer.isActive()) {
            Timer.startTimer(configValues.stateMaschine.TempStabilizingTime);
          } else {
            if(Timer.checking())      { newState = (StartupFlush? FLUSH : READY); }
          }
        }
        if(PumpSens.isActive())     { newState=SUPPLY; }
        if(event == BUTTON_PRESSED) { newState=FLUSH; }
        break;

      case READY:
        if(!TempWithinThreshold())  { newState=HEATING; }
        if(PumpSens.isActive())     { newState=SUPPLY; }
        if(event == BUTTON_PRESSED) { newState=FLUSH; }
        break;

      case SUPPLY:
        newState=PREINFUSION;
        if(!PumpSens.isActive()) { newState=HEATING; }
        break;

      case PREINFUSION:
        if(Timer.checking())      { newState=PREINFUSION_WAIT; }
        if(!PumpSens.isActive())  { newState=HEATING; }
        break;

      case PREINFUSION_WAIT:
        if(Timer.checking())      { newState=BREWING; }
        if(!PumpSens.isActive())  { newState=HEATING; }
        break;

      case BREWING:
        if(Timer.checking())      { newState=DONE; }
        if(!PumpSens.isActive())  { newState=HEATING; }
        break;

      case DONE:
        if(!PumpSens.isActive())    { newState=FLUSH; }
        if(event == BUTTON_PRESSED) { newState=HEATING; }
        break;

      case FLUSH:
        if(Timer.checking())        { newState=HEATING; }
        break;

      case CLEANING_PREPARE:
        if(PumpSens.isActive())     { newState=CLEANING; }
        if(event == BUTTON_PRESSED) { newState=HEATING; }
        break;

      case CLEANING:
        if(!PumpSens.isActive())    { newState=HEATING; }
        if(Timer.checking())        { newState= (memory!=0 ? CLEANING_WAIT : CLEANING_DONE); }
        break;

      case CLEANING_WAIT:
        if(!PumpSens.isActive())    { newState=HEATING; }
        if(Timer.checking())        { newState=CLEANING; }
        break;

      case CLEANING_FLUSH:
        if(!PumpSens.isActive())    { newState=HEATING; }
        if(Timer.checking())        { newState=CLEANING_DONE; }
        break;

      case CLEANING_DONE:
        if(!PumpSens.isActive())    { newState=FLUSH; }
        if(event == BUTTON_PRESSED) { newState=CLEANING_FLUSH; }
        break;
    }

    if(newState!=_state) {
      changeState(newState);
    }
  }

  void changeState(uint8_t newState) {

    Serial.print("State changed to ");

    switch(newState) {

      case STANDBY:
        Serial.println("STANDBY");
        memory = 0;
        globalValues.TempSetValue = DEFAULT_STANDBY_TEMPERATURE;
        Serial.print("TempSetValue: ");Serial.println(globalValues.TempSetValue);
        Pump.setState(0);
        Valve.setState(0);
        Power.setState(0);
        PowerLed.setState(0);
        Interface.activateStandbyScreen("Standby");
        pushValueToInterface("Power", "off");
        globalValues.standby=true;
        break;

      case STARTUP:
        globalValues.standby=false;
        PowerLed.setState(1);
        Power.setState(1);
        globalValues.TempSetValue = configValues.TempSetValue;
        Serial.print("TempSetValue: ");Serial.println(globalValues.TempSetValue);
        Buzzer.beep(2);
        checkPowerOnTime(false);
        StartupFlush = true;
        pushValueToInterface("Power", "on");
        break;

      case HEATING:
        Serial.println("HEATING");
        Pump.setState(0);
        Valve.setState(0);
        if(memory && _state < CLEANING_PREPARE) { globalValues.TempSetValue -= memory;}
        memory=0;
        Interface.activateValueScreen("Heating", &globalValues.TempValue, "degC",&globalValues.TempSetValue, &globalValues.Pid_Percent);
        break;

      case READY:
        Serial.println("READY");
        Buzzer.beep(2);
        Interface.activateValueScreen("Ready", &globalValues.TempValue, "degC", &globalValues.TempSetValue, &globalValues.Pid_Percent);
        break;

      case SUPPLY:
        Serial.println("SUPPLY");
        memory = configValues.stateMaschine.BrewingTempSetpointIncrease;
        globalValues.TempSetValue += configValues.stateMaschine.BrewingTempSetpointIncrease;
        checkPowerOnTime(false);
        break;

      case PREINFUSION:
        Serial.println("PREINFUSION");
        Interface.activateValueScreen("Preinfusion...");
        // Interface.flush();
        Valve.setState(1);
        Pump.setState(1);
        Timer.startTimer(configValues.stateMaschine.Preinfusion_PumpDuration);
        // delay(configValues.stateMaschine.Preinfusion_PumpDuration*1000);
        break;

      case PREINFUSION_WAIT:
        Serial.println("PREINFUSION_WAIT");
        Pump.setState(0);
        Interface.activateValueScreen("Preinfusion...", &Timer.leftSeconds, "sec", &Timer.setSeconds, &Timer.percent);
        Timer.startTimer(configValues.stateMaschine.Preinfusion_Duration);
        break;

      case BREWING:
        Serial.println("BREWING");
        Valve.setState(1);
        Pump.setState(1);
        Timer.startTimer(configValues.stateMaschine.Brewing_Duration);
        Interface.activateValueScreen("Brewing...", &Timer.leftSeconds, "sec", &Timer.setSeconds, &Timer.percent);
        break;

      case DONE:
        Serial.println("DONE");
        Pump.setState(0);
        Valve.setState(0);
        globalValues.TempSetValue -= memory;
        memory=0;
        Interface.activateTextScreen("Done!", "Flush ???");
        break;

      case FLUSH:
        Serial.println("FLUSH");
        Interface.activateTextScreen("Flushing...", "!! HOT !!");
        if(StartupFlush) StartupFlush = false;
        Timer.startTimer(configValues.stateMaschine.Flush_Duration);
        Valve.setState(1);
        Pump.setState(1);
        break;

      case CLEANING_PREPARE:
        Serial.println("CLEANING_PREPARE");
        Valve.setState(0);
        Pump.setState(0);
        Interface.activateTextScreen("Cleaning...", "preparation...");
        memory = configValues.stateMaschine.Cleaning_NumFlushes;
        checkPowerOnTime(false);
        break;

      case CLEANING:
        Serial.println("CLEANING_PREPARE");
        if(memory == configValues.stateMaschine.Cleaning_NumFlushes) { //fillup chamber
          Timer.startTimer(configValues.stateMaschine.Cleaning_FlushDuration*2);
        } else {
          Timer.startTimer(configValues.stateMaschine.Cleaning_FlushDuration);
        }
        Valve.setState(1);
        Pump.setState(1);
        memory--;
        Interface.activateTextScreen("Cleaning...", "!! HOT !!");
        break;

      case CLEANING_WAIT:
        Serial.println("CLEANING_WAIT");
        Timer.startTimer(configValues.stateMaschine.Cleaning_WaitDuration);
        Valve.setState(0);
        Pump.setState(0);
        Interface.activateValueScreen("Cleaning...", &Timer.leftSeconds, "sec", &Timer.setSeconds, &Timer.percent);
        break;

      case CLEANING_FLUSH:
        Serial.println("CLEANING_FLUSH");
        Interface.activateTextScreen("Cleaning...", "!! HOT !!");
        Timer.startTimer(configValues.stateMaschine.Flush_Duration*2);
        Valve.setState(1);
        Pump.setState(1);
        break;

      case CLEANING_DONE:
        Serial.println("CLEANING_DONE");
        Valve.setState(0);
        Pump.setState(0);
        Interface.activateTextScreen("Cleaning...", "clean basket...Flush!");
        break;
    }

    _state = newState;
  }

  bool checkPowerOnTime(bool check=true) {

    uint16_t now = globalValues.TimeDate.hour*60 + globalValues.TimeDate.min;
    if(!check) {
      PowerOnTime = now;
    } else {
      if((now - PowerOnTime) >= configValues.stateMaschine.MaxPowerOnTime) {
        return true;
      }
    }
    return false;
  }

  void PowerSwitch(bool on) {
    // Serial.print("Remote Power ");Serial.println(on ? "on" : "off");
    changeState(on ? STARTUP : STANDBY);
  }

  // void startBrewing() {
  //   changeState(SUPPLY);
  // }

  // uint8_t getBrewingTime() {
  //   if(!msSupplyBegin) return SUPPLY_DURATION;
  //   return (uint8_t) (SUPPLY_DURATION - ((millis()-msSupplyBegin)/1000));
  // }

  // bool isPowerOn() {
  //   return PowerSens.getState();
  // }

  // String getState() {
  //   switch(_state) {
  //     case STANDBY:
  //       return "STANDBY";
  //     case HEATING:
  //       return "HEATING";
  //     case READY:
  //       return "READY";
  //     case SUPPLY:
  //       return "SUPPLY";
  //     case PREINFUSION:
  //       return "PREINFUSION";
  //     case BREWING:
  //       return "BREWING";
  //     case DONE:
  //       return "DONE";
  //   }
  //   return String("unknown");
  // }
};


#endif
