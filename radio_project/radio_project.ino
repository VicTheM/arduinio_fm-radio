/*--------------------------------------------------------------------------------------------------------

THE ALGORITHM
This was developed originally for Arduino Uno

LAST UPDATED: August 17, 2024
---------------------------------------------------------------------------------------------------------*/

#include <TEA5767Radio.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DEFAULT_FREQ 95.1
#define FREQ_ERROR 0.1

/*--------------------------------------- VARIABLE DECLARATIONS -----------------------------------------*/
LiquidCrystal_I2C lcd(0x27, 16, 2);
TEA5767Radio radio = TEA5767Radio();

const int tunePot = A1;
char freqStr[10];

float currentFreq = DEFAULT_FREQ;
float prevFreq = DEFAULT_FREQ;
float smoothTuneValue;
float smoothValFromTunePot;

byte radio_character[] = { B10000, B10000, B11111, B10001, B10001, B11111, B10111, B11111 };

// Broadcasting frequencies
const float broadcastFrequencies[] = {88.9, 89.7, 90.9, 91.3, 92.3, 93.7, 94.7, 95.1, 95.7, 96.1, 96.9, 97.3, 98.1, 99.3, 99.9, 100.5, 101.5, 102.3, 103.1, 103.5, 104.3, 105.1, 105.9, 106.5, 107.5};
const int numBroadcastFrequencies = sizeof(broadcastFrequencies) / sizeof(broadcastFrequencies[0]);

// Non-broadcasting intermediate frequencies
const float nonBroadcastFrequencies[] = {89.3, 90.3, 91.8, 92.8, 93.2, 94.2, 95.4, 96.5, 97.1, 98.0, 98.7, 99.5, 100.1, 101.0, 101.8, 102.7, 104.0, 104.7, 105.5, 106.1, 107.0};
const int numNonBroadcastFrequencies = sizeof(nonBroadcastFrequencies) / sizeof(nonBroadcastFrequencies[0]);


/*---------------------------------- FUNCTION D3EFINITIONS ----------------------------------------------*/
/**
 * mapFloat - This function changes the range or scale of a value
 * @x: The value you want to transform
 * @in_min: The minimum input value @x can have
 * @in_max: The maximum value @x can have
 * @out_mun: The new minimum you want
 * @out_max: The new maximum you want
 *
 * Returns - Returns @x represented in the new scale
 */
float mapFloat(long x, long in_min, long in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


/**
 * writeFrequency - This function write the current station to the lcd
 * @frequency: A c_str that contains the frequency in 2 dp
 * Description: The frequency takes only the upper half of the lcd
 * so we clear it first by printing spaces befroe writing again
 */
void writeToLcd(char *frequency, int volume) {
  lcd.clear();
  lcd.setCursor(0, 0); // lcd.setCursor(col, row);
  lcd.write(byte(0));//Write radio icon
  lcd.print(" ");

  if (strcmp(frequency, "103.1") == 0) {
    lcd.print("UNILAG FM 103.1");
  }
  else if (strcmp(frequency, " 95.1") == 0) {
    lcd.print("WAZOBIA FM 95.1");
  }
  else {
    lcd.print(frequency);
    lcd.print(" MHz");
  }
}


/**
 * getSmoothedReadings - This function returns the mean of a set of readings from a POT
 * @pin: The analog pin you want to read from
 * @numReadings: The number of sample values before averaging
 *
 * Returns - The average 
 */
float getSmoothedReading(int pin, int numReadings) {
  long sum = 0;
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(pin);
    delay(5); // Small delay to allow ADC to settle
  }
  return sum / numReadings;
}


/**
 * This function picks from our set of predefined frequency
 * based on the proximity of the parameter passed to it
 * It helps quantize our frequency band thereby increasing
 * tunning accuracy
 *
 * NOTE: It uses some global variables (the static arrays
 * defined at the top of this file) To change the frequency
 * point edit the array
 *
 * Returns - The predefined frequncy closest to the input
 */
float quantizeFrequency(float freq) {
  float closestFreq = nonBroadcastFrequencies[0];
  float minDiff = abs(freq - closestFreq);

  // Compare with broadcast frequencies
  for (int i = 0; i < numBroadcastFrequencies; i++) {
    float diff = abs(freq - broadcastFrequencies[i]);
    if (diff < minDiff) {
      closestFreq = broadcastFrequencies[i];
      minDiff = diff;
    }
  }
  // Compare with non-broadcast frequencies
  for (int i = 1; i < numNonBroadcastFrequencies; i++) {
    float diff = abs(freq - nonBroadcastFrequencies[i]);
    if (diff < minDiff) {
      closestFreq = nonBroadcastFrequencies[i];
      minDiff = diff;
    }
  }
  
  return closestFreq;
}



/*------------------------------------------ ENTRY POINT -----------------------------------------------*/
void setup() {
  Wire.begin();
  delay(200);
  lcd.init();
  delay(200);
  lcd.backlight();
  lcd.clear();
  lcd.print("Yello!...");
  delay(1500);

  lcd.createChar(0, radio_character);

  //Set initial frequency
  smoothValFromTunePot = getSmoothedReading(tunePot, 10);
  currentFreq = mapFloat(smoothValFromTunePot, 0, 1023, 88.9, 107.5);
  currentFreq = quantizeFrequency(currentFreq);
  dtostrf(currentFreq, 5, 1, freqStr);
  writeToLcd(freqStr);
  radio.setFrequency(currentFreq);
  prevFreq = currentFreq;
}

void loop() {
  smoothTuneValue = getSmoothedReading(tunePot, 10); // Average over 10 readings
  currentFreq = mapFloat(smoothTuneValue, 0, 1023, 88.9, 107.5);
  currentFreq = quantizeFrequency(currentFreq);


  if (abs(prevFreq - currentFreq) > FREQ_ERROR) {
    radio.setFrequency(currentFreq);
    dtostrf(currentFreq, 5, 1, freqStr);
    prevFreq = currentFreq;
    writeToLcd(freqStr);
  }
  
  delay(500);
}
