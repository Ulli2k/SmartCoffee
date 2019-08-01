#ifndef __PID_H__
#define __PID_H__

#ifdef ESP32
  #include <ESP32Ticker.h>
#else
  #include <Ticker.h>
#endif
#include <PIDmod.h>

#define PID_Kp      30
#define PID_Ki      0.1
#define PID_Kd      0//0.25

#define PID_ACTIVATE_BOOST_PARAMETER      1  // 0=off, 1=on
#define PID_GAP_4_BOOST_PARAMETER         30 // [°C]
#define PID_KpBoost   100
#define PID_KiBoost   0
#define PID_KdBoost   0

#define PID_OUTPUT_LIMIT_MIN                   0
#define PID_OUTPUT_LIMIT_MAX                1000  //0-1023

//Define the aggressive and conservative Tuning Parameters
class ClassPID {

private:
  Ticker ticker;

  PID *myPID;
  double *PID_Setpoint;
  double *PID_Input;
  double *PID_Output;
  // double consKp=PID_Kp;
  // double consKi=PID_Ki;
  // double consKd=PID_Kd;
  // double boostKp=PID_KpBoost;
  // double boostKi=PID_KiBoost;
  // double boostKd=PID_KdBoost;

  // uint8_t Gap4BoostParameter=(PID_ACTIVATE_BOOST_PARAMETER ? PID_GAP_4_BOOST_PARAMETER : 0);

public:

  ClassPID(double *SetPoint, double *Input, double *Output) {
    PID_Setpoint = SetPoint;
    PID_Input = Input;
    PID_Output = Output;
    configValues.PID.consKp = PID_Kp;
    configValues.PID.consKi = PID_Ki;
    configValues.PID.consKd = PID_Kd;
    configValues.PID.Gap4BoostParameter = (PID_ACTIVATE_BOOST_PARAMETER ? PID_GAP_4_BOOST_PARAMETER : 0);
    configValues.PID.boostKp=PID_KpBoost;
    configValues.PID.boostKi=PID_KiBoost;
    configValues.PID.boostKd=PID_KdBoost;
    myPID = new PID(PID_Input, PID_Output, PID_Setpoint, configValues.PID.consKp, configValues.PID.consKi, configValues.PID.consKd, DIRECT);
  }

  void initialize( uint16_t CalcPeriod /* [ms] */, uint16_t outputMax=PID_OUTPUT_LIMIT_MAX) {
    myPID->SetSampleTime(CalcPeriod);
    myPID->SetOutputLimits(PID_OUTPUT_LIMIT_MIN,outputMax);
    //turn the PID on
    myPID->SetMode(AUTOMATIC);
    // ticker.attach_ms(CalcPeriod, std::bind(&ClassPID::calcPID, this));
    ticker.attach_ms(CalcPeriod, &ClassPID::staticCalcPID, this);
    // ticker.detach();
  }

  void setBoostParameter(uint8_t gap, double Kp=PID_KpBoost, double Ki=PID_KiBoost, double Kd=PID_KdBoost) {
    configValues.PID.Gap4BoostParameter = gap;
    configValues.PID.boostKp=Kp;
    configValues.PID.boostKi=Ki;
    configValues.PID.boostKd=Kd;
  }

  void setParameter(double Kp=PID_Kp, double Ki=PID_Ki, double Kd=PID_Kd) {
    configValues.PID.consKp=Kp;
    configValues.PID.consKi=Ki;
    configValues.PID.consKd=Kd;
  }

  static void staticCalcPID(ClassPID *pThis) { pThis->calcPID(); }
  void calcPID() {
    if(myPID->ComputeInterruptDriven()) {
      // Serial.print("Ist: ");Serial.println(*PID_Input);Serial.print("Soll: ");Serial.println(*PID_Setpoint);
      // Serial.print("Gap: ");Serial.println(gap);
      // Serial.print("Output: ");Serial.println(*PID_Output);
      // Serial.println("compute");
    } else {
      Serial.println("PID: compute to early");
    }
  }

  bool poll() {

    uint8_t gap = (uint8_t)abs(*PID_Setpoint - *PID_Input); //distance away from setpoint
    if (!configValues.PID.Gap4BoostParameter || gap < configValues.PID.Gap4BoostParameter) {  //we're close to setpoint, use conservative tuning parameters
      myPID->SetTunings(configValues.PID.consKp, configValues.PID.consKi, configValues.PID.consKd);
    } else { //we're far from setpoint, use aggressive tuning parameters
      myPID->SetTunings(configValues.PID.boostKp, configValues.PID.boostKi, configValues.PID.boostKd);
    }

    return true;
    // return myPID->Compute(); --> via Interrupt
  }

};


#endif
