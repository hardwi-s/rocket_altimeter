#include <Wire.h>
#include <MS5611.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include <SPI.h>
#include <SD.h>

#define SD_CHIP_SELECT 4
#define OUTPUT_READABLE_ACCELGYRO

unsigned long time;

MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;

MS5611 ms5611;
double referencePressure;
bool blinkState = false;

void setup() {
	// join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
	Wire.begin();
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  	Fastwire::setup(400, true);
#endif

	Serial.begin(9600);

	// Initialize MS5611 sensor
	Serial.println("Initialize MS5611 Sensor");

	#if 1
	while (!ms5611.begin()) {
		Serial.println("Could not find a valid MS5611 sensor, check wiring!");
		delay(500);
	}

	// Get reference pressure for relative altitude
	referencePressure = ms5611.readPressure();
  	#endif

	// Check settings
	checkSettings();

	// initialize device
	Serial.println("Initializing I2C devices...");
	accelgyro.initialize();

	// verify connection
	Serial.println("Testing device connections...");
	Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CHIP_SELECT)) { // see if the card is present and can be initialized:
		Serial.println("Card failed, or not present"); // don't do anything more:
		return;
	}
	Serial.println("card initialized.");
}

void checkSettings() {
	Serial.print("Oversampling: ");
	Serial.println(ms5611.getOversampling());
}

void loop() {
	// read raw accel measurements from device
	accelgyro.getAcceleration(&ax, &ay, &az);

	// Read raw pressure and temp values
	uint32_t rawTemp = ms5611.readRawTemperature();
	uint32_t rawPressure = ms5611.readRawPressure();

	// Read true temperature & Pressure
	double realTemperature = ms5611.readTemperature();
	long realPressure = ms5611.readPressure();

	// Calculate altitude
	float absoluteAltitude = ms5611.getAltitude(realPressure);
	float relativeAltitude = ms5611.getAltitude(realPressure,
			referencePressure);

	//change variables to strings
	String comma = String(',');
	String absalt = String(ms5611.getAltitude(realPressure)); //absoloute altitude in meters.
	String temp = String(ms5611.readTemperature()); //real temp in degrees, may over read initially, will take time for sensor to stabilize.
	String realpressure = String(ms5611.readPressure());  // pressure in pascals
	String accelXraw = String(ax); // raw accel in X. Divide by 2048 to get a "G" reading.
	String timer = String(millis());  //puts a millis time stamp on each string.
	//make a big string containing above strings
	String Baro_data = String(
			absalt + comma + temp + comma + realpressure + comma + accelXraw
					+ comma + timer);

	Serial.println(Baro_data);
	delay(500);
	File dataFile = SD.open("datalog.txt", FILE_WRITE);
	// if the file is available, write to it:
	if (dataFile) {
		dataFile.println(Baro_data); //put Baro_data on the SD card
		dataFile.close();

		if (millis() > 10000) //change this number to change alarm delay (1s = 1000ms)
				{
			tone(8, 1000); // change the second number to alter the tone of the peizo alarm
		} else {
			noTone(8);
		}
	}
}