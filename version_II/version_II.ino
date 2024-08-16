#include <TEA5767Radio.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define VOLUME_ERROR 1

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int volumePot = A3;
int mappedVolumeValue, prevMappedVolumeValue;

byte volume_character[] = { B00001, B00011, B10111, B10111, B10111, B10111, B00011, B00001 };
byte volume_bar[] = { B00000, B01110, B01110, B01110, B01110, B01110, B01110, B00000 };
byte radio_character[] = { B10000, B10000, B11111, B10001, B10001, B11111, B10111, B11111 };

float mapFloat(long x, long in_min, long in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void generate_volume_bars(int vol) {
  lcd.setCursor(0, 1);
  lcd.write(byte(1));//Write volume icon
  for (int i = 0; i < vol; i++) {
    lcd.write(byte(2));//Write volume bar
  }
}

void writeToLcd(char *frequency, int volume) {
  lcd.clear();
  lcd.setCursor(0, 0); // lcd.setCursor(col, row);

  lcd.write(byte(0));//Write radio icon
  lcd.print(" ");
  lcd.print(frequency);
  lcd.print(" MHz");

  generate_volume_bars(volume);
}

float getSmoothedReading(int pin, int numReadings) {
  analogReference("DEFAULT");//Max voltage = 5v for tune pot
  long sum = 0;
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(pin);
    delay(5); // Small delay to allow ADC to settle
  }
  return sum / numReadings;
}
float getMaxReading(int pin, int numReadings) {
  analogReference("INTERNAL");//1.1V inorder to more accurately read volume pot
  int max = 0;
  for (int i = 0; i < numReadings; i++) {
    if(max < analogRead(pin))
    {
      max = analogRead(pin);
    }
    delay(5); // Small delay to allow ADC to settle
  }
  return max;
}

void setup() {
  Wire.begin();
  delay(200); // Wait for I2C bus to stabilize
  
  Serial.begin(9600);
  lcd.init();
  delay(200); // Allow LCD to initialize

  lcd.backlight();
  lcd.clear();
  lcd.print("Yello!...");

  delay(500); // Give some time to display the initial message

  //Set initial frequency
  float smoothValFromTunePot = getSmoothedReading(tunePot, 10); // Average over 10 readings
  currentFreq = mapFloat(smoothValFromTunePot, 0, 1023, 88.9, 107.5);
  radio.setFrequency(currentFreq);

  lcd.createChar(0, radio_character);
  lcd.createChar(1, volume_character);
  lcd.createChar(2, volume_bar);
}
void loop() {
  float smoothTuneValue = getSmoothedReading(tunePot, 10); // Average over 10 readings
  currentFreq = mapFloat(smoothTuneValue, 0, 1023, 88.9, 107.5);
  currentFreq = quantizeFrequency(currentFreq);
  
  float maxVolumeValue = getMaxReading(volumePot, 50);
  mappedVolumeValue = map(maxVolumeValue, 0, 350, 0, 15);//Read value doesn't get to 1023

  Serial.print(smoothTuneValue);
  Serial.print(" ");
  Serial.println(maxVolumeValue);

  if (abs(prevFreq - currentFreq) > FREQ_ERROR) {
    radio.setFrequency(currentFreq);
    dtostrf(currentFreq, 5, 1, freqStr);
    prevFreq = currentFreq;
    writeToLcd(freqStr, mappedVolumeValue);
  }
  else if(abs(mappedVolumeValue - prevMappedVolumeValue) > VOLUME_ERROR) {
    writeToLcd(freqStr, mappedVolumeValue);
    prevMappedVolumeValue = mappedVolumeValue; 
  }

  //delay(400); // Adjust delay as needed
}
