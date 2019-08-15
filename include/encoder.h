
#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <ClickEncoder.h>
#include <Ticker.h>

#ifndef ESP32
  #define IRAM_ATTR
#endif
#define ENCODER_TICKER_MS     1 //[ms]
#define ENCODER_STEPS         4 //[-]

/******************************************** ClickEncoder ********************************************/

  enum EncoderEvent {
    ENC_NOTHING=0,
    ENC_BUTTON_PRESSED,
    ENC_BUTTON_HELD,
    ENC_ROTATE_LEFT,
    ENC_ROTATE_RIGHT
  };

  template <uint8_t PinA, uint8_t PinB, uint8_t PinBtn>
  class ClassEncoder {

  private:
    Ticker ticker;
    int16_t oldEncPos, encPos;
    // uint8_t buttonState;

    EncoderEvent oldButtonState;

  public:
    ClickEncoder encoder;

  	ClassEncoder() : encoder(PinA, PinB, PinBtn, ENCODER_STEPS) {

    }


    static void IRAM_ATTR staticEncoderTicker(ClassEncoder *pThis) { pThis->encoderTicker(); }
    void IRAM_ATTR encoderTicker() {
      encoder.service();
    }

    void initialize() {
      ticker.attach_ms(ENCODER_TICKER_MS, &ClassEncoder::staticEncoderTicker, this);
      encoder.setAccelerationEnabled(true);
      encoder.setDoubleClickEnabled(false);
      encoder.setButtonHeldEnabled(true);

      oldEncPos = -1;
    }

    enum EncoderEvent poll() {
      encPos += encoder.getValue();
      EncoderEvent retVal = ENC_NOTHING;

      if (encPos != oldEncPos) {
        Serial.print("Encoder Value: ");
        Serial.println(encPos);
        retVal = (encPos > oldEncPos ? ENC_ROTATE_RIGHT : ENC_ROTATE_LEFT);
        oldEncPos = encPos;
      }

      ClickEncoder::Button b = encoder.getButton();
      if (b != ClickEncoder::Open) {
        Serial.print("Button: ");
        #define VERBOSECASE(label) case label: Serial.println(#label); break;
        switch (b) {
          // VERBOSECASE(ClickEncoder::Pressed);
          // VERBOSECASE(ClickEncoder::Held)
          // VERBOSECASE(ClickEncoder::Released)
          // VERBOSECASE(ClickEncoder::Clicked)
          // VERBOSECASE(ClickEncoder::DoubleClicked)
          case ClickEncoder::Held:
            // Serial.println("Held");
            retVal = (oldButtonState == ENC_BUTTON_HELD ? ENC_NOTHING : ENC_BUTTON_HELD);
            break;
          case ClickEncoder::Clicked:
            Serial.println("Clicked");
            retVal = ENC_BUTTON_PRESSED;
            break;

          default:
            break;
        }
      }

      return retVal;
    }
  };

#endif
