#ifndef __PWM_H__
#define __PWM_H__

template <typename cDigitalOutPut, unsigned long period>
class ClassPWM {

private:
  unsigned long  PWM_WindowStartTime;
  cDigitalOutPut dOutput;

public:

  ClassPWM() { }

  void initialize() {
    PWM_WindowStartTime = millis();
    dOutput.initialize();
    dOutput.setState(HIGH); delay(100); dOutput.setState(LOW); // short toggle
  }

  void poll(double currentWindowPosition) {
    /************************************************
     * turn the output pin on/off based on pid output
     ************************************************/
    if (millis() - PWM_WindowStartTime > period) {
      //time to shift the Relay Window
      PWM_WindowStartTime += period;
    }
    if (currentWindowPosition < millis() - PWM_WindowStartTime)  {
      dOutput.setState(LOW);
    } else {
      dOutput.setState(HIGH);
    }
  }

};


#endif
