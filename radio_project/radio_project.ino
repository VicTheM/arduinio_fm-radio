#include <TEA5767Radio.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DEFAULT_FREQ 90.0                       // initial frequency in MHz
#define ERROR 0.1                               // allowable discrepancy in any reading

/*-------------------------------- GLOBAL CONSTANTS ---------------------------------------*/
LiquidCrystal_I2C lcd(0x27, 16, 2);             // addrress may vary based on arduino
TEA5767Radio radio = TEA5767Radio();

const int tunePot = A1;
char freqStr[10];
float currentFreq = DEFAULT_FREQ;
const float broadcastFrequencies[] = {
  88.9, 89.7, 90.9, 91.3, 92.3, 93.7, 94.7, 95.1, 95.7, 96.1, 96.9, 97.3, 98.1, 99.3, 99.9, 100.5,
  101.5, 102.3, 103.1, 103.5, 104.3, 105.1, 105.9, 106.5, 107.5
  };
const int numBroadcastFrequencies = sizeof(broadcastFrequencies) / sizeof(broadcastFrequencies[0]);
const float nonBroadcastFrequencies[] = {
  89.3, 90.3, 91.8, 92.8, 93.2, 94.2, 95.4, 96.5, 97.1, 98.0, 98.7,
  99.5, 100.1, 101.0, 101.8, 102.7, 104.0, 104.7, 105.5, 106.1, 107.0
  };
const int numNonBroadcastFrequencies = sizeof(nonBroadcastFrequencies) / sizeof(nonBroadcastFrequencies[0]);


/* ------------------------------ FUNCTION DEFINITIONS ------------------------------------*/
float mapFloat(long x, long in_min, long in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


/**
 * This function write the current station to the lcd
 */
void writeToLcd(char *frequency) {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (strcmp(frequency, "103.1") == 0) {
    lcd.print("UNILAG FM 103.1");
  }
  else if (strcmp(frequency, "95.1") == 0) {
    lcd.print("WAZOBIA FM 95.1");
  }
  else {
    lcd.print("Freq: ");
    lcd.print(frequency);
  }
}


/**
 * This function returns the mean of a set of readings from a POT
 */
float getSmoothedReading(int pin, int numReadings) {
  long sum = 0;
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(pin);
    delay(10); // Small delay to allow ADC to settle
  }
  return sum / numReadings;
}


/**
 * This function picks from our set of predefined frequency
 * based on the proximity of the parameter passed to it
 * It helps quantize our frequency band thereby increasing
 * tunning accuracy
 */
float quantizeFrequency(float freq) {
  float closestFreq = broadcastFrequencies[0];
  float minDiff = abs(freq - closestFreq);

  // Compare with broadcast frequencies
  for (int i = 1; i < numBroadcastFrequencies; i++) {
    float diff = abs(freq - broadcastFrequencies[i]);
    if (diff < minDiff) {
      closestFreq = broadcastFrequencies[i];
      minDiff = diff;
    }
  }

  // Compare with non-broadcast frequencies
  for (int i = 0; i < numNonBroadcastFrequencies; i++) {
    float diff = abs(freq - nonBroadcastFrequencies[i]);
    if (diff < minDiff) {
      closestFreq = nonBroadcastFrequencies[i];
      minDiff = diff;
    }
  }

  return closestFreq;
}

/*------------------------------- ENTRY POINT -------------------------------------*/
void setup() {
  Wire.begin();
  delay(200);                                           // Wait for I2C bus to stabilize
  lcd.init();
  delay(200);                                           // Allow LCD to initialize
  lcd.backlight();
  lcd.clear();
  lcd.print("EEG 326 PROJECT");
  delay(1000);
  
  radio.setFrequency(currentFreq);
}

float prevFreq = DEFAULT_FREQ;
void loop() {
  float smoothVal = getSmoothedReading(tunePot, 10);      // Average over 10 readings
  currentFreq = mapFloat(smoothVal, 0, 1023, 88.9, 107.5);
  currentFreq = quantizeFrequency(currentFreq);

  if (abs(prevFreq - currentFreq) > ERROR) {
    radio.setFrequency(currentFreq);
    dtostrf(currentFreq, 5, 1, freqStr);
    writeToLcd(freqStr);
    prevFreq = currentFreq;
  }
  delay(500);
}