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

#include "MyTransportNRF24.h"

MyTransportNRF24::MyTransportNRF24(uint8_t ce, uint8_t cs, uint8_t channel, uint8_t paLevel, uint8_t dataRate)
	:
	MyTransport(),
	rf24(ce, cs),
	_channel(channel),
	_paLevel(paLevel),
	_dataRate(dataRate)
{
	// SBS added 2016-06-28
	_myNetworkID = 0;
	_currentNetworkID = 0;
	_bValid = false;
	enableBaseNetwork();
}

bool MyTransportNRF24::init() {
	// Start up the radio library
	rf24.begin();

	if (!rf24.isPVariant()) {
		_bValid = false;
		return false;
	}

	rf24.setAutoAck(1);
	//rf24.setAutoAck(BROADCAST_PIPE,false); // Turn off auto ack for broadcast

	rf24.enableAckPayload();
	rf24.setChannel(_channel);
	rf24.setPALevel(_paLevel);
	rf24.setDataRate((rf24_datarate_e)_dataRate);
	rf24.setRetries(5,15);
	rf24.setCRCLength(RF24_CRC_16);
	rf24.enableDynamicPayloads(false);

	// All nodes listen to broadcast pipe (for FIND_PARENT_RESPONSE messages)
	for( uint8_t i=0; i<6; i++) {
		rf24.closeReadingPipe(i);
	}
	rf24.openReadingPipe(BROADCAST_PIPE, TO_ADDR(RF24_BASE_RADIO_ID, BROADCAST_ADDRESS));
	_bValid = true;
	return true;
}

bool MyTransportNRF24::init(uint8_t channel, uint8_t paLevel, uint8_t dataRate) {
	_channel = channel;
	_paLevel = paLevel;
	_dataRate = dataRate;
	return init();
}

void MyTransportNRF24::setAddress(uint8_t address, uint64_t network) {
	// SBS added 2016-06-28
	if( _address == address && _currentNetworkID == network )
		return;
	if( _currentNetworkID > 0 )
		rf24.stopListening();
	if( RF24_BASE_RADIO_ID != network )
		_myNetworkID = network;

	_currentNetworkID = network;
	_address = address;
	// SBS added 2016-07-21
	if( _address == GATEWAY_ADDRESS ) {
		rf24.openReadingPipe(WRITE_PIPE, TO_ADDR(RF24_BASE_RADIO_ID, BASESERVICE_ADDRESS));
		if( CURRENT_NODE_PIPE != WRITE_PIPE ) {
			rf24.openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(RF24_BASE_RADIO_ID, address));
		}
		rf24.openReadingPipe(BROADCAST_PIPE, TO_ADDR(network, BROADCAST_ADDRESS));
		if( _currentNetworkID != RF24_BASE_RADIO_ID ) {
			rf24.openReadingPipe(PRIVATE_NET_PIPE, TO_ADDR(network, address));
		}
	} else {
		rf24.openReadingPipe(WRITE_PIPE, TO_ADDR(network, address));
		if( CURRENT_NODE_PIPE != WRITE_PIPE ) {
			rf24.openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(network, address));
		}
		rf24.openReadingPipe(BROADCAST_PIPE, TO_ADDR(network, BROADCAST_ADDRESS));
	}
	rf24.startListening();
}

uint8_t MyTransportNRF24::getAddress() {
	return _address;
}

// SBS added 2016-06-28
uint64_t MyTransportNRF24::getCurrentNetworkID() const {
	return _currentNetworkID;
}

// SBS added 2016-06-28
uint64_t MyTransportNRF24::getMyNetworkID() const {
	return _myNetworkID;
}

// SBS added 2016-06-28
bool MyTransportNRF24::switch2BaseNetwork() {
	setAddress(_address, RF24_BASE_RADIO_ID);
	enableBaseNetwork();
	return true;
}

// SBS added 2016-06-28
bool MyTransportNRF24::switch2MyNetwork() {
	if( _myNetworkID == 0 )
		return false;

	setAddress(_address, _myNetworkID);
	return true;
}

// SBS added 2016-06-28
bool MyTransportNRF24::isValid() {
	return(_bValid & rf24.isValid());
}

bool MyTransportNRF24::CheckConfig()
{
	if( rf24.getChannel() != _channel ) return false;
	if( rf24.getDataRate() != _dataRate ) return false;
	if( rf24.getPALevel() != _paLevel ) return false;
	if( rf24.getCRCLength() != RF24_CRC_16 ) return false;
	return true;
}

