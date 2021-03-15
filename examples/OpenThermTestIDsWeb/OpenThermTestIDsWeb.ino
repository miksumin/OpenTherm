
/*
OpenTherm Test IDs Web version
By: miks69
Date: February 1, 2021

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
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
	#define STASSID	"your-ssid"
	#define STAPASS "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPASS;

ESP8266WebServer server(80);

#include <OpenTherm.h>

const int inPin = 4; 	// for ESP8266
const int outPin = 5; 	// for ESP8266

OpenTherm ot(inPin, outPin);

void ICACHE_RAM_ATTR handleInterrupt() {
    ot.handleInterrupt();
}

void handleRoot() {
  unsigned long time = millis();
  server.send(200, "text/plain", ot_test_ids());
  Serial.println("Page rendering time: "+String(millis()-time)+"msec");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Started");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  
  ot.begin(handleInterrupt);
  Serial.println("OpenTherm library initialized");
  
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}

bool checkResult(unsigned long response, char* result) {
	OpenThermResponseStatus responseStatus = ot.getLastResponseStatus();
	if (responseStatus == OpenThermResponseStatus::SUCCESS) {
		return true;
	}
	if (responseStatus == OpenThermResponseStatus::NONE) {
        strcpy(result, "Error: OpenTherm is not initialized<br>");
    }
    else if (responseStatus == OpenThermResponseStatus::INVALID) {
        //char chRsponse[10];
		//sprintf(chRsponse, "0x08X", response);
		sprintf(result, "Error: Invalid response 0x%08X<br>", response);
		//strcpy(result, "Error: Invalid response " + String(response, HEX)+"<br>";
    }
    else if (responseStatus == OpenThermResponseStatus::TIMEOUT) {
        strcpy(result, "Error: Response timeout<br>");
    }
	return false;
}

String ot_test_ids() {
	
    bool enableCentralHeating = false;
    bool enableHotWater = false;
    bool enableCooling = false;
	unsigned long request;
	unsigned long response;
	//OpenThermMessageID msgID;
	byte ID;
	String result;
	char result1[40];
	//
	// Control and Status Information
    //
	ID = 0;	// Set/Get Boiler Status
	result += "ID="+String(ID)+": OpenTherm status request<br>";
	response = ot.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling);
	if (checkResult(response, result1)) {
		result += "Fault status: " + String((response & 0x01) ? "on" : "off")+"<br>";
		result += "CH mode: " + String((response & 0x02) ? "on" : "off")+"<br>";
        result += "DHW mode: " + String((response & 0x04) ? "on" : "off")+"<br>";
        result += "Flame status: " + String((response & 0x08) ? "on" : "off")+"<br>";
		result += "Cooling status: " + String((response & 0x10) ? "on" : "off")+"<br>";
		result += "CH2 mode: " + String((response & 0x20) ? "on" : "off")+"<br>";
		result += "Diagnostic event: " + String((response & 0x40) ? "on" : "off")+"<br>";
	}
	else result += String(result1);
	//
	ID = 5;	// Fault flags & code
	result += "ID="+String(ID)+": Fault flags & code"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::ASFflags, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "OEM-specific fault/error code (HEX): " + String(response & 0xFF, HEX)+"<br>";
		result += "Application-specific fault flags:<br>";
		response = response >> 8; 	// Data High Byte (HB)
		result += "Service request: " + String((response & 0x01) ? "on" : "off")+"<br>";
		result += "Lockout-reset: " + String((response & 0x02) ? "on" : "off")+"<br>";
        result += "Low water press: " + String((response & 0x04) ? "on" : "off")+"<br>";
        result += "Gas/flame fault: " + String((response & 0x08) ? "on" : "off")+"<br>";
		result += "Air press fault: " + String((response & 0x10) ? "on" : "off")+"<br>";
		result += "Water over-temp: " + String((response & 0x20) ? "on" : "off")+"<br>";
	}
	else result += String(result1);
	//
	ID = 115;	// OEM diagnostic code
	result += "ID="+String(ID)+": OEM diagnostic code"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::OEMDiagnosticCode, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data(HEX): " + String(response & 0xFFFF, HEX)+"<br>";
	}
	else result += String(result1);
	//
	// Configuration Information
	//
	ID = 3;		// Slave configuration & MemberID code
	result += "ID="+String(ID)+": Slave configuration & MemberID code"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::SConfigSMemberIDcode, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "MemberID code (HEX): " + String(response & 0xFF, HEX)+"<br>";
		response = response >> 8;	// Data High Byte (HB)
		result += "DHW present: " + String((response & 0x01) ? "on" : "off")+"<br>";
		result += "Control type modulating: " + String((response & 0x02) ? "on" : "off")+"<br>";
        result += "Cooling supported: " + String((response & 0x04) ? "on" : "off")+"<br>";
        result += "DHW config: " + String((response & 0x08) ? "on" : "off")+"<br>";
		result += "Master low-off&pump control function allowed: " + String((response & 0x10) ? "on" : "off")+"<br>";
		result += "CH2 present: " + String((response & 0x20) ? "on" : "off")+"<br>";
	}
	else result += String(result1);
	//
	ID = 125;	// OpenTherm protocol version
	result += "ID="+String(ID)+": OpenTherm protocol version"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::OpenThermVersionSlave, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF))+"<br>";
	}
	else result += String(result1);
	//
	ID = 127;	// Slave product version
	result += "ID="+String(ID)+": Slave product version"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::SlaveVersion, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Product type: " + String((response >> 8) & 0xFF)+"<br>";
		result += "Product version: " + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	// Sensor and Informational Data
	//
	ID = 17;	// Relative Modulation Level (f8.8)
	result += "ID="+String(ID)+": Relative Modulation Level, %"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RelModLevel, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF))+"<br>";
	}
	else result += String(result1);
	//
	ID = 18;	// CH water pressure (f8.8)
	result += "ID="+String(ID)+": CH water pressure, bar"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPressure, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF))+"<br>";
	}
	else result += String(result1);
	//
	ID = 19;	// DHW flow rate (f8.8)
	result += "ID="+String(ID)+": DHW flow rate, L/min"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWFlowRate, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String((response & 0xFF))+"<br>";
	}
	else result += String(result1);
	//
	ID = 20;	// Day of Week & Time of Day
	result += "ID="+String(ID)+": Day of Week & Time of Day"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DayTime, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Day of week: " + String((response >> 8) & 0xE0)+"<br>";
		result += "Hours: " + String((response >> 8) & 0x1F)+"<br>";
		result += "Minutes: " + String((response) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 21;	// Date
	result += "ID="+String(ID)+": Date"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Date, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Month: " + String((response >> 8) & 0xFF)+"<br>";
		result += "Day of month: " + String((response) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 22;	// Year
	result += "ID="+String(ID)+": Year"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Year, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Year: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 25;	// Boiler water temp (f8.8)
	result += "ID="+String(ID)+": Boiler water temp (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tboiler, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 26;	// DHW temperature (f8.8)
	result += "ID="+String(ID)+": DHW temperature (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 27;	// Outside temperature (f8.8)
	result += "ID="+String(ID)+": Outside air temperature (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Toutside, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 28;	// Return water temperature (f8.8)
	result += "ID="+String(ID)+": Return water temperature (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tret, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 29;	// Solar storage temperature (f8.8)
	result += "ID="+String(ID)+": Solar storage temperature (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tstorage, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 30;	// Solar collector temperature (s16)
	result += "ID="+String(ID)+": Solar collector temperature (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tcollector, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 31;	// Flow water temperature CH2 (f8.8)
	result += "ID="+String(ID)+": Flow water temperature CH2 circuit (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TflowCH2, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 32;	// DHW2 temperature (f8.8)
	result += "ID="+String(ID)+": Domestic hot water temperature 2 (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw2, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 33;	// Boiler exhaust temperature (s16)
	result += "ID="+String(ID)+": Boiler exhaust temperature (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Texhaust, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 116;	// Burner starts (u16)
	result += "ID="+String(ID)+": Number of starts burner"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::BurnerStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 117;	// CH pump starts (u16)
	result += "ID="+String(ID)+": Number of CH pump starts"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPumpStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 118;	// DHW pump/valve starts (u16)
	result += "ID="+String(ID)+": Number of starts DHW pump/valve"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWPumpValveStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 119;	// DHW burner starts (u16)
	result += "ID="+String(ID)+": Number of starts burner during DHW mode"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWBurnerStarts, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 120;	// Burner operation hours (u16)
	result += "ID="+String(ID)+": Number of hours that burner is in operation (i.e. flame on)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::BurnerOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 121;	// CH pump operation hours (u16)
	result += "ID="+String(ID)+": Number of hours that CH pump has been running"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPumpOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 122;	// DHW pump/valve operation hours (u16)
	result += "ID="+String(ID)+": Number of hours that DHW pump has been running or DHW valve has been opened"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWPumpValveOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 123;	// DHW burner operation hours (u16)
	result += "ID="+String(ID)+": Number of hours that burner is in operation during DHW mode"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::DHWBurnerOperationHours, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String(response & 0xFFFF)+"<br>";
	}
	else result += String(result1);
	//
	// Pre-Defined Remote Boiler Parameters
	//
	ID = 6;	// Remote boiler parameter transfer-enable & read/write flags
	result += "ID="+String(ID)+": Remote boiler parameter transfer-enable & read/write flags"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RBPflags, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		// Data Low Byte (LB) - Remote-parameter read/write flags
		result += "DHW setpoint read/write: " + String((response & 0x01) ? "on" : "off")+"<br>";
		result += "max CHsetpoint read/write: " + String((response & 0x02) ? "on" : "off")+"<br>";
		response = response >> 8;
		// Data High Byte (HB) - Remote-parameter transfer-enable flags
		result += "DHW setpoint transfer-enable: " + String((response & 0x01) ? "on" : "off")+"<br>";
		result += "max CHsetpoint transfer-enable: " + String((response & 0x02) ? "on" : "off")+"<br>";
	}
	else result += String(result1);
	//
	ID = 48;	// DHW setpoint upper & lower bounds for adjustment (s8/s8)
	result += "ID="+String(ID)+": DHW setpoint upper & lower bounds for adjustment (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TdhwSetUBTdhwSetLB, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "DHWsetp low-bound: " + String(response & 0xFF)+"<br>";
		result += "DHWsetp upp-bound: " + String((response >> 8) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 49;	// Max CH water setpoint upper & lower bounds for adjustment (s8/s8)
	result += "ID="+String(ID)+": Max CH water setpoint upper & lower bounds for adjustment (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxTSetUBMaxTSetLB, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "max CHsetp low-bound: " + String(response & 0xFF)+"<br>";
		result += "max CHsetp upp-bound: " + String((response >> 8) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 50;	// OTC heat curve ratio upper & lower bounds for adjustment (s8/s8)
	result += "ID="+String(ID)+": OTC heat curve ratio upper & lower bounds for adjustment  (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxTSetUBMaxTSetLB, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "OTC heat curve ratio low-bound: " + String(response & 0xFF)+"<br>";
		result += "OTC heat curve ratio upp-bound: " + String((response >> 8) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 56;	// DHW setpoint (f8.8)
	result += "ID="+String(ID)+": Domestic hot water temperature setpoint (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TdhwSet, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 57;	// max CH water setpoint (f8.8)
	result += "ID="+String(ID)+": Maximum allowable CH water setpoint (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxTSet, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 58;	// OTC heat curve ratio (f8.8)
	result += "ID="+String(ID)+": OTC heat curve ratio (°C)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Hcratio, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	// Transparent Slave Parameters
	//
	ID = 10;	// Number of Transparent-Slave-Parameters supported by slave
	result += "ID="+String(ID)+": Number of Transparent-Slave-Parameters supported by slave"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TSP, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	// Fault History Data
	//
	ID = 12;	// Size of Fault-History-Buffer supported by slave
	result += "ID="+String(ID)+": Size of Fault-History-Buffer supported by slave"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::FHBsize, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	// Boiler Sequencer Control
	//
	ID = 15;	// Maximum boiler capacity (kW) / Minimum boiler modulation level(%)
	result += "ID="+String(ID)+": Maximum boiler capacity (kW) / Minimum boiler modulation level(%)"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::MaxCapacityMinModLevel, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "max boiler capacity: " + String((response >> 8) & 0xFF)+"<br>";
		result += "min modulation level: " + String((response) & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	// Remote override room setpoint
	//
	ID = 9;	// Remote override room setpoint (f8.8)
	result += "ID="+String(ID)+": Remote override room setpoint"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TrOverride, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		result += "Data: " + String((response >> 8) & 0xFF) + "." + String(response & 0xFF)+"<br>";
	}
	else result += String(result1);
	//
	ID = 100;	// Function of manual and program changes in master and remote room setpoint
	result += "ID="+String(ID)+": Remote override function"+"<br>";
	request = ot.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RemoteOverrideFunction, 0x0000);
	response = ot.sendRequest(request);
	if (checkResult(response, result1)) {
		// Data Low Byte (LB) - Remote-parameter read/write flags
		result += "Manual change priority: " + String((response & 0x01) ? "on" : "off")+"<br>";
		result += "Program change priority: " + String((response & 0x02) ? "on" : "off")+"<br>";
	}
	else result += String(result1);
	
	return result;
	
}

