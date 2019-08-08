#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__

#include <globals.h>

#define DEFAULT_STANDBY_TEMPERATURE                0  //[°C]
#define DEFAULT_BREWING_TEMPERATURE               92  //[°C] 92

#define TEMP_STABELIZING_TIME                     10 // [seconds]
#define TEMP_MIN_ERROR                             1 // [°C]

#define SUPPLY_TEMP_SETPOINT_INCREASE             31 // [°C], ggf. gleich PID_GAP_4_BOOST_PARAMETER ??
#define SUPPLY_DURATION                           26 // [s]
#define PREINFUSION_DURATION                     3.4 // [s]
#define PREINFUSION_PUMP_DURATION                1.6 // [s]
#define FLUSH_DURATION                             3 // [s]

enum State {
  STANDBY = 0,
  STARTUP,
  HEATING,
  READY,
  SUPPLY,
  PREINFUSION,
  PREINFUSION_WAIT,
  BREWING,
  DONE,
  FLUSH
};

class ClassTiming {
  unsigned long ms;
  unsigned long time_ms;
  bool active;
public:
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

template<typename InterfaceType, typename LedType, typename PowerType,typename PowerSensType,typename PumpType,typename PumpSensType,typename ValveType>
class ClassStateMachine {

private:

  uint8_t _state; //current State of StateMachine
  bool PowerOnOff;
  ClassTiming Timer;
  InterfaceType Interface;
  LedType PowerLed;
  PowerType Power;
  PowerSensType PowerSens;
  PumpType Pump;
  PumpSensType PumpSens;
  ValveType Valve;

public:

  ClassStateMachine() : _state(STANDBY) {
    globalValues.TempSetValue                               = DEFAULT_STANDBY_TEMPERATURE;
    configValues.TempSetValue                               = DEFAULT_BREWING_TEMPERATURE;
    configValues.stateMaschine.TempMinError                 = TEMP_MIN_ERROR;
    configValues.stateMaschine.TempStabilizingTime          = TEMP_STABELIZING_TIME;
    configValues.stateMaschine.Preinfusion_Duration         = PREINFUSION_DURATION;
    configValues.stateMaschine.Preinfusion_PumpDuration     = PREINFUSION_PUMP_DURATION;
    configValues.stateMaschine.Brewing_Duration             = SUPPLY_DURATION;
    configValues.stateMaschine.Flush_Duration               = FLUSH_DURATION;
    configValues.stateMaschine.BrewingTempSetpointIncrease  = SUPPLY_TEMP_SETPOINT_INCREASE;
  }

  void initialize() {

    // Display & RotaryEncoder
    Interface.initialize();

    // Power LED
    PowerLed.initialize();

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
  }

  void poll() {

    uint8_t newState=_state;

    enum EncoderEvent event = Interface.poll();
    if(event == EXIT) {
      changeState(_state);
      return;
    }

    if(PowerSens.updateState(&PowerOnOff)) {
      Serial.print("POWER: ");Serial.println(PowerOnOff);
      changeState(PowerOnOff ? STARTUP : STANDBY);
      return;
    }

    switch(_state) {

      case STANDBY:
        break;

      case STARTUP:
        newState=HEATING;
        break;

      case HEATING:
        if(abs(globalValues.TempSetValue - globalValues.TempValue) <= configValues.stateMaschine.TempMinError) {
          if(!Timer.isActive()) {
            Timer.startTimer(configValues.stateMaschine.TempStabilizingTime);
          } else {
            if(Timer.checking())      { newState=READY; }
          }
        }
        if(PumpSens.isActive())     { newState=SUPPLY; }
        if(event == BUTTON_PRESSED) { newState=FLUSH; }
        break;

      case READY:
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
        if(!PumpSens.isActive())    { newState=HEATING; }
        if(event == BUTTON_PRESSED) { newState=FLUSH; }
        break;

      case FLUSH:
        if(Timer.checking())      { newState=HEATING; }
        //if(!PumpSens.isActive())  { newState=HEATING; }
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
        globalValues.TempSetValue = DEFAULT_STANDBY_TEMPERATURE;
        Serial.print("TempSetValue: ");Serial.println(globalValues.TempSetValue);
        Power.setState(0);
        PowerLed.setState(0);
        Pump.setState(0);
        Valve.setState(0);
        Interface.activateValueScreen("STANDBY");
        break;

      case STARTUP:
        PowerLed.setState(1);
        Power.setState(1);
        globalValues.TempSetValue = configValues.TempSetValue;
        Serial.print("TempSetValue: ");Serial.println(globalValues.TempSetValue);
        break;

      case HEATING:
        Serial.println("HEATING");
        Pump.setState(0);
        Valve.setState(0);
        if(_state != STARTUP) { globalValues.TempSetValue -= configValues.stateMaschine.BrewingTempSetpointIncrease; }
        Interface.activateValueScreen("Heating", &globalValues.TempValue, "degC",&globalValues.TempSetValue, &globalValues.Pid_Percent);
        break;

      case READY:
        Serial.println("READY");
        Interface.activateValueScreen("Ready", &globalValues.TempValue, "degC", &globalValues.TempSetValue, &globalValues.Pid_Percent);
        break;

      case SUPPLY:
        Serial.println("SUPPLY");
        globalValues.TempSetValue += configValues.stateMaschine.BrewingTempSetpointIncrease;
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
        Interface.activateTextScreen("Done!", "Flush ???");
        break;

      case FLUSH:
        Serial.println("FLUSH");
        Interface.activateTextScreen("Flushing...", "!! HOT !!");
        // Interface.flush();
        Timer.startTimer(configValues.stateMaschine.Flush_Duration);
        Valve.setState(1);
        Pump.setState(1);
        // delay(configValues.stateMaschine.Flush_Duration*1000);
        // Pump.setState(0);
        // Valve.setState(0);
        // Interface.activateTextScreen("Flushing...", "done!");
    }

    _state = newState;
  }

  // void PowerSwitch(bool on) {
  //   Power.setState(1);
  //   delay(1000);
  //   Power.setState(0);
  // }

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
