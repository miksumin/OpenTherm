
/*
OpenTherm Master Communication Example
By: Ihor Melnyk
Date: January 19th, 2018

Uses the OpenTherm library to get/set boiler status and water temperature
Open serial monitor at 115200 baud to see output.

Hardware Connections (OpenTherm Adapter (http://ihormelnyk.com/pages/OpenTherm) to Arduino/ESP8266):
-OT1/OT2 = Boiler X1/X2
-VCC = 5V or 3.3V
-GND = GND
-IN  = Arduino (3) / ESP8266 (5) Output Pin
-OUT = Arduino (2) / ESP8266 (4) Input Pin

Controller(Arduino/ESP8266) input pin should support interrupts.
Arduino digital pins usable for interrupts: Uno, Nano, Mini: 2,3; Mega: 2, 3, 18, 19, 20, 21
ESP8266: Interrupts may be attached to any GPIO pin except GPIO16,
but since GPIO6-GPIO11 are typically used to interface with the flash memory ICs on most esp8266 modules, applying interrupts to these pins are likely to cause problems
*/

#include <Arduino.h>
#include <OpenTherm.h>

const int inPin = 2; //for Arduino, 4 for ESP8266
const int outPin = 3; //for Arduino, 5 for ESP8266

OpenTherm ot(inPin, outPin);

void ICACHE_RAM_ATTR handleInterrupt() {
    ot.handleInterrupt();
}

void responseCallback(unsigned long response, OpenThermResponseStatus responseStatus) {
	
	float temperature;
	
	if (responseStatus == OpenThermResponseStatus::SUCCESS) {
		switch (ot.getDataID(response)) {
			case OpenThermMessageID::Status:
				Serial.println("Central Heating: " + String(ot.isCentralHeatingActive(response) ? "on" : "off"));
				Serial.println("Hot Water: " + String(ot.isHotWaterActive(response) ? "on" : "off"));
				Serial.println("Flame: " + String(ot.isFlameOn(response) ? "on" : "off"));
				break;
			case OpenThermMessageID::Tboiler:
				temperature = ot.getTemperature(response);
				Serial.println("Boiler temperature is " + String(temperature,1) + " degrees C");
				break;
			default: break;
		}
	}
	else {
		if (responseStatus == OpenThermResponseStatus::NONE)
			Serial.println("OpenTherm is not initialized");
		if (responseStatus == OpenThermResponseStatus::TIMEOUT)
			Serial.println("Request timeout");
		if (responseStatus == OpenThermResponseStatus::INVALID)
			Serial.printf("Response invalid : 0x%08X\r\n", response);
	}
	
}

void setup() {
	
    Serial.begin(9600);
    Serial.println("Start");

    ot.begin(handleInterrupt, responseCallback);
	
	Serial.println("OpenTherm initialized");
	
}

void loop() {
	
    bool enableCentralHeating = true;
    bool enableHotWater = true;
    bool enableCooling = false;
	byte boiler_setpoint = 64;
	
	ot.process();
	
	static byte requestCounter = 0;
	unsigned long request;
	
	if (ot.isReady()) {
	
		switch (requestCounter) {
			case 0:
				request = ot.buildSetBoilerStatusRequest(enableCentralHeating, enableHotWater, enableCooling, false, false);
				break;
			case 1:
				request = ot.buildSetBoilerTemperatureRequest((float)boiler_setpoint);
				break;
			case 2:
				request = ot.buildGetBoilerTemperatureRequest();
				break;
			default: 
				requestCounter = 0;
				break;
		}
		
		if (ot.sendRequestAync(request))
			requestCounter++;
		
	}
	
}

