
#ifndef __DIGITAL_PIN_H__
#define __DIGITAL_PIN_H__

#include <Arduino.h>
#ifdef ESP32
  #include <ESP32Ticker.h>
#else
  #include <Ticker.h>
#endif

class ArduinoPins {
	uint8_t pin;
	bool activeLow;	// true -> the button connects the input pin to GND when pressed.
	uint8_t channel;
public:
	ArduinoPins(uint8_t p, bool aLow) 									{ activeLow=aLow; pin=p; }
  inline void setOutput()  														{ setLow(); pinMode(pin,OUTPUT); }
	inline void setPWM(uint16_t freq, uint8_t _channel) { channel = _channel; ledcSetup(channel, freq, 8 /*resolution*/);	ledcAttachPin(pin, channel); ledcWriteTone(channel, freq); setDutyCycle(0); }
	inline void setDutyCycle(uint8_t dutyCycle) 				{ ledcWrite(channel, (uint8_t)(255 * (dutyCycle/100.))); }
	inline void setInput(bool pullUp)										{ pinMode(pin, (pullUp?INPUT_PULLUP:INPUT) ); }
  inline void setInput()    													{ pinMode(pin,INPUT); }
  inline void setHigh()     													{ digitalWrite(pin,(activeLow?LOW:HIGH)); }
  inline void setLow()      													{ digitalWrite(pin,(activeLow?HIGH:LOW)); }
	inline void setState (bool on)											{ if(on) { setHigh(); } else { setLow(); } }
  inline bool getState() 									    				{ return (activeLow?(digitalRead(pin)==LOW):(digitalRead(pin)==HIGH)); }
};

/******************************************** SimplePin ********************************************/
#define DEBOUNCE_TIME			100 // [ms] for updateState function
#define BEEP_DURATION			100 // [ms]
#define BEEP_DUTYCYCLE		 50 // [%]

template <uint8_t Pin1, bool output=true, bool activeLow=false, uint16_t freq=0, uint8_t channel=0>
class simplePin {

private:
	ArduinoPins	dPin;
	bool lastState;
	Ticker ticker;
	uint8_t beepNum;

public:
	simplePin() : dPin(Pin1, activeLow), beepNum(0) {}

	void initialize() {
    if(output) {
		    dPin.setOutput();
    } else {
        dPin.setInput(activeLow);
				lastState = dPin.getState();
    }
		if(freq) {
			dPin.setPWM(freq, channel);
		}
  }
//IRAM_ATTR
	static void staticLowBeep(simplePin *pThis) { pThis->lowBeep(); }
	void lowBeep() {
		if(beepNum) {
				dPin.setDutyCycle((beepNum & 0x01 ) ? 0 : 50);
				ticker.attach_ms(BEEP_DURATION, &simplePin::staticLowBeep, this);
				beepNum--;
		} else {
			dPin.setDutyCycle(0);
			ticker.detach();
		}
	}

	void beep(uint8_t num) {
		beepNum = num*2;
		lowBeep();
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
		if(lastState != s && s) { //state change and state=HIGH
      delay(DEBOUNCE_TIME); //Debounce
      s = getState();
      if(s) { // filter noise
        *oldState = !(*oldState); //save new state
			  retVal = true;
      }
		}
		lastState = s;
		return retVal;
	}
};

#endif
