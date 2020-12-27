#include <Wire.h>
#include <MS5611.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include <SPI.h>
#include <SD.h>
#include <stdio.h>

#define SD_CHIP_SELECT 4
#define BUFFER_SIZE 80
#define LOG_FILE "data.log"

char data[BUFFER_SIZE];
char altBuffer[20];
char tempBuffer[10];

MPU6050 MPU6050;
int16_t ax, ay, az;
int16_t gx, gy, gz;

MS5611 MS5611;
double referencePressure;

void setupMS5611() {
    Serial.println("Initializing MS5611 sensor");
    if (!MS5611.begin()) {
        Serial.println("MS5611 init failed");
        return;
    }
    Serial.println("MS5611 initialized.");
    
    // Get reference pressure for relative altitude
    referencePressure = MS5611.readPressure();    
}

void setupMPU6050() {
    Serial.println("Initializing MPU6050 sensor");
    MPU6050.initialize();

    Serial.println("Testing MPU6050 connection...");
    Serial.println(MPU6050.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
}

void setupSDcard() {
    Serial.print("Initializing SD card");
    
    if (!SD.begin(SD_CHIP_SELECT)) {
        Serial.println("Card failed, or not present");
        return;
    }
    
    if (SD.exists(LOG_FILE) && !SD.remove(LOG_FILE)) {
        Serial.println("Warning, failed to delete existing log");
    }
    
    Serial.println("SD card initialized.");
}

void setup() {
    // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
#endif

    Serial.begin(115200);
    setupMS5611();
    setupMPU6050();
    setupSDcard();
}

void loop() {
    unsigned long timer = millis();
    MPU6050.getAcceleration(&ax, &ay, &az);
    long realPressure = MS5611.readPressure();
    double temp = MS5611.readTemperature();
    
    double absoluteAltitude = MS5611.getAltitude(realPressure);
    double relativeAltitude = MS5611.getAltitude(realPressure, referencePressure);

    dtostrf(relativeAltitude, 10, 1, altBuffer);
    dtostrf(temp, 7, 2, tempBuffer);
    sprintf(data, "%lu,%s,%s,%ld,%d", timer, altBuffer, tempBuffer, realPressure, ax);
    Serial.println(data);
    
    delay(500);
    
    File file = SD.open(LOG_FILE, FILE_WRITE);
    if (file) {
        file.println(data);
        file.close();
    }
}
