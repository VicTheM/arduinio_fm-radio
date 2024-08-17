/*-----------------------------------------------------------------------------------------
  THERE ARE 4 VOLUME POINTS (25, 50, 75, 100). TO INCREASE THE NUMBER OF POSSIBLE VOLUMES
  YOU MAY CHANGE THE VOLUME POINTS TO (10, 20, 30, 40, 50, 60, 70, 80, 90, 100)
  LIKEWISE WHERE mapFloat() IS CALLED FOR VOLUME, INSTEAD OF USING 0 AND 1023 AS THE
  RANGE OF INPUT VALUES, YOU MAY USE 5 TO 1000 CONSIDERING THE POTENTIOMETER NEVER
  REACHES IT'S MAXIMUM VALUE

  * IF THE VOLUME CHANGES ON ITS OWN, REPLACE THE TOLERANCE VALUE BY TWICE ITSELF
  * IF THE VOLUME NEVER CHANGES AT ALL OR IS TOO SLOW TO CHANGE REPLACE THE TOLERANCE
    VALUE BY HALF ITSELF AND ALSO ADD MORE VALUES TO THE VOLUME POINTS ARRAY
-------------------------------------------------------------------------------------------*/

#include <TEA5767Radio.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DEFAULT_FREQ 95.1
#define ERROR 0.3
#define TOLERANCE 10

/*--------------------- Radio specific variables and constants  ----------------------------*/
LiquidCrystal_I2C lcd(0x27, 16, 2);                    // addrress may vary based on arduino
TEA5767Radio radio = TEA5767Radio();
const int tunePot = A1;
char freqStr[10];
const float broadcastFrequencies[] = {
  88.9, 89.7, 90.9, 91.3, 92.3, 93.7, 94.7, 95.1, 96.1, 96.9, 97.3, 98.1, 99.3, 99.9, 100.5,
  101.5, 102.3, 103.1, 103.5, 104.3, 105.1, 105.9, 106.5, 107.5
  };
const int numBroadcastFrequencies = sizeof(broadcastFrequencies) / sizeof(broadcastFrequencies[0]);
const float nonBroadcastFrequencies[] = {
  89.3, 90.3, 91.8, 92.8, 93.2, 94.2, 95.8, 96.5, 97.1, 98.0, 98.7,
  99.5, 100.1, 101.0, 101.8, 102.7, 104.0, 104.7, 105.5, 106.1, 107.0
  };
const int numNonBroadcastFrequencies = sizeof(nonBroadcastFrequencies) / sizeof(nonBroadcastFrequencies[0]);
float currentFreq = DEFAULT_FREQ;
float prevFreq = DEFAULT_FREQ;
float smoothFreq;


/*-------------------- Volume specific variables and constants -----------------------------*/
const int volume_pot = A3;
float prevVol = 0.0;
float currentVol = 0.0;
float smoothVol;
char volStr[10];
byte volume_character[] = { B00001, B00011, B10111, B10111, B10111, B10111, B00011, B00001 };
const float volumePoints[] = { 0.0, 25.0, 50.0, 75.0, 100 };
const int numVolumePoints = sizeof(volumePoints) / sizeof(float);



/* ------------------------------ FUNCTION DEFINITIONS ------------------------------------*/
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
void writeFrequency(char *frequency) {
  lcd.setCursor(0, 0);
  lcd.print("               ");                                                 // 16 spaces
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
 * writeVolume - This function write the volume to the lower row of the lcd
 * @volume: a c_str having the volume value between 0 and 100
 * Description: the volume icon is constant so it takes the first column
 * then a constat space takes the second column, so we have just the remaining
 * 14 spaces for printing the volume variable
 */
void writeVolume(char *volume) {
  lcd.setCursor(2, 1);
  lcd.print("              ");                                                // 14 spaces
  lcd.setCursor(2, 1);
  lcd.print(volume);
  lcd.print(" %");
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
    delay(10);                                          // Small delay to allow ADC to settle
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


/**
 * This function picks from our set of predefined volume for
 * display based on the closness of the parameter passed to it
 *
 * NOTE: It uses some global variables (the static arrays
 * defined at the top of this file) To change the volume point
 * edit the array
 *
 * Returns - The predefined volume closest to the input
 */
float quantizeVolume(float vol) {
  float closestVol = volumePoints[0];
  float minDiff = abs(vol - closestVol);
  float diff = 0.0;

  for (int i = 1; i < numVolumePoints; i++) {
    diff = abs(vol - volumePoints[i]);
    if (diff < minDiff) {
      minDiff = diff;
      closestVol = volumePoints[i];
    }
  }

  return closestVol;
}



/*----------------------------------- ENTRY POINT ----------------------------------------*/
void setup() {
  Wire.begin();
  delay(200);                                               // Wait for I2C bus to stabilize
  lcd.init();
  delay(200);                                               // Allow LCD to initialize
  lcd.backlight();
  lcd.clear();
  lcd.print("EEG 326 PROJECT");
  delay(1000);
  lcd.clear();

  lcd.createChar(0, volume_character);
  lcd.setCursor(0, 1);
  lcd.write(0);                                               // volume icon, supposedly
  
  currentVol = quantizeVolume(mapFloat(getSmoothedReading(volume_pot, 2), 0, 1023, 0.0, 100.0));
  prevVol = currentVol;
  dtostrf(currentVol, 3, 1, volStr);
  writeVolume(volStr);

  radio.setFrequency(currentFreq);
  dtostrf(currentFreq, 5, 1, freqStr);
  writeFrequency(freqStr);
}

float prevSmoothVol = getSmoothedReading(volume_pot, 10);
void loop() {
  smoothFreq = getSmoothedReading(tunePot, 10);
  currentFreq = mapFloat(smoothFreq, 0, 1023, 88.9, 107.5);
  currentFreq = quantizeFrequency(currentFreq);
  if (abs(prevFreq - currentFreq) > ERROR) {
    radio.setFrequency(currentFreq);
    dtostrf(currentFreq, 5, 1, freqStr);
    writeFrequency(freqStr);
    prevFreq = currentFreq;
  }

  smoothVol = getSmoothedReading(volume_pot, 10);
  if (abs(volume_pot - prevSmoothVol) > TOLERANCE) {
    currentVol = mapFloat(smoothVol, 0, 1023, 0.0, 100.0);
    currentVol = quantizeVolume(currentVol);
    if (abs(prevVol - currentVol) > ERROR) {
      dtostrf(currentVol, 3, 1, volStr);
      writeVolume(volStr);
      prevVol = currentVol;
      prevSmoothVol = getSmoothedReading(volume_pot, 10);
    }    
  }
  delay(500);
}