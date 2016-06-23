/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <protocol/cs_MeshMessageTypes.h>
#include <storage/cs_Settings.h>

/** Wrapper around meshing functionality.
 *
 */
class MeshControl : public EventListener {

public:
	//! use static variant of singleton, no dynamic memory allocation
	static MeshControl& getInstance() {
		static MeshControl instance;
		return instance;
	}

	/** Send a message with the scanned devices over the mesh
	 *
	 * @p_list pointer to the list of scanned devices
	 * @size number of scanned devices
	 */
	void sendScanMessage(peripheral_device_t* p_list, uint8_t size);

	void sendPowerSamplesMessage(power_samples_mesh_message_t* samples);

	void sendServiceDataMessage(service_data_mesh_message_t* serviceData);

	/** Send a message into the mesh
	 *
	 * @channel the channel number, see <MeshChannels>
	 * @p_data a pointer to the data which should be sent
	 * @length number of bytes of data to with p_data points
	 */
	ERR_CODE send(uint8_t channel, void* p_data, uint8_t length);

	/**
	 * Get incoming messages and perform certain actions.
	 * @channel the channel number, see <MeshChannels>
	 * @p_data a pointer to the data which was received
	 * @length number of bytes received
	 */
	void process(uint8_t channel, void* p_data, uint16_t length);

protected:

	/** Handle events dispatched through the EventDispatcher
	 *
	 * @evt the event type, can be one of <GeneralEventType>, <ConfigurationTypes>, <StateVarTypes>
	 * @p_data pointer to the data which was sent with the dispatch, can be NULL
	 * @length number of bytes of data provided
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	/** Decode a received mesh data message
	 *
	 * @msg pointer to the message data
	 */
	void decodeDataMessage(uint16_t type, uint8_t* payload);

	/** Check if a message is for us, meaning the current device.
	 * Checks the target address and returns true if targetAddress == myAddress
	 *
	 * @p_data pointer to the mesh message
	 */

#ifdef VERSION_V2
    bool isMessageForUs(void* p_data) {
    	device_mesh_message_t* msg = (device_mesh_message_t*) p_data;

    	uint16_t crownstoneId;
    	Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &crownstoneId);

    	return msg->header.targetCrownstoneId == crownstoneId;
    }
#else
    bool isMessageForUs(void* p_data) {
    	device_mesh_message_t* msg = (device_mesh_message_t*) p_data;

    	if (memcmp(msg->header.targetAddress, _myAddr.addr, BLE_GAP_ADDR_LEN) == 0) {
    		//! target address of package is set to our address
    		return true;
    	} else {
			// _log(INFO, "message not for us, target: ");
			// BLEutil::printArray(msg->header.targetAddress, BLE_GAP_ADDR_LEN);
			return false;
    	}
    }
#endif


	/** Check if a message is a broadcast
	 * Checks the target address and returns true if targetAddress == BROADCAST_ADDRESS
	 *
	 * @p_data pointer to the mesh message
	 */

#ifdef VERSION_V2
    bool isBroadcast(void* p_data) {
    	device_mesh_message_t* msg = (device_mesh_message_t*) p_data;
    	return msg->header.targetCrownstoneId == 0;
    }
#else
    bool isBroadcast(void* p_data) {
    	device_mesh_message_t* msg = (device_mesh_message_t*) p_data;
    	return memcmp(msg->header.targetAddress, new uint8_t[BLE_GAP_ADDR_LEN] BROADCAST_ADDRESS, BLE_GAP_ADDR_LEN) == 0;
    }
#endif


	/** Check if a message is valid
	 * Checks the length parameter based on the type of the mesh message
	 *
	 * @p_data pointer to the mesh message
	 * @length number of bytes received
	 */

	bool isValidMessage(void* p_data, uint16_t length) {
		device_mesh_message_t* msg = (device_mesh_message_t*) p_data;

		switch (msg->header.messageType) {
		case CONTROL_MESSAGE:
		case CONFIG_MESSAGE: {
			//! command and config message have an array for parameters which don't have to be filled
			//! so we don't know in advance how long it needs to be exactly. can only give
			//! a lower and upper bound.
			return (length > getMessageSize(msg->header.messageType) && length <= MAX_MESH_MESSAGE_DATA_LENGTH);
		}
		default:{
			uint16_t desiredLength = getMessageSize(msg->header.messageType);
			bool valid = length == desiredLength;

			if (!valid) {
				LOGd("invalid message, length: %d != %d", length, desiredLength);
			}
			return valid;
		}
		}
	}

	/** Returns the number of bytes required for a mesh message of the type
	 *
	 * @messageType type of mesh message, see <MeshMessageTypes>
	 */
	uint16_t getMessageSize(uint16_t messageType) {
		switch(messageType) {
		case EVENT_MESSAGE:
			return sizeof(device_mesh_header_t) + sizeof(event_mesh_message_t);
		case BEACON_MESSAGE:
			return sizeof(device_mesh_header_t) + sizeof(beacon_mesh_message_t);
		case CONTROL_MESSAGE:
		case CONFIG_MESSAGE:
			return sizeof(device_mesh_header_t) + SB_HEADER_SIZE;
		default:
			return 0;
		}
	}

#ifdef VERSION_V2
	hub_mesh_message_t* createHubMessage(uint16_t messageType) {
		hub_mesh_message_t* message = new hub_mesh_message_t();
		memset(message, 0, sizeof(hub_mesh_message_t));
    	Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &message->header.sourceCrownstoneId);
		message->header.messageType = messageType;
		return message;
	}
#else
	hub_mesh_message_t* createHubMessage(uint16_t messageType) {
		hub_mesh_message_t* message = new hub_mesh_message_t();
		memset(message, 0, sizeof(hub_mesh_message_t));
		memcpy(&message->header.sourceAddress, &_myAddr.addr, BLE_GAP_ADDR_LEN);
		return message;
	}
#endif

#ifdef VERSION_V2
	std::string getTargetAddress(device_mesh_message_t* msg) {
		char buffer[32];
		sprintf(buffer, "%d", msg->header.targetCrownstoneId);
		return std::string(buffer);
	}
#else
	std::string getTargetAddress(device_mesh_message_t* msg) {
		char buffer[32];
		sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", msg->header.targetAddress[5],
					msg->header.targetAddress[4], msg->header.targetAddress[3], msg->header.targetAddress[2],
					msg->header.targetAddress[1], msg->header.targetAddress[0]);
		return std::string(buffer);
	}
#endif

#ifdef VERSION_V2
	std::string getSourceAddress(hub_mesh_message_t* msg) {
		char buffer[32];
		sprintf(buffer, "%d", msg->header.sourceCrownstoneId);
		return std::string(buffer);
	}
#else
	std::string getSourceAddress(hub_mesh_message_t* msg) {
		char buffer[32];
		sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", msg->header.sourceAddress[5],
					msg->header.sourceAddress[4], msg->header.sourceAddress[3], msg->header.sourceAddress[2],
					msg->header.sourceAddress[1], msg->header.sourceAddress[0]);
		return std::string(buffer);
	}
#endif

private:
	MeshControl();

	MeshControl(MeshControl const&); //! singleton, deny implementation
	void operator=(MeshControl const &); //! singleton, deny implementation

	// stores the MAC address of the devices to be used for mesh message handling
    ble_gap_addr_t _myAddr;

};
