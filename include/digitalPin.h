
#ifndef __DIGITAL_PIN_H__
#define __DIGITAL_PIN_H__

#include <Arduino.h>

class ArduinoPins {
	uint8_t pin;
	bool activeLow;	// true -> the button connects the input pin to GND when pressed.
public:
	ArduinoPins(uint8_t p, bool aLow) 					{ activeLow=aLow; pin=p; }
  inline void setOutput()  										{ setLow(); pinMode(pin,OUTPUT); }
	inline void setInput(bool pullUp)						{ pinMode(pin, (pullUp?INPUT_PULLUP:INPUT) ); }
  inline void setInput()    									{ pinMode(pin,INPUT); }
  inline void setHigh()     									{ digitalWrite(pin,(activeLow?LOW:HIGH)); }
  inline void setLow()      									{ digitalWrite(pin,(activeLow?HIGH:LOW)); }
	inline void setState (bool on)							{ if(on) { setHigh(); } else { setLow(); } }
  inline bool getState() 									    { return (activeLow?(digitalRead(pin)==LOW):(digitalRead(pin)==HIGH)); }
};

/******************************************** SimplePin ********************************************/
#define DEBOUNCE_TIME			100 //[ms] for updateState function

template <uint8_t Pin1, bool output=true, bool activeLow=false>
class simplePin {

private:
	ArduinoPins	dPin;
	bool lastState;

public:
	simplePin() : dPin(Pin1, activeLow) {}

	void initialize() {
    if(output) {
		    dPin.setOutput();
    } else {
        dPin.setInput(activeLow);
				lastState = dPin.getState();
    }
  }

	void setState(bool state) {
		dPin.setState(state);
	}

  bool getState() {
    return dPin.getState();
  }

  bool isActive() {
    return getState();
  }

	bool updateState(bool *oldState) {
		bool s = getState();
		bool retVal = false;
		if(lastState != s && s) {
			*oldState = !(*oldState);
			retVal = true;
			delay(DEBOUNCE_TIME); //Debounce
		}
		lastState = s;
		return retVal;
	}
};

#endif
