/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyParser.h"
#include "MyParserSerial.h"
#include "MyTransport.h"

MyParserSerial serialMsgParser;

MyParserSerial::MyParserSerial() : MyParser() {}

bool MyParserSerial::parse(MyMessage &message, char *inputString) {
	char *str, *p, *value=NULL;
	uint8_t bvalue[MAX_PAYLOAD];
	uint8_t blen = 0;
	int i = 0;
	uint8_t command = 0;
	uint8_t ack = 0;
	String sNodeid = "";
	int nPos = 0;
	message.setSender( GATEWAY_ADDRESS );
	message.setLast( GATEWAY_ADDRESS );
	message.setSensor(0);

	// Extract command data coming on serial line
	/// Contains subID?
	bool _hasSubID = false;
	/*str = strtok_r(inputString, "-", &p);
	if( str ) {
		_hasSubID = true;
		message.setDestination((uint8_t)atoi(str));
		str = strtok_r(NULL, "-", &p);
		if( str ) {
			message.setSensor((uint8_t)atoi(str));
		}
	}*/
	for (str = strtok_r(inputString, ";", &p); // split using semicolon
		str && i < 6; // loop while str is not null an max 5 times
		str = strtok_r(NULL, ";", &p) // get subsequent tokens
			) {
		switch (i) {
			case 0: // Radioid (destination)
			  sNodeid = str;
				nPos = sNodeid.indexOf('-');
				if (nPos > 0) {
					// contain subID
					_hasSubID = true;
					/*SERIAL("nodeid ");
					Serial.println((uint8_t)(sNodeid.substring(0, nPos).toInt()));
					SERIAL("subid ");
					Serial.println((uint8_t)(sNodeid.substring(nPos + 1).toInt()));*/
					message.setDestination((uint8_t)(sNodeid.substring(0, nPos).toInt()));
					message.setSensor((uint8_t)(sNodeid.substring(nPos + 1).toInt()));
				}
				if( !_hasSubID )
				{
					message.setDestination((uint8_t)atoi(str));
				}
				break;
			case 1: // Sender
				message.setSender((uint8_t)atoi(str));
				break;
			case 2: // Command (message type)
				command = atoi(str);
				mSetCommand(message.msg, command);
				break;
			case 3: // Should we request ack from destination?
				ack = atoi(str);
				break;
			case 4: // Sub-type
				message.setType((uint8_t)atoi(str));
				break;
			case 5: // Variable value
				if (command == C_STREAM) {
					blen = 0;
					uint8_t val;
					while (*str) {
						val = h2i(*str++) << 4;
						val += h2i(*str++);
						bvalue[blen] = val;
						blen++;
					}
				} else {
					value = str;
					// Remove ending carriage return character (if it exists)
					uint8_t lastCharacter = strlen(value)-1;
					if (value[lastCharacter] == '\r' || value[lastCharacter] == '\n')
						value[lastCharacter] = 0;
				}
				break;
		}
		i++;
	}
	// Check for invalid input
	if (i < 5)
		return false;

	if( ack == 2 ) {
		mSetAck(message.msg, true);
		mSetRequestAck(message.msg, false);
	} else {
		mSetAck(message.msg, false);
  	mSetRequestAck(message.msg, ack?1:0);
	}
	if (command == C_STREAM)
		message.set(bvalue, blen);
	else
		message.set(value);
	return true;
}

// Sun added 2016-05-18
char* MyParserSerial::getSerialString(MyMessage &message, char *buffer) const {
	if (buffer != NULL) {
		char payl[MAX_PAYLOAD*2+1];

		if( message.getSender() > 0 ) {
			sprintf(buffer, "%d-%d;%d;%d;%d;%d;%s\n",
				message.getDestination(), message.getSensor(), message.getSender(),
				mGetCommand(message.msg), (mGetAck(message.msg) ? 2 : mGetRequestAck(message.msg)),
				message.getType(),	message.getString(payl));
		} else {
			sprintf(buffer, "%d;%d;%d;%d;%d;%s\n",
			  message.getDestination(), message.getSender(),
				mGetCommand(message.msg), (mGetAck(message.msg) ? 2 : mGetRequestAck(message.msg)),
				message.getType(),	message.getString(payl));
		}
		return buffer;
	}

	return NULL;
}
