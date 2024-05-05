/*

AC Switch with 3 WiFi controllable switches

Inspired from:

AC Voltage dimmer with Zero cross detection
Author: Charith Fernanado http://www.inmojo.com
License: Creative Commons Attribution Share-Alike 3.0 License.

*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EepromUtil.h>
#include <ESPConfig.h>
#include <ESP8266Controller.h>
#include "ACSwitch.h"

ESPConfig configuration(/*controller name*/	"AC 3 SWITCH", /*location*/ "Unknown", /*firmware version*/ "ac3s.200919.bin", /*router SSID*/ "onion", /*router SSID key*/ "242374666");
WiFiUDP Udp;

byte packetBuffer[255 * 3];
byte replyBuffer[255 * 3];
short replyBufferSize = 0;

// initialize dimmer, switch objects
ACSwitch sswitch1("SWITCH ONE", 0/*AC load pin*/, 2/*No. of capabilities*/, configuration.sizeOfEEPROM());
ACSwitch sswitch2("SWITCH TWO", 2/*AC load pin*/, 2/*No. of capabilities*/, configuration.sizeOfEEPROM() + sswitch1.sizeOfEEPROM());
ACSwitch sswitch3("SWITCH THREE", 3/*AC load pin*/, 2/*No. of capabilities*/, configuration.sizeOfEEPROM() + sswitch1.sizeOfEEPROM() + sswitch2.sizeOfEEPROM());

void setup() {

	delay(1000);
	Serial.begin(115200);//,SERIAL_8N1,SERIAL_TX_ONLY);

	DEBUG_PRINTLN("Starting AC switch...");

	configuration.init(sswitch1.pin);

	// load controller capabilities (values) from EEPROM
	sswitch1.loadCapabilities();
	sswitch2.loadCapabilities();
	sswitch3.loadCapabilities();

	Udp.begin(port);
}

unsigned long last = 0;
int packetSize = 0;
_udp_packet udpPacket;
int heap_print_interval = 10000;

