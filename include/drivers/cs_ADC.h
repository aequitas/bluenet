/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

extern "C" {
#include <nrf_drv_saadc.h>
#include <nrf_drv_timer.h>
#include <nrf_drv_ppi.h>
}

#include "structs/buffer/cs_CircularBuffer.h"
#include "structs/buffer/cs_StackBuffer.h"
#include "structs/buffer/cs_DifferentialBuffer.h"
#include "cfg/cs_Config.h"

typedef void (*adc_done_cb_t) (nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum);

struct adc_done_cb_data_t {
	adc_done_cb_t callback;
	nrf_saadc_value_t* buffer;
	uint16_t bufSize;
	uint8_t bufNum;
};

//#include "common/cs_Types.h"

/** Analog-Digital conversion.
 *
 * The ADC is a hardware peripheral that can be initialized for a particular pin. Subsequently, the analog signal
 * is converted into a digital value. The resolution of the conversion is limited (default 10 bits). The conversion
 * is used to get the current curve.
 */
class ADC {

public:
	//! Use static variant of singleton, no dynamic memory allocation
	static ADC& getInstance() {
		static ADC instance;
		return instance;
	}

	/** Initialize ADC.
	 * @param pins Array of pins to use as input (AIN<pin>). ADC will alternate between these pins.
	 * @param size Size of the array.
	 *
	 * The init function must called once before operating the AD converter.
	 * Call it after you start the SoftDevice.
	 */
	uint32_t init(uint8_t pins[], uint8_t numPins);

	/** Start the ADC sampling
	 */
	void start();

	/** Stop the ADC sampling
	 */
	void stop();


	/** Release a buffer, so it can be used again by the ADC.
	 *
	 * @param bufNum The buffer num as received in the done callback.
	 *
	 * @return true on success.
	 */
	bool releaseBuffer(uint8_t bufNum);

	/** Set the callback which is called when a buffer is filled
	 */
	void setDoneCallback(adc_done_cb_t callback);

	//! Function to be called from interrupt, do not do much there!
	void update(uint32_t value);

	void update(nrf_saadc_value_t* buf);

	static void staticTimerHandler(nrf_timer_event_t event_type, void* ptr) {
//		_log(SERIAL_DEBUG, "t\r\n");
	}

	uint32_t _lastStartTime;

private:
	/** Constructor
	 */
//	ADC(): _buffers(NULL), _timeBuffers(NULL), _circularBuffers(NULL) {}
	ADC();

	//! This class is singleton, deny implementation
	ADC(ADC const&);
	//! This class is singleton, deny implementation
	void operator=(ADC const &);

	uint8_t _pins[CS_ADC_MAX_PINS];
	uint8_t _numPins;
	nrf_drv_timer_t* _timer;
	nrf_ppi_channel_t _ppiChannel;
	nrf_saadc_value_t* _bufferPointers[CS_ADC_NUM_BUFFERS];

	nrf_saadc_value_t* _bufferForCallback;
	uint8_t _lastFilledBufInd;
	uint8_t _queuedBufInd;
	uint8_t _currentBufInd;


	adc_done_cb_data_t _doneCallbackData;

	//! Function to set the input pin, this can be done after each sample. Only used internally!
	uint32_t configPin(uint8_t channal, uint8_t pin);
};
