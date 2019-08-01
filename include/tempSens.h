#ifndef __TEMP_SENS_H__
#define __TEMP_SENS_H__

/* TSIC 306 Sensor */
#include <TSIC.h>

/* Dallas Sensor */
#include <OneWire.h>
#include <DallasTemperature.h>

/* Resolution | Conversion Time
           9  | 94 ms
          10  | 188 ms
          11  | 375 ms
          12  | 750 ms
*/
#define TEMP_DALLAS_SENS_RESOLUTION         10 //[bit]

#ifdef ESP32
  #include <ESP32Ticker.h>
#else
  #include <Ticker.h>
#endif

#include <MovingAverage.h>  //https://github.com/pilotak/MovingAverage

#define MOVING_AVERAGE_DATAPOINTS     4 //"binary" length only: 2, 4, 8, 16, 32, 64, 128, etc.

template <typename cSensor>
class ClassTempSens {

private:
  Ticker ticker;

  cSensor sensor;
  double *curAvgValue;

  uint16_t value;
  MovingAverage<uint16_t, MOVING_AVERAGE_DATAPOINTS> AvgValue;

public:

  ClassTempSens() { }

  bool initialize(double *AvgValuePtr) {
    curAvgValue = AvgValuePtr;
    AvgValue.reset();
    value = 0;
    *curAvgValue = 0;
    return sensor.initialize();
  }

  void setAutoSampling(bool on=true) {
    if(on) {
      // ticker.attach_ms(sensor.getMinSamplingTime(), std::bind(&ClassTempSens::updateValue, this));
      // ticker.attach_ms(sensor.getMinSamplingTime(), [this](){ this->updateValue(); });
      ticker.attach_ms(sensor.getMinSamplingTime(), &ClassTempSens::staticUpdateValue,this);
    } else {
      ticker.detach();
    }
    sensor.setAutoSampling(on);
  }

  static void IRAM_ATTR staticUpdateValue(ClassTempSens *pThis) { pThis->updateValue(); }
  void updateValue() {
    float newValue = sensor.updateValue();
    if(sensor.checkPlausibility(newValue, (value/100.))) {
      value = (uint16_t)(newValue*100.);
      *curAvgValue = (double)(AvgValue.add(value)/100.);
    } else {
      Serial.print("TempSens value skipped: ");Serial.print(newValue);
      Serial.print(" oldValue ");Serial.println((float)(value/100.));
    }
  }

  float getValue() {
    return (float)(value/100.);
  }

  float getAvgValue() {
    return (float)(AvgValue.get()/100.);
  }


};

/*********************************** SENSORS ***********************************/

template <uint8_t SensorPin=32>
class ClassTSIC {
private:
  TSIC sensor;
  uint16_t temperature = 0;
  float Temperatur_C = 0;
public:
  ClassTSIC() : sensor(SensorPin, NO_VCC_PIN, TSIC_30x) { }
  bool initialize() { return true; }
  void setAutoSampling(bool on=true) {} // not supported
  float getMinSamplingTime() { return 250; } // [ms]

  bool checkPlausibility(float CheckValue, float oldValue) {
    if(oldValue <= 0.1) return true; //init check
    if( (CheckValue < 150) && (CheckValue > 10) && (abs(CheckValue-oldValue) <= 15 )) {
      return true;
    }
    return false;
  }

  float updateValue() {
    uint16_t value = 0;
    cli();
    if(sensor.getTemperature(&value)) {
      sei();
      return sensor.calc_Celsius(&value);
    } else {
      sei();
      return 0;
    }
  }
};

template <uint8_t SensorPin=2>
class ClassDallas {

private:
  OneWire oneWire;
  DallasTemperature sensor;
  DeviceAddress sensorDeviceAddress;

  uint8_t sensResolution;

public:
  ClassDallas() : oneWire(SensorPin), sensor(&oneWire), sensResolution(TEMP_DALLAS_SENS_RESOLUTION) { }

  bool initialize() {

    sensor.begin();
    sensor.getAddress(sensorDeviceAddress, 0);
    sensor.setResolution(sensorDeviceAddress, sensResolution);

    return sensor.validAddress(sensorDeviceAddress);
  }

  void setAutoSampling(bool on=true) {
    if(on) {
      sensor.setWaitForConversion(false);
      sensor.requestTemperatures();
    } else {
      sensor.setWaitForConversion(true);
    }
  }

  float updateValue() {
    cli();
    float buf =  sensor.getTempCByIndex(0);
    sei();
    sensor.requestTemperatures();
    return buf;
  }

  bool checkPlausibility(float CheckValue, float oldValue) {
    if(oldValue <= 0.1) return true; //init check
    if( (CheckValue < 125) && (CheckValue > 10) && (abs(CheckValue-oldValue) <= 15 )) {
      return true;
    }
    return false;
  }

  float getMinSamplingTime() { return 300; } // [ms]
};

#endif