// SBS added 2016-07-04
void MyTransportNRF24::PrintRFDetails() {
	rf24.printDetails();
}

// SBS added 2016-07-22
void MyTransportNRF24::enableBaseNetwork(bool sw) {
	if(sw) { _baseStartTick = millis(); }
	_bBaseNetworkEnabled = sw;
}

uint16_t MyTransportNRF24::getBaseNetworkDuration() {
	if( getCurrentNetworkID() == RF24_BASE_RADIO_ID || !isBaseNetworkEnabled() ) return 0;
	if( millis() <= _baseStartTick ) return 0;
	return (millis() - _baseStartTick) / 1000;
}

bool MyTransportNRF24::send(uint8_t to, const void* data, uint8_t len, uint8_t pipe) {
	// Make sure radio has powered up
	rf24.powerUp();
	rf24.stopListening();
	if( _address == GATEWAY_ADDRESS && pipe == CURRENT_NODE_PIPE ) {
		if( !_bBaseNetworkEnabled && to != NODEID_RF_SCANNER) {
			rf24.startListening();
			return false;
		}
		rf24.openWritingPipe(TO_ADDR(RF24_BASE_RADIO_ID, to));
	} else {
		rf24.openWritingPipe(TO_ADDR(_currentNetworkID, to));
	}
	bool ok = rf24.write(data, len, to == BROADCAST_ADDRESS);
	rf24.startListening();
	return ok;
}

bool MyTransportNRF24::send(uint8_t to, MyMessage &message, uint8_t pipe) {
	message.setVersion(PROTOCOL_VERSION);
	message.setLast(_address);
	uint8_t length = message.getSigned() ? MAX_MESSAGE_LENGTH : message.getLength();
	return send(to, (void *)&(message.msg), min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length), pipe);
}

bool MyTransportNRF24::available(uint8_t *to, uint8_t *pipe) {
	uint8_t lv_pipe = 255;
	boolean avail = rf24.available(&lv_pipe);
	//(void)avail; //until somebody makes use of 'avail'
	// SBS added 2016-07-21
	if( pipe ) *pipe = lv_pipe;
	//if(lv_pipe != 255)
	//  Serial.printf("avai data pipe = %d\r\n",lv_pipe);
	if (lv_pipe == CURRENT_NODE_PIPE)
	{
		*to = BASESERVICE_ADDRESS;
		//if( _address == GATEWAY_ADDRESS && !_bBaseNetworkEnabled ) {
			// Discard message due to disabled BaseNetwork
			//UC lv_pData[MAX_MESSAGE_LENGTH];
			//receive(lv_pData);
			//if(lv_pData[1] == NODEID_RF_SCANNER) return true;
			//return false;
		//}
	}
	else if(lv_pipe == PRIVATE_NET_PIPE)
		*to = _address;
	else if (lv_pipe == BROADCAST_PIPE)
		*to = BROADCAST_ADDRESS;

	return (rf24.available() && lv_pipe < 6);
}

uint8_t MyTransportNRF24::receive(void* data) {
	uint8_t len = rf24.getDynamicPayloadSize();
	rf24.read(data, len);
	return len;
}

void MyTransportNRF24::powerDown() {
	rf24.powerDown();
}

uint8_t MyTransportNRF24::getChannel(bool read)
{
	if( read ) {
		_channel = rf24.getChannel();
	}
	return _channel;
}

void MyTransportNRF24::setChannel(uint8_t channel)
{
	if( _channel != channel )	{
		_channel = channel;
		rf24.setChannel(channel);
	}
}

uint8_t MyTransportNRF24::getPALevel(bool read)
{
	if( read ) {
		_paLevel = rf24.getPALevel();
	}
	return _paLevel;
}

void MyTransportNRF24::setPALevel(uint8_t level)
{
	if( _paLevel != level )	{
		_paLevel = level;
		rf24.setPALevel(level);
	}
}

uint8_t MyTransportNRF24::getDataRate(bool read)
{
	if( read ) {
		_dataRate = (uint8_t)rf24.getDataRate();
	}
	return _dataRate;
}

bool MyTransportNRF24::setDataRate(uint8_t speed)
{
	if( _dataRate != speed )	{
		_dataRate = speed;
		return rf24.setDataRate((rf24_datarate_e)speed);
	}
	return true;
}