void loop() {

	/*if (millis() - last > heap_print_interval) {
		DEBUG_PRINTLN();

		last = millis();
		DEBUG_PRINT("[MAIN] Free heap: ");
		Serial.println(ESP.getFreeHeap(), DEC);

	}*/

	int packetSize = Udp.parsePacket();

	if (packetSize) {
		IPAddress remoteIp = Udp.remoteIP();

		DEBUG_PRINT("Received packet of size "); DEBUG_PRINT(packetSize); DEBUG_PRINT(" from "); DEBUG_PRINT(remoteIp); DEBUG_PRINT(", port ");
		DEBUG_PRINTLN(Udp.remotePort());

		// read the packet into packetBuffer
		//int packetLength = Udp.read(packetBuffer, packetSize);
		Udp.read(packetBuffer, packetSize);
		//if (packetLength > 0) {
		packetBuffer[packetSize] = 0;
		//}

		// initialize the replyBuffer
		memcpy(replyBuffer, packetBuffer, 3);

		// prepare the UDP header from buffer
		//short _size = packetBuffer[1] << 8 | packetBuffer[0];

		udpPacket._size = packetBuffer[1] << 8 | packetBuffer[0];
		//udpPacket._size = _size;
		udpPacket._command = packetBuffer[2];
		udpPacket._payload = (char*)packetBuffer + 3;

		if (udpPacket._command == DEVICE_COMMAND_DISCOVER) {

			//replyBufferSize = 3 + configuration.discover(replyBuffer+3);
      replyBufferSize += configuration.toByteArray(replyBuffer+replyBufferSize);

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONFIGURATION) {

			replyBufferSize = 3 + configuration.set(replyBuffer+3, (byte*)udpPacket._payload);

		} else if (udpPacket._command == DEVICE_COMMAND_GET_CONTROLLER) {

			byte _pin = udpPacket._payload[0];
			if (_pin == sswitch1.pin) {

				//memcpy(replyBuffer + 3, sswitch1.toByteArray(), sswitch1.sizeOfUDPPayload());
				//replyBufferSize = 3 + sswitch1.sizeOfUDPPayload();
        replyBufferSize += sswitch1.toByteArray(replyBuffer + replyBufferSize);

			} else if (_pin == sswitch2.pin) {

				//memcpy(replyBuffer + 3, sswitch2.toByteArray(), sswitch2.sizeOfUDPPayload());
				//replyBufferSize = 3 + sswitch2.sizeOfUDPPayload();
        replyBufferSize += sswitch2.toByteArray(replyBuffer + replyBufferSize);

			} else if (_pin == sswitch3.pin) {

				//memcpy(replyBuffer + 3, sswitch3.toByteArray(), sswitch3.sizeOfUDPPayload());
				//replyBufferSize = 3 + sswitch3.sizeOfUDPPayload();
        replyBufferSize += sswitch3.toByteArray(replyBuffer + replyBufferSize);

			}

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONTROLLER) {

			byte _pin = udpPacket._payload[0];

			// (OVERRIDE) send 3 bytes (size, command) as reply to client
			replyBufferSize = 3;

			if (_pin == sswitch1.pin) {

				sswitch1.fromByteArray((byte*)udpPacket._payload);

			} else if (_pin == sswitch2.pin) {

				sswitch2.fromByteArray((byte*)udpPacket._payload);

			} else if (_pin == sswitch3.pin) {

				sswitch3.fromByteArray((byte*)udpPacket._payload);

			}

		} else if (udpPacket._command == DEVICE_COMMAND_GETALL_CONTROLLER) {

			//memcpy(replyBuffer + 3, sswitch1.toByteArray(), sswitch1.sizeOfUDPPayload());
			//memcpy(replyBuffer + 3 + sswitch1.sizeOfUDPPayload(), sswitch2.toByteArray(), sswitch2.sizeOfUDPPayload());
			//memcpy(replyBuffer + 3 + sswitch1.sizeOfUDPPayload() + sswitch3.sizeOfUDPPayload(), sswitch3.toByteArray(), sswitch3.sizeOfUDPPayload());

			//replyBufferSize = 3 + sswitch1.sizeOfUDPPayload() + sswitch2.sizeOfUDPPayload() + sswitch3.sizeOfUDPPayload();
      replyBufferSize += sswitch1.toByteArray(replyBuffer + replyBufferSize);
      replyBufferSize += sswitch2.toByteArray(replyBuffer + replyBufferSize);
      replyBufferSize += sswitch3.toByteArray(replyBuffer + replyBufferSize);

		} else if (udpPacket._command == DEVICE_COMMAND_SETALL_CONTROLLER) {

      int i = 0;

			// update the LED variables with new values
      for (int count = 0; count < 3; count++) {

        if (udpPacket._payload[i] == sswitch1.pin) {

          sswitch1.fromByteArray((byte*)udpPacket._payload + i);

          i = i + sswitch1.sizeOfEEPROM();

        } else if (udpPacket._payload[i] == sswitch2.pin) {

          sswitch2.fromByteArray((byte*)udpPacket._payload + i);

          i = i + sswitch2.sizeOfEEPROM();

        } else if (udpPacket._payload[i] == sswitch3.pin) {

          sswitch3.fromByteArray((byte*)udpPacket._payload + i);

          i = i + sswitch3.sizeOfEEPROM();

        }
      }

			// (OVERRIDE) send 3 bytes (size, command) as reply to client
			replyBufferSize = 3;
		}

		// update the size of replyBuffer in packet bytes
		replyBuffer[0] = lowByte(replyBufferSize);
		replyBuffer[1] = highByte(replyBufferSize);

		//DEBUG_PRINT("replyBuffer ");printArray(replyBuffer, replyBufferSize, true);DEBUG_PRINTLN();
		//DEBUG_PRINT("packetBuffer ");printArray(packetBuffer, packetSize, true);DEBUG_PRINTLN();

		// send a reply, to the IP address and port that sent us the packet we received
		Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
		Udp.write(replyBuffer, replyBufferSize);
		Udp.endPacket();
	}

	sswitch1.loop();
	sswitch2.loop();
	sswitch3.loop();

	yield();
}
