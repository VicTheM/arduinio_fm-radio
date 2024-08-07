#include <TEA5767Radio.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DEFAULT_FREQ 90.0
#define ERROR 0.1

LiquidCrystal_I2C lcd(0x27, 16, 2);
TEA5767Radio radio = TEA5767Radio();

const int tunePot = A1;
char freqStr[10];
float currentFreq = DEFAULT_FREQ; // Initial frequency in MHz

const float frequencies[] = {88.9, 89.7, 90.9, 91.3, 92.3, 93.7, 94.7, 95.1, 95.7, 96.1, 96.9, 97.3, 98.1, 99.3, 99.9, 100.5, 101.5, 102.3, 103.1, 103.5, 104.3, 105.1, 105.9, 106.5, 107.5};
const int numFrequencies = sizeof(frequencies) / sizeof(frequencies[0]);

float mapFloat(long x, long in_min, long in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void writeToLcd(char *frequency) {
  lcd.clear();
  lcd.setCursor(0, 0); // lcd.setCursor(col, row);
  lcd.print("Freq: ");
  lcd.print(frequency);
}

void setup() {
  Wire.begin();
  delay(200); // Wait for I2C bus to stabilize
  
  lcd.init();
  delay(200); // Allow LCD to initialize
  
  lcd.backlight();
  lcd.clear();
  lcd.print("Radio..");
  
  delay(500); // Give some time to display the initial message
  
  radio.setFrequency(currentFreq); // Set the initial frequency to 90.0 MHz
}

float getSmoothedReading(int pin, int numReadings) {
  long sum = 0;
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(pin);
    delay(10); // Small delay to allow ADC to settle
  }
  return sum / numReadings;
}

float quantizeFrequency(float freq) {
  float closestFreq = frequencies[0];
  float minDiff = abs(freq - closestFreq);
  for (int i = 1; i < numFrequencies; i++) {
    float diff = abs(freq - frequencies[i]);
    if (diff < minDiff) {
      closestFreq = frequencies[i];
      minDiff = diff;
    }
  }
  return closestFreq;
}

float prevFreq = DEFAULT_FREQ;
void loop() {
  float smoothVal = getSmoothedReading(tunePot, 10); // Average over 10 readings
  // float smoothVal = analogRead(tunePot);
  currentFreq = mapFloat(smoothVal, 0, 1023, 88.9, 107.5);
  currentFreq = quantizeFrequency(currentFreq);

  if (abs(prevFreq - currentFreq) > ERROR) {
    radio.setFrequency(currentFreq);
    dtostrf(currentFreq, 5, 1, freqStr);
    writeToLcd(freqStr);
    prevFreq = currentFreq;
  }
  delay(500); // Adjust delay as needed
}
