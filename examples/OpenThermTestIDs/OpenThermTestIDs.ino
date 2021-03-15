
/*
OpenTherm Test IDs supported by Slave
By: miks69
Date: January 31, 2021

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

const int inPin = 4; 	// for ESP8266
const int outPin = 5; 	// for ESP8266

OpenTherm ot(inPin, outPin);

void ICACHE_RAM_ATTR handleInterrupt() {
    ot.handleInterrupt();
}

void setup() {
	
    Serial.begin(115200);
    Serial.println("Start");

    ot.begin(handleInterrupt);
}

bool checkResult(unsigned long response) {
	OpenThermResponseStatus responseStatus = ot.getLastResponseStatus();
	if (responseStatus == OpenThermResponseStatus::SUCCESS) {
		return true;
	}
	if (responseStatus == OpenThermResponseStatus::NONE) {
        Serial.println("Error: OpenTherm is not initialized");
    }
    else if (responseStatus == OpenThermResponseStatus::INVALID) {
        Serial.println("Error: Invalid response " + String(response, HEX));
    }
    else if (responseStatus == OpenThermResponseStatus::TIMEOUT) {
        Serial.println("Error: Response timeout");
    }
	return false;
}

void loop() {
	
    bool enableCentralHeating = false;
    bool enableHotWater = false;
    bool enableCooling = false;
	unsigned long request;
	unsigned long response;
	OpenThermMessageID msgID;
	byte ID;
	//
	// Control and Status Information
    //
	ID = 0;	// Set/Get Boiler Status
	Serial.println("ID="+String(ID)+": OpenTherm status request");
	response = ot.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling);
	if (checkResult(response)) {
		Serial.println("Fault status: " + String((response & 0x01) ? "on" : "off"));
		Serial.println("CH mode: " + String((response & 0x02) ? "on" : "off"));
        Serial.println("DHW mode: " + String((response & 0x04) ? "on" : "off"));
        Serial.println("Flame status: " + String((response & 0x08) ? "on" : "off"));
		Serial.println("Cooling status: " + String((response & 0x10) ? "on" : "off"));
		Serial.println("CH2 mode: " + String((response & 0x20) ? "on" : "off"));
		Serial.println("Diagnostic event: " + String((response & 0x40) ? "on" : "off"));
	}
	//
	ID = 5;	// Fault flags & code
	Serial.println("ID="+String(ID)+": Fault flags & code");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::ASFflags, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("OEM-specific fault/error code (HEX): " + String(response & 0xFF, HEX));
		Serial.println("Application-specific fault flags:");
		response = response >> 8; 	// Data High Byte (HB)
		Serial.println("Service request: " + String((response & 0x01) ? "on" : "off"));
		Serial.println("Lockout-reset: " + String((response & 0x02) ? "on" : "off"));
        Serial.println("Low water press: " + String((response & 0x04) ? "on" : "off"));
        Serial.println("Gas/flame fault: " + String((response & 0x08) ? "on" : "off"));
		Serial.println("Air press fault: " + String((response & 0x10) ? "on" : "off"));
		Serial.println("Water over-temp: " + String((response & 0x20) ? "on" : "off"));
	}
	//
	ID = 115;	// OEM diagnostic code
	Serial.println("ID="+String(ID)+": OEM diagnostic code");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::OEMDiagnosticCode, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data(HEX): " + String(response & 0xFFFF, HEX));
	}
	//
	// Configuration Information
	//
	ID = 3;		// Slave configuration & MemberID code
	Serial.println("ID="+String(ID)+": Slave configuration & MemberID code");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::SConfigSMemberIDcode, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("MemberID code (HEX): " + String(response & 0xFF, HEX));
		response = response >> 8;	// Data High Byte (HB)
		Serial.println("DHW present: " + String((response & 0x01) ? "on" : "off"));
		Serial.println("Control type modulating: " + String((response & 0x02) ? "on" : "off"));
        Serial.println("Cooling supported: " + String((response & 0x04) ? "on" : "off"));
        Serial.println("DHW config: " + String((response & 0x08) ? "on" : "off"));
		Serial.println("Master low-off&pump control function allowed: " + String((response & 0x10) ? "on" : "off"));
		Serial.println("CH2 present: " + String((response & 0x20) ? "on" : "off"));
	}
	//
	ID = 125;	// OpenTherm protocol version
	Serial.println("ID="+String(ID)+": OpenTherm protocol version");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::OpenThermVersionSlave, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF)));
	}
	//
	ID = 127;	// Slave product version
	Serial.println("ID="+String(ID)+": Slave product version");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::SlaveVersion, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Product type: " + String((response >> 8) & 0xFF));
		Serial.println("Product version: " + String(response & 0xFF));
	}
	//
	// Sensor and Informational Data
	//
	ID = 17;	// Relative Modulation Level (f8.8)
	Serial.println("ID="+String(ID)+": Relative Modulation Level, %");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RelModLevel, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF)));
	}
	//
	ID = 18;	// CH water pressure (f8.8)
	Serial.println("ID="+String(ID)+": CH water pressure, bar");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPressure, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF)));
	}
	ID = 19;	// DHW flow rate (f8.8)
	Serial.println("ID="+String(ID)+": DHW flow rate, L/min");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWFlowRate, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF)));
	}
	//
	ID = 20;	// Day of Week & Time of Day
	Serial.println("ID="+String(ID)+": Day of Week & Time of Day");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DayTime, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Day of week: " + String((response >> 8) & 0xE0));
		Serial.println("Hours: " + String((response >> 8) & 0x1F));
		Serial.println("Minutes: " + String((response) & 0xFF));
	}
	ID = 21;	// Date
	Serial.println("ID="+String(ID)+": Date");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Date, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Month: " + String((response >> 8) & 0xFF));
		Serial.println("Day of month: " + String((response) & 0xFF));
	}
	//
	ID = 22;	// Year
	Serial.println("ID="+String(ID)+": Year");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Year, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Year: " + String(response & 0xFFFF));
	}
	//
	ID = 25;	// Boiler water temp (f8.8)
	Serial.println("ID="+String(ID)+": Boiler water temp (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tboiler, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 26;	// DHW temperature (f8.8)
	Serial.println("ID="+String(ID)+": DHW temperature (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 27;	// Outside temperature (f8.8)
	Serial.println("ID="+String(ID)+": Outside air temperature (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Toutside, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 28;	// Return water temperature (f8.8)
	Serial.println("ID="+String(ID)+": Return water temperature (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tret, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 29;	// Solar storage temperature (f8.8)
	Serial.println("ID="+String(ID)+": Solar storage temperature (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tstorage, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 30;	// Solar collector temperature (s16)
	Serial.println("ID="+String(ID)+": Solar collector temperature (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tcollector, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 31;	// Flow water temperature CH2 (f8.8)
	Serial.println("ID="+String(ID)+": Flow water temperature CH2 circuit (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TflowCH2, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 32;	// DHW2 temperature (f8.8)
	Serial.println("ID="+String(ID)+": Domestic hot water temperature 2 (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw2, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 33;	// Boiler exhaust temperature (s16)
	Serial.println("ID="+String(ID)+": Boiler exhaust temperature (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Texhaust, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 116;	// Burner starts (u16)
	Serial.println("ID="+String(ID)+": Number of starts burner");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::BurnerStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 117;	// CH pump starts (u16)
	Serial.println("ID="+String(ID)+": Number of CH pump starts");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPumpStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 118;	// DHW pump/valve starts (u16)
	Serial.println("ID="+String(ID)+": Number of starts DHW pump/valve");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWPumpValveStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 119;	// DHW burner starts (u16)
	Serial.println("ID="+String(ID)+": Number of starts burner during DHW mode");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWBurnerStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 120;	// Burner operation hours (u16)
	Serial.println("ID="+String(ID)+": Number of hours that burner is in operation (i.e. flame on)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::BurnerOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 121;	// CH pump operation hours (u16)
	Serial.println("ID="+String(ID)+": Number of hours that CH pump has been running");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPumpOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 122;	// DHW pump/valve operation hours (u16)
	Serial.println("ID="+String(ID)+": Number of hours that DHW pump has been running or DHW valve has been opened");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWPumpValveOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	ID = 123;	// DHW burner operation hours (u16)
	Serial.println("ID="+String(ID)+": Number of hours that burner is in operation during DHW mode");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWBurnerOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String(response & 0xFFFF));
	}
	//
	// Pre-Defined Remote Boiler Parameters
	//
	ID = 6;	// Remote boiler parameter transfer-enable & read/write flags
	Serial.println("ID="+String(ID)+": Remote boiler parameter transfer-enable & read/write flags");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RBPflags, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		// Data Low Byte (LB) - Remote-parameter read/write flags
		Serial.println("DHW setpoint read/write: " + String((response & 0x01) ? "on" : "off"));
		Serial.println("max CHsetpoint read/write: " + String((response & 0x02) ? "on" : "off"));
		response = response >> 8;
		// Data High Byte (HB) - Remote-parameter transfer-enable flags
		Serial.println("DHW setpoint transfer-enable: " + String((response & 0x01) ? "on" : "off"));
		Serial.println("max CHsetpoint transfer-enable: " + String((response & 0x02) ? "on" : "off"));
	}
	//
	ID = 48;	// DHW setpoint upper & lower bounds for adjustment (s8/s8)
	Serial.println("ID="+String(ID)+": DHW setpoint upper & lower bounds for adjustment (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TdhwSetUBTdhwSetLB, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("DHWsetp low-bound: " + String(response & 0xFF));
		Serial.println("DHWsetp upp-bound: " + String((response >> 8) & 0xFF));
	}
	//
	ID = 49;	// Max CH water setpoint upper & lower bounds for adjustment (s8/s8)
	Serial.println("ID="+String(ID)+": Max CH water setpoint upper & lower bounds for adjustment (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxTSetUBMaxTSetLB, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("max CHsetp low-bound: " + String(response & 0xFF));
		Serial.println("max CHsetp upp-bound: " + String((response >> 8) & 0xFF));
	}
	//
	ID = 50;	// OTC heat curve ratio upper & lower bounds for adjustment (s8/s8)
	Serial.println("ID="+String(ID)+": OTC heat curve ratio upper & lower bounds for adjustment  (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxTSetUBMaxTSetLB, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("OTC heat curve ratio low-bound: " + String(response & 0xFF));
		Serial.println("OTC heat curve ratio upp-bound: " + String((response >> 8) & 0xFF));
	}
	//
	ID = 56;	// DHW setpoint (f8.8)
	Serial.println("ID="+String(ID)+": Domestic hot water temperature setpoint (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TdhwSet, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 57;	// max CH water setpoint (f8.8)
	Serial.println("ID="+String(ID)+": Maximum allowable CH water setpoint (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxTSet, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 58;	// OTC heat curve ratio (f8.8)
	Serial.println("ID="+String(ID)+": OTC heat curve ratio (°C)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Hcratio, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	// Transparent Slave Parameters
	//
	ID = 10;	// Number of Transparent-Slave-Parameters supported by slave
	Serial.println("ID="+String(ID)+": Number of Transparent-Slave-Parameters supported by slave");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TSP, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF));
	}
	//
	// Fault History Data
	//
	ID = 12;	// Size of Fault-History-Buffer supported by slave
	Serial.println("ID="+String(ID)+": Size of Fault-History-Buffer supported by slave");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::FHBsize, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF));
	}
	//
	// Boiler Sequencer Control
	//
	ID = 15;	// Maximum boiler capacity (kW) / Minimum boiler modulation level(%)
	Serial.println("ID="+String(ID)+": Maximum boiler capacity (kW) / Minimum boiler modulation level(%)");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxCapacityMinModLevel, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("max boiler capacity: " + String((response >> 8) & 0xFF));
		Serial.println("min modulation level: " + String((response) & 0xFF));
	}
	//
	// Remote override room setpoint
	//
	ID = 9;	// Remote override room setpoint (f8.8)
	Serial.println("ID="+String(ID)+": Remote override room setpoint");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TrOverride, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		Serial.println("Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF));
	}
	//
	ID = 100;	// Function of manual and program changes in master and remote room setpoint
	Serial.println("ID="+String(ID)+": Remote override function");
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RemoteOverrideFunction, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response)) {
		// Data Low Byte (LB) - Remote-parameter read/write flags
		Serial.println("Manual change priority: " + String((response & 0x01) ? "on" : "off"));
		Serial.println("Program change priority: " + String((response & 0x02) ? "on" : "off"));
	}
	
    Serial.println("Finished");
    while (1) {}
	
}
