////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Copyright (C) 2015-2020 M-Way Solutions GmbH
// ** Contact: https://www.blureange.io/licensing
// **
// ** This file is part of the Bluerange/FruityMesh implementation
// **
// ** $BR_BEGIN_LICENSE:GPL-EXCEPT$
// ** Commercial License Usage
// ** Licensees holding valid commercial Bluerange licenses may use this file in
// ** accordance with the commercial license agreement provided with the
// ** Software or, alternatively, in accordance with the terms contained in
// ** a written agreement between them and M-Way Solutions GmbH. 
// ** For licensing terms and conditions see https://www.bluerange.io/terms-conditions. For further
// ** information use the contact form at https://www.bluerange.io/contact.
// **
// ** GNU General Public License Usage
// ** Alternatively, this file may be used under the terms of the GNU
// ** General Public License version 3 as published by the Free Software
// ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
// ** included in the packaging of this file. Please review the following
// ** information to ensure the GNU General Public License requirements will
// ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
// **
// ** $BR_END_LICENSE$
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////
#ifdef SIM_ENABLED

#include <SystemTest.h>
#include <stdio.h>
#include <types.h>
#include <CherrySim.h>
#include <FruityMesh.h>
#include <FruityHalBleGatt.h>
#include <json.hpp>
#include <Logger.h>
#include <fstream>

extern "C" {
#include <app_timer.h>
#include <aes.h>
}

/**
This file implements all the softdevice calls used by FruityMesh

Some of the implementations provide a rather good simulation of the softdevice behaviour, others are implemented
to at least give a hint of the expected SoftDevice behaviour.
All calls work in conjunction with helper methods and simulation routines defined in CherrySim.
*/

using json = nlohmann::json;

//These variables are normally defined by the linker sections, so we need to define them here
uint32_t __application_start_address;
uint32_t __application_end_address;
uint32_t __start_conn_type_resolvers;
uint32_t __stop_conn_type_resolvers;

//Pointer to FruityMesh state
GlobalState* simGlobalStatePtr;

//nRF hardware abstraction
NRF_FICR_Type* simFicrPtr;
NRF_UICR_Type* simUicrPtr;
NRF_GPIO_Type* simGpioPtr;
uint8_t* simFlashPtr;


//########################################### SoftDevice Call Redirection #####################################################

extern "C"
{
	//################ UART ##################
	void nrf_gpio_pin_set(uint32_t pin_number) {}
	void nrf_gpio_pin_clear(uint32_t pin_number) {}
	void nrf_gpio_pin_toggle(uint32_t pin_number) {}
	void nrf_gpio_cfg_default(uint32_t pin_number) {}
	void nrf_gpio_cfg_output(uint32_t pin_number) {}
	void nrf_gpio_cfg_input(uint32_t pin_number, nrf_gpio_pin_pull_t pull_config) {}
	void nrf_uart_baudrate_set(NRF_UART_Type *p_reg, nrf_uart_baudrate_t baudrate) {}
	void nrf_uart_configure(NRF_UART_Type *p_reg, nrf_uart_parity_t parity, nrf_uart_hwfc_t hwfc) {}
	void nrf_uart_txrx_pins_set(NRF_UART_Type *p_reg, uint32_t pseltxd, uint32_t pselrxd) {}
	void nrf_uart_hwfc_pins_set(NRF_UART_Type *p_reg, uint32_t pselrts, uint32_t pselcts) {}
	void nrf_uart_event_clear(NRF_UART_Type *p_reg, nrf_uart_event_t event) {}
	void nrf_uart_enable(NRF_UART_Type *p_reg) {}
	void nrf_uart_task_trigger(NRF_UART_Type *p_reg, nrf_uart_task_t task) {}


	void nrf_uart_int_enable(NRF_UART_Type *p_reg, uint32_t int_mask) 
	{
		START_OF_FUNCTION();
		if (p_reg != NRF_UART0)
		{
			//At the moment this function is only used with NRF_UART0.
			//Other functionality is not implemented.
			SIMEXCEPTION(IllegalArgumentException);
		}
		SoftdeviceState &state = cherrySimInstance->currentNode->state;
		state.currentlyEnabledUartInterrupts |= int_mask;
	}

	void nrf_uart_int_disable(NRF_UART_Type * p_reg, uint32_t int_mask)
	{
		START_OF_FUNCTION();
		if (p_reg != NRF_UART0)
		{
			//At the moment this function is only used with NRF_UART0.
			//Other functionality is not implemented.
			SIMEXCEPTION(IllegalArgumentException);
		}
		cherrySimInstance->currentNode->state.currentlyEnabledUartInterrupts &= ~int_mask;
	}

	void nrf_wdt_reload_request_set(int rr_register)
	{
		START_OF_FUNCTION();
		cherrySimInstance->currentNode->lastWatchdogFeedTime = cherrySimInstance->currentNode->state.timeMs;
	}

	void nrf_wdt_task_trigger(int task) {}

	void nrf_wdt_behaviour_set(int behaviour) {}

	void nrf_wdt_reload_value_set(uint32_t reload_value)
	{
		START_OF_FUNCTION();
		cherrySimInstance->currentNode->watchdogTimeout = reload_value / 32768UL * 1000UL;
		nrf_wdt_reload_request_set(0);
	}

	void nrf_wdt_reload_request_enable(int rr_regist) {}

	bool nrf_uart_int_enable_check(NRF_UART_Type * p_reg, uint32_t int_mask)
	{
		START_OF_FUNCTION();
		if (p_reg != NRF_UART0)
		{
			//At the moment this function is only used with NRF_UART0.
			//Other functionality is not implemented.
			SIMEXCEPTION(IllegalArgumentException);
		}
		bool retVal = ((cherrySimInstance->currentNode->state.currentlyEnabledUartInterrupts) & int_mask) == int_mask;
		return retVal;
	}

	bool nrf_uart_event_check(NRF_UART_Type * p_reg, nrf_uart_event_t event)
	{
		START_OF_FUNCTION();
		if (p_reg != NRF_UART0)
		{
			//At the moment this function is only used with NRF_UART0.
			//Other functionality is not implemented.
			SIMEXCEPTION(IllegalArgumentException);
		}
		if (event != NRF_UART_EVENT_RXDRDY)
		{
			//At the moment we don't simulate uart errors or timeouts.
			return false;
		}
		SoftdeviceState &state = cherrySimInstance->currentNode->state;
		return state.uartBufferLength != state.uartReadIndex;
	}

	void ResetUart(SoftdeviceState &state)
	{
		START_OF_FUNCTION();
		state.uartBuffer.zeroData();
		state.uartReadIndex = 0;
		state.uartBufferLength = 0;
		state.currentlyEnabledUartInterrupts = 0;
	}

	void nrf_delay_ms(uint32_t volatile number_of_ms) {}

	uint8_t nrf_uart_rxd_get(NRF_UART_Type * p_reg)
	{
		START_OF_FUNCTION();
		if (p_reg != NRF_UART0)
		{
			//At the moment this function is only used with NRF_UART0.
			//Other functionality is not implemented.
			SIMEXCEPTION(IllegalArgumentException);
		}
		SoftdeviceState &state = cherrySimInstance->currentNode->state;
		if (state.uartBufferLength == state.uartReadIndex)
		{
			//Check nrf_uart_event_check first!
			SIMEXCEPTION(TriedToReadEmptyBufferException);
		}

		uint8_t retVal = state.uartBuffer[state.uartReadIndex];
		state.uartReadIndex++;
		if (state.uartBufferLength == state.uartReadIndex)
		{
			ResetUart(state);
		}
		return retVal;
	}

	uint8_t ST_getRebootReason()
	{
		START_OF_FUNCTION();
		return (uint8_t)cherrySimInstance->currentNode->rebootReason;
	}

	u32 bmg250_init(bmg250_dev * data)
	{
		START_OF_FUNCTION();
		if (cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was already initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		cherrySimInstance->currentNode->bmgWasInit = true;
		return BMG250_OK;
	}

	u8 bmg250_set_power_mode(const bmg250_dev * dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	u8 bmg250_get_sensor_settings(bmg250_cfg * gyro_cfg, const bmg250_dev * dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	uint32_t bmg250_set_sensor_settings(const bmg250_cfg * gyro_cfg, const bmg250_dev * dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	int8_t bmg250_set_fifo_wm(uint8_t wm_frame_count, uint16_t *fifo_length, const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	int8_t bmg250_set_fifo_config(uint8_t config, uint8_t enable, const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	int8_t bmg250_get_fifo_data(const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	int8_t bmg250_extract_gyro(bmg250_sensor_data *gyro_data, uint8_t *data_length, const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}
	int8_t bmg250_get_sensor_data(uint8_t data_sel, bmg250_sensor_data * gyro, const bmg250_dev * dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		gyro->x          = (uint16_t)cherrySimInstance->simState.rnd.nextU32();
		gyro->y          = (uint16_t)cherrySimInstance->simState.rnd.nextU32();
		gyro->z          = (uint16_t)cherrySimInstance->simState.rnd.nextU32();
		gyro->sensortime =           cherrySimInstance->simState.rnd.nextU32();
		return BMG250_OK;
	}

	int8_t bmg250_set_fifo_flush(const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	int8_t bmg250_set_int_config(const bmg250_int_settg *int_config, const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	uint32_t bmg250_set_regs(uint8_t reg_addr, uint8_t *data, uint16_t len, const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	int8_t bmg250_get_int_status(uint8_t *int_status, const bmg250_dev *dev)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bmgWasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		return BMG250_OK;
	}

	uint32_t TLV493D_A1B6_initialise(i32 dataPin)
	{
		START_OF_FUNCTION();
		if (cherrySimInstance->currentNode->Tlv49dA1b6WasInit)
		{
			//Was already initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		cherrySimInstance->currentNode->Tlv49dA1b6WasInit = true;
		return 0;
	}

	void TLV493D_A1B6_set_operation_mode(void * ptr, uint32_t mode)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->Tlv49dA1b6WasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
	}

	uint32_t TLV493D_A1B6_read_frame(void * ptr, TLV493D_data_frame_t * out)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->Tlv49dA1b6WasInit)
		{
			//Was not initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		out->x    = (uint16_t)cherrySimInstance->simState.rnd.nextU32();
		out->y    = (uint16_t)cherrySimInstance->simState.rnd.nextU32();
		out->z    = (uint16_t)cherrySimInstance->simState.rnd.nextU32();
		out->temp = (uint16_t)cherrySimInstance->simState.rnd.nextU32();
		return 0;
	}

	//############### GAP ###################

	uint32_t sd_ble_gap_adv_data_set(const uint8_t* p_data, uint8_t dlen, const uint8_t* p_sr_data, uint8_t srdlen)
	{
		START_OF_FUNCTION();
		if (cherrySimInstance->simConfig.sdBleGapAdvDataSetFailProbability != 0 && PSRNG() < cherrySimInstance->simConfig.sdBleGapAdvDataSetFailProbability) {
			printf("Simulated fail for sd_ble_gap_adv_data_set\n");
			return NRF_ERROR_INVALID_STATE;
		}

		//TODO: Should check advertising data if it is valid or not (parse length and type fields)

		CheckedMemcpy(cherrySimInstance->currentNode->state.advertisingData, p_data, dlen);
		cherrySimInstance->currentNode->state.advertisingDataLength = dlen;

		//TODO: could copy scan response data

		return 0;
	}

	uint32_t sd_ble_gap_adv_stop()
	{
		START_OF_FUNCTION();
		cherrySimInstance->currentNode->state.advertisingActive = false;

		//TODO: could return invalid sate

		return 0;
	}

	static FruityHal::BleGapAdvType AdvertisingTypeToGeneric(u8 type)
	{
		switch (type)
		{
#if SDK == 15
			case BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED:
				return FruityHal::BleGapAdvType::ADV_IND;
			case BLE_GAP_ADV_TYPE_CONNECTABLE_NONSCANNABLE_DIRECTED:
				return FruityHal::BleGapAdvType::ADV_DIRECT_IND;
			case BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED:
				return FruityHal::BleGapAdvType::ADV_SCAN_IND;
			case BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED:
				return FruityHal::BleGapAdvType::ADV_NONCONN_IND;
#else
			case BLE_GAP_ADV_TYPE_ADV_IND:
				return FruityHal::BleGapAdvType::ADV_IND;
			case BLE_GAP_ADV_TYPE_ADV_DIRECT_IND:
				return FruityHal::BleGapAdvType::ADV_DIRECT_IND;
			case BLE_GAP_ADV_TYPE_ADV_SCAN_IND:
				return FruityHal::BleGapAdvType::ADV_SCAN_IND;
			case BLE_GAP_ADV_TYPE_ADV_NONCONN_IND:
				return FruityHal::BleGapAdvType::ADV_NONCONN_IND;
#endif
			default:
                return FruityHal::BleGapAdvType::ADV_IND;
		}
	}

	uint32_t sd_ble_gap_adv_start(const ble_gap_adv_params_t* p_adv_params)
	{
		START_OF_FUNCTION();
		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		u8 activePeripheralConnCount = 0;
		for (int i = 0; i < cherrySimInstance->currentNode->state.configuredTotalConnectionCount; i++) {
			if (cherrySimInstance->currentNode->state.connections[i].connectionActive && !cherrySimInstance->currentNode->state.connections[i].isCentral) {
				activePeripheralConnCount++;
			}
		}

		//If the device wants to advertise connectable, we can only allow this if it has another free peripheral connection
		if (AdvertisingTypeToGeneric(p_adv_params->type) != FruityHal::BleGapAdvType::ADV_NONCONN_IND) {
			if (activePeripheralConnCount >= cherrySimInstance->currentNode->state.configuredPeripheralConnectionCount) {
				return NRF_ERROR_CONN_COUNT;
			}
		}

		//TODO: Check for other error conditions such as invalid state and invalid param as well

		cherrySimInstance->currentNode->state.advertisingActive = true;
		cherrySimInstance->currentNode->state.advertisingIntervalMs = UNITS_TO_MSEC(p_adv_params->interval, UNIT_0_625_MS);
		cherrySimInstance->currentNode->state.advertisingType = AdvertisingTypeToGeneric(p_adv_params->type);

		//TODO: could return invalid state

		//FIXME: Check parameters and return invalid param on fail

		return NRF_SUCCESS;
	}

	uint8_t lis2dh12_init(uint32_t interface, i32 slaveSelectPin)
	{
		START_OF_FUNCTION();
		if (cherrySimInstance->currentNode->lis2dh12WasInit)
		{
			//Already initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		cherrySimInstance->currentNode->lis2dh12WasInit = true;

		return 0;
	}
	void lis2dh12_reset() {}
	void lis2dh12_enable() {}
	void lis2dh12_set_fifo_mode(uint32_t) {}
	void lis2dh12_set_scale(uint32_t) {}
	void lis2dh12_set_resolution(uint32_t) {}
	void lis2dh12_set_data_rate(uint32_t) {}
	void lis2dh12_set_mode(uint32_t) {}
	void lis2dh12_write_reg2(uint32_t) {}
	void lis2dh12_set_sleep_to_wake_threshold(float) {}
	uint32_t lis2dh12_set_interrupts(uint8_t,uint8_t) { return 0; }
	//TODO check the functionality once it is used by INS,currently count not changing value is fine
	lis2dh12_ret_t lis2dh12_set_fifo_watermark(size_t count) { return LIS2DH12_RET_OK;}
	lis2dh12_ret_t lis2dh12_get_fifo_sample_number(size_t* count) {return LIS2DH12_RET_OK;}
	lis2dh12_ret_t lis2dh12_set_sample_rate(lis2dh12_sample_rate_t sample_rate) { return LIS2DH12_RET_OK;}
	uint8_t lis2dh12_set_latch() { return 0; }
	void lis2dh12_set_hlactive() {}
	void lis2dh12_set_sleep_to_wake_duration(uint32_t, uint32_t, float) {}
	uint32_t lis2dh12_set_int1_ths(float) { return 0; }
	uint32_t lis2dh12_set_int1_duration(uint32_t) { return 0; }
	uint32_t lis2dh12_set_int1_cfg(uint32_t) { return 0; }
	lis2dh12_ret_t lis2dh12_read_int1_src(uint8_t *ctrl, uint32_t size) { return LIS2DH12_RET_OK; }
	uint32_t lis2dh12_reset_act_dur() { return 0; }
	uint32_t lis2dh12_reset_act_ths() { return 0; }
	void lis2dh12_read_samples(lis2dh12_sensor_buffer_t* buffer, uint32_t count)
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->lis2dh12WasInit)
		{
			//Not initialized!
			SIMEXCEPTION(IllegalStateException);
		}

		for (uint32_t i = 0; i < count; i++)
		{
			buffer[i].sensor.x = cherrySimInstance->simState.rnd.nextU32();
			buffer[i].sensor.y = cherrySimInstance->simState.rnd.nextU32();
			buffer[i].sensor.z = cherrySimInstance->simState.rnd.nextU32();
		}
	}
	uint32_t nrf_drv_gpiote_in_init(nrf_drv_gpiote_pin_t pin, nrf_drv_gpiote_in_config_t const * p_config, nrf_drv_gpiote_evt_handler_t evt_handler) { return 0; }
	uint32_t nrf_drv_gpiote_in_event_enable(nrf_drv_gpiote_pin_t pin, bool) { return 0; }

	lis2dh12_ret_t lis2dh12_get_fifo_status(fifo_status* status)
	{

		START_OF_FUNCTION();
		if (cherrySimInstance->currentNode->lis2dh12WasInit) 
		{
			status->number_of_fifo_sample = 0;
			status->fifo_watermark_interrupt = 0;
			status->fifo_is_empty = 1;
		}
		return LIS2DH12_RET_OK;
	}

	uint32_t bme280_init(int32_t slaveSelectPin)
	{
		START_OF_FUNCTION();
		if (cherrySimInstance->currentNode->bme280WasInit)
		{
			//Already initialized!
			SIMEXCEPTION(IllegalStateException);
		}
		cherrySimInstance->currentNode->bme280WasInit = true;
		return 0;
	}
	uint32_t bme280_set_mode_assert(uint32_t) { return 0; }
	uint32_t bme280_set_oversampling_press(uint32_t) { return 0; }
	uint32_t bme280_set_oversampling_temp(uint32_t) { return 0; }
	uint32_t bme280_set_oversampling_hum(uint32_t) { return 0; }
	uint32_t bme280_set_iir(uint32_t) { return 0; }
	uint32_t bme280_set_interval(uint32_t) { return 0; }
	uint32_t bme280_read_reg(uint32_t) { return 0; }
	uint32_t bme280_read_measurements() { return 0; }
	uint32_t bme280_get_pressure()
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bme280WasInit)
		{
			//Not initialized!
			SIMEXCEPTION(IllegalStateException);
		}

		return cherrySimInstance->simState.rnd.nextU32();
	}
	uint32_t bme280_get_temperature()
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bme280WasInit)
		{
			//Not initialized!
			SIMEXCEPTION(IllegalStateException);
		}

		return cherrySimInstance->simState.rnd.nextU32();
	}
	uint32_t bme280_get_humidity()
	{
		START_OF_FUNCTION();
		if (!cherrySimInstance->currentNode->bme280WasInit)
		{
			//Not initialized!
			SIMEXCEPTION(IllegalStateException);
		}

		return cherrySimInstance->simState.rnd.nextU32();
	}

	uint32_t sd_ble_gap_connect(const ble_gap_addr_t* p_peer_addr, const ble_gap_scan_params_t* p_scan_params, const ble_gap_conn_params_t* p_conn_params)
	{
		START_OF_FUNCTION();
		if (cherrySimInstance->currentNode->state.connectingActive) {
			return NRF_ERROR_INVALID_STATE;
		}

		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		//Count our current connections as a peripheral and check if a free spot is available in the totalConnections
		u8 activeCentralConnCount = 0;
		bool freeSpot = false;
		for (int i = 0; i < cherrySimInstance->currentNode->state.configuredTotalConnectionCount; i++) {
			if (cherrySimInstance->currentNode->state.connections[i].connectionActive && cherrySimInstance->currentNode->state.connections[i].isCentral) {
				activeCentralConnCount++;
			}
			if (!cherrySimInstance->currentNode->state.connections[i].connectionActive) {
				freeSpot = true;
			}
		}

		//Number of pheripheral connections exceeded
		//TODO: Not sure if this is the correct error or if it should be error_conn_count
		if (activeCentralConnCount >= cherrySimInstance->currentNode->state.configuredCentralConnectionCount) return NRF_ERROR_RESOURCES;

		//Number of total connections exceeded
		if (!freeSpot) return NRF_ERROR_CONN_COUNT;

		//Scanning is stopped as soon as connecting is activated
		cherrySimInstance->currentNode->state.scanningActive = false;

		cherrySimInstance->currentNode->state.connectingActive = true;
		cherrySimInstance->currentNode->state.connectingStartTimeMs = cherrySimInstance->simState.simTimeMs;
		CheckedMemcpy(&cherrySimInstance->currentNode->state.connectingPartnerAddr, p_peer_addr, sizeof(*p_peer_addr));

		cherrySimInstance->currentNode->state.connectingIntervalMs = UNITS_TO_MSEC(p_scan_params->interval, UNIT_0_625_MS);
		cherrySimInstance->currentNode->state.connectingWindowMs = UNITS_TO_MSEC(p_scan_params->window, UNIT_0_625_MS);
		cherrySimInstance->currentNode->state.connectingTimeoutTimestampMs = cherrySimInstance->simState.simTimeMs + p_scan_params->timeout * 1000UL;

		cherrySimInstance->currentNode->state.connectingParamIntervalMs = UNITS_TO_MSEC(p_conn_params->min_conn_interval, UNIT_1_25_MS);

		//TODO: could save more params, could return invalid state

		return 0;
	}

	uint32_t sd_ble_gap_connect_cancel()
	{
		START_OF_FUNCTION();
		cherrySimInstance->currentNode->state.connectingActive = false;

		return 0;
	}

	uint32_t sd_ble_gap_disconnect(uint16_t conn_handle, uint8_t hci_status_code)
	{
		START_OF_FUNCTION();
		//Find the connection by its handle
		SoftdeviceConnection* connection = cherrySimInstance->findConnectionByHandle(cherrySimInstance->currentNode, conn_handle);

		return cherrySimInstance->DisconnectSimulatorConnection(connection, BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);


		return NRF_SUCCESS;
	}

	//Called by the central to start encrypting the connection
	uint32_t sd_ble_gap_encrypt(uint16_t conn_handle, const ble_gap_master_id_t* p_master_id, const ble_gap_enc_info_t* p_enc_info)
	{
		START_OF_FUNCTION();
		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		SoftdeviceConnection* connection = cherrySimInstance->findConnectionByHandle(cherrySimInstance->currentNode, conn_handle);

		if (connection == nullptr) {
			return BLE_ERROR_INVALID_CONN_HANDLE;
		}

		if (!connection->isCentral) SIMEXCEPTION(IllegalStateException); //Peripheral cannot start encryption

		//Send an event to the connection partner to request the key information
		simBleEvent s1;
		s1.globalId = cherrySimInstance->simState.globalEventIdCounter++;
		s1.bleEvent.header.evt_id = BLE_GAP_EVT_SEC_INFO_REQUEST;
		s1.bleEvent.header.evt_len = s1.globalId;
		s1.bleEvent.evt.gap_evt.conn_handle = connection->connectionHandle;
		ble_gap_addr_t address = CherrySim::Convert(&cherrySimInstance->currentNode->address);
		CheckedMemcpy(&s1.bleEvent.evt.gap_evt.params.sec_info_request.peer_addr, &address, sizeof(ble_gap_addr_t));
		s1.bleEvent.evt.gap_evt.params.sec_info_request.master_id = { 0 }; //TODO: incomplete information
		s1.bleEvent.evt.gap_evt.params.sec_info_request.enc_info = 0; //TODO: incomplete information
		s1.bleEvent.evt.gap_evt.params.sec_info_request.id_info = 0; //TODO: incomplete information
		s1.bleEvent.evt.gap_evt.params.sec_info_request.sign_info = 0; //TODO: incomplete information
		connection->partner->eventQueue.push_back(s1);

		//Save the key that should be used for encrypting the connection
		CheckedMemcpy(cherrySimInstance->currentNode->state.currentLtkForEstablishingSecurity, p_enc_info->ltk, 16);

		return NRF_SUCCESS;
	}

	//Called by the peripheral to send back its key
	uint32_t sd_ble_gap_sec_info_reply(uint16_t conn_handle, const ble_gap_enc_info_t* p_enc_info, const ble_gap_irk_t* p_id_info, const ble_gap_sign_info_t* p_sign_info)
	{
		START_OF_FUNCTION();
		SoftdeviceConnection* connection = cherrySimInstance->findConnectionByHandle(cherrySimInstance->currentNode, conn_handle);

		if (connection == nullptr) {
			return BLE_ERROR_INVALID_CONN_HANDLE;
		}

		//Check if the encryption key matches
		if (
			memcmp(connection->partner->state.currentLtkForEstablishingSecurity, p_enc_info->ltk, 16) == 0
		) {
			//Set our own conneciton to encrypted
			connection->connectionEncrypted = true;
			simBleEvent s1;
			s1.globalId = cherrySimInstance->simState.globalEventIdCounter++;
			s1.bleEvent.header.evt_id = BLE_GAP_EVT_CONN_SEC_UPDATE;
			s1.bleEvent.header.evt_len = s1.globalId;
			s1.bleEvent.evt.gap_evt.conn_handle = connection->connectionHandle;
			s1.bleEvent.evt.gap_evt.params.conn_sec_update.conn_sec.encr_key_size = 16;
			s1.bleEvent.evt.gap_evt.params.conn_sec_update.conn_sec.sec_mode.sm = 1;
			s1.bleEvent.evt.gap_evt.params.conn_sec_update.conn_sec.sec_mode.lv = 3;
			cherrySimInstance->currentNode->eventQueue.push_back(s1);

			//Set our own partners connection to encrypted
			connection->partnerConnection->connectionEncrypted = true;
			simBleEvent s2;
			s2.globalId = cherrySimInstance->simState.globalEventIdCounter++;
			s2.bleEvent.header.evt_id = BLE_GAP_EVT_CONN_SEC_UPDATE;
			s2.bleEvent.header.evt_len = s2.globalId;
			s2.bleEvent.evt.gap_evt.conn_handle = connection->connectionHandle;
			s2.bleEvent.evt.gap_evt.params.conn_sec_update.conn_sec.encr_key_size = 16;
			s2.bleEvent.evt.gap_evt.params.conn_sec_update.conn_sec.sec_mode.sm = 1;
			s2.bleEvent.evt.gap_evt.params.conn_sec_update.conn_sec.sec_mode.lv = 3;
			connection->partner->eventQueue.push_back(s2);
		}
		//Keys do not match, generate a failure
		else {
			//Disconnect the connection with a MIC error
			cherrySimInstance->DisconnectSimulatorConnection(connection, BLE_HCI_CONN_TERMINATED_DUE_TO_MIC_FAILURE, BLE_HCI_CONNECTION_TIMEOUT);
		}

		return NRF_SUCCESS;
	}

	uint32_t sd_ble_gap_conn_param_update(uint16_t conn_handle, const ble_gap_conn_params_t* p_conn_params)
	{
		START_OF_FUNCTION();
		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		return 0;
	}

	uint32_t sd_ble_gap_scan_stop()
	{
		START_OF_FUNCTION();
        if (cherrySimInstance->currentNode->state.scanningActive == false) return NRF_ERROR_INVALID_STATE;

		cherrySimInstance->currentNode->state.scanningActive = false;
		cherrySimInstance->currentNode->state.scanIntervalMs = 0;
		cherrySimInstance->currentNode->state.scanWindowMs = 0;

		return 0;
	}

	uint32_t sd_ble_gap_scan_start(const ble_gap_scan_params_t* p_scan_params)
	{
		START_OF_FUNCTION();
		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		//Scanning is not possible while in a connection
		if (cherrySimInstance->currentNode->state.connectingActive) return NRF_ERROR_INVALID_STATE;

		cherrySimInstance->currentNode->state.scanningActive = true;
		cherrySimInstance->currentNode->state.scanIntervalMs = UNITS_TO_MSEC(p_scan_params->interval, UNIT_0_625_MS);
		cherrySimInstance->currentNode->state.scanWindowMs = UNITS_TO_MSEC(p_scan_params->window, UNIT_0_625_MS);


		return 0;
	}
	uint32_t sd_ble_gap_address_set(uint8_t addr_cycle_mode, const ble_gap_addr_t* p_addr)
	{
		START_OF_FUNCTION();
		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		//Just for checking that we do not change the type, could be removed
		if (p_addr->addr_type != (u8)cherrySimInstance->currentNode->address.addr_type) {
			SIMEXCEPTION(IllegalStateException);
		}

		//We just set it without any error detection
		cherrySimInstance->currentNode->address = CherrySim::Convert(p_addr);

		return 0;
	}

	uint32_t sd_ble_gap_address_get(ble_gap_addr_t* p_addr)
	{
		START_OF_FUNCTION();		
		FruityHal::BleGapAddr addr = cherrySimInstance->currentNode->address;
		CheckedMemcpy(p_addr->addr, addr.addr, BLE_GAP_ADDR_LEN);
		p_addr->addr_type = (u8)addr.addr_type;


		return 0;
	}

	uint32_t sd_ble_gap_rssi_start(uint16_t conn_handle, uint8_t threshold_dbm, uint8_t skip_count)
	{
		START_OF_FUNCTION();
		SoftdeviceConnection* connection = cherrySimInstance->findConnectionByHandle(cherrySimInstance->currentNode, conn_handle);
		if (connection == nullptr) {
			return BLE_ERROR_INVALID_CONN_HANDLE;
		}
		connection->rssiMeasurementActive = true;

		return 0;
	}

	uint32_t sd_ble_gap_rssi_stop(uint16_t conn_handle)
	{
		START_OF_FUNCTION();
		SoftdeviceConnection* connection = cherrySimInstance->findConnectionByHandle(cherrySimInstance->currentNode, conn_handle);
		if (connection == nullptr) {
			return BLE_ERROR_INVALID_CONN_HANDLE;
		}
		connection->rssiMeasurementActive = false;

		return 0;
	}

	//############### GATT Services ###################

	uint32_t sd_ble_uuid_vs_add(const ble_uuid128_t* p_vs_uuid, uint8_t* p_uuid_type)
	{
		START_OF_FUNCTION();
		//TODO: Should have a table with maybe 128 UUIDs (probably depending on the sd ble enable params)
		//TODO: Should add UUID to an array of uuids and should write a new id starting at BLE_UUID_TYPE_VENDOR_BEGIN into type
		

		return 0;
	}

	uint32_t sd_ble_gatts_service_add(uint8_t type, const ble_uuid_t* p_uuid, uint16_t* p_handle)
	{
		START_OF_FUNCTION();
		//TODO: uuid handle is not checked against table yet, not all possible errors returned

        if ((p_uuid == nullptr) || (p_handle == nullptr)) return NRF_ERROR_INVALID_PARAM;

		nodeEntry * p_tempNode = cherrySimInstance->currentNode;

		if (p_tempNode->state.servicesCount >= SIM_NUM_SERVICES) return NRF_ERROR_NO_MEM;
		p_tempNode->state.services[p_tempNode->state.servicesCount].uuid = *p_uuid;
		// Generate handle value pseudorandomly
		*p_handle = p_uuid->uuid + p_tempNode->state.servicesCount;
		p_tempNode->state.services[p_tempNode->state.servicesCount].handle = *p_handle;
		p_tempNode->state.servicesCount++;
		return 0;
	}

	uint32_t sd_ble_gatts_characteristic_add(uint16_t service_handle, const ble_gatts_char_md_t* p_char_md, const ble_gatts_attr_t* p_attr_char_value, ble_gatts_char_handles_t* p_handles)
	{
		START_OF_FUNCTION();
		//TODO: not all possible errors returned yet

        if ((p_char_md == nullptr) || (p_attr_char_value == nullptr) || (p_handles == nullptr)) return NRF_ERROR_INVALID_PARAM;

		nodeEntry * p_tempNode = cherrySimInstance->currentNode;
		ServiceDB_t * p_tempService = nullptr;

		for (int i = 0; i < p_tempNode->state.servicesCount; i++)
		{
			if (p_tempNode->state.services[i].handle == service_handle)
			{
				p_tempService = &p_tempNode->state.services[i];
				break;
			}
		}

        if (p_tempService == nullptr) return NRF_ERROR_INVALID_PARAM;
		if (p_tempService->charCount >= SIM_NUM_CHARS) return NRF_ERROR_NO_MEM;

		// TODO: Not all information is stored for now
		// Generate handle value pseudorandomly
		p_handles->value_handle = p_tempService->uuid.uuid + p_tempService->charCount * 2 + 1;
		p_tempService->charateristics[p_tempService->charCount].handle = p_handles->value_handle;
		p_tempService->charateristics[p_tempService->charCount].uuid = *p_attr_char_value->p_uuid;
		// FIXME: Enabling of notifications not yet implemented and ignroed in simulator
		p_tempService->charateristics[p_tempService->charCount].cccd_handle = p_handles->value_handle + 1;

		p_tempService->charCount++;

		return 0;
	}

	uint32_t sd_ble_gatts_sys_attr_set(uint16_t conn_handle, const uint8_t* p_sys_attr_data, uint16_t len, uint32_t flags)
	{
		START_OF_FUNCTION();


		return 0;
	}

	uint32_t sd_ble_gattc_descriptors_discover(uint16_t conn_handle, ble_gattc_handle_range_t const *p_handle_range)
	{
		START_OF_FUNCTION();


		return 0;
	}

	uint32_t sd_ble_gattc_primary_services_discover(uint16_t conn_handle, uint16_t start_handle, const ble_uuid_t* p_srvc_uuid)
	{
		START_OF_FUNCTION();


		return 0;
	}

	uint32_t sd_ble_gattc_characteristics_discover(uint16_t conn_handle, const ble_gattc_handle_range_t* p_handle_range)
	{
		START_OF_FUNCTION();


		return 0;
	}

	SoftDeviceBufferedPacket* findFreePacketBuffer(SoftdeviceConnection* connection) {
		START_OF_FUNCTION();
		for (int i = 0; i < SIM_NUM_UNRELIABLE_BUFFERS; i++) {
			if (connection->unreliableBuffers[i].sender == nullptr) {
				return &connection->unreliableBuffers[i];
			}
		}
		return nullptr;
	}

	uint32_t sd_ble_gap_tx_power_set(int8_t tx_power)
	{
		START_OF_FUNCTION();
		if (tx_power == -40 || tx_power == -30 || tx_power == -20 || tx_power == -16
			|| tx_power == -12 || tx_power == -8 || tx_power == -4 || tx_power == 0 || tx_power == 4) {
			cherrySimInstance->currentNode->state.txPower = tx_power;
		}
		else {
			SIMEXCEPTION(IllegalStateException);

		}

		return 0;
	}

	//############### GATT Write ###################

	uint32_t sd_ble_gattc_write(uint16_t conn_handle, const ble_gattc_write_params_t* p_write_params)
	{
		START_OF_FUNCTION();
		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		SoftdeviceConnection* connection = cherrySimInstance->findConnectionByHandle(cherrySimInstance->currentNode, conn_handle);
		//TODO: maybe log this?
		if (connection == nullptr) {
			return BLE_ERROR_INVALID_CONN_HANDLE;
		}

		if (p_write_params->len > connection->connectionMtu - FruityHal::ATT_HEADER_SIZE)
		{
			return NRF_ERROR_DATA_SIZE;
		}

		nodeEntry* partnerNode = connection->partner;
		SoftdeviceConnection* partnerConnection = connection->partnerConnection;

		//Should not happen, sim connection is always terminated at both ends simultaniously
		if (partnerConnection == nullptr) {
			SIMEXCEPTION(IllegalStateException);
		}

		//Always uses the more sophisticated connection simulation, rather than sending packets immediately
		//Fill either reliable or unreliable buffer with the packet
		SoftDeviceBufferedPacket* buffer = nullptr;
		if (p_write_params->write_op == BLE_GATT_OP_WRITE_REQ) {
			//Check if the buffer is free to use
			if (connection->reliableBuffers[0].sender == nullptr) {
				buffer = &connection->reliableBuffers[0];
			}
		}
		else if (p_write_params->write_op == BLE_GATT_OP_WRITE_CMD) {
			buffer = findFreePacketBuffer(connection);
		}
		else {
			SIMEXCEPTION(IllegalStateException);
		}

		if (buffer == nullptr || buffer->sender != 0) {
			return NRF_ERROR_RESOURCES;
		}

		//We save a global id for each packet that is sent, so that we can debug where a packet was generated
		buffer->globalPacketId = cherrySimInstance->simState.globalPacketIdCounter++;
		buffer->sender = cherrySimInstance->currentNode;
		buffer->receiver = partnerNode;
		buffer->connHandle = conn_handle;
		buffer->queueTimeMs = cherrySimInstance->simState.simTimeMs;
		CheckedMemcpy(buffer->data, p_write_params->p_value, p_write_params->len);
		buffer->params.writeParams = *p_write_params;
		buffer->params.writeParams.p_value = buffer->data; //Reassign data pointer to our buffer
		buffer->isHvx = false;
		
		//Record statistics for every packet queued in the SoftDevice
		cherrySimInstance->AddMessageToStats(cherrySimInstance->currentNode->routedPackets, buffer->data, buffer->params.writeParams.len);

		//if (cherrySimInstance->currentNode->id == 37 && conn_handle == 680) printf("Q@NODE %u WRITES %s messageType %u" EOL, cherrySimInstance->currentNode->id, p_write_params->write_op == BLE_GATT_OP_WRITE_REQ ? "WRITE_REQ" : "WRITE_CMD", buffer->data[0]);

		//TODO: check against p_write_params->handle?

		return 0;
	}

	//############### Flash ###################

	uint32_t sd_flash_page_erase(uint32_t page_number)
	{
		START_OF_FUNCTION();

		logt("RS", "Erasing Page %u", page_number);

		u32* p = (u32*)(FLASH_REGION_START_ADDRESS + (u32)page_number * FruityHal::GetCodePageSize());

		for (u32 i = 0; i < FruityHal::GetCodePageSize() / 4; i++) {
			p[i] = 0xFFFFFFFF;
		}


		if (cherrySimInstance->simConfig.simulateAsyncFlash) {
			cherrySimInstance->currentNode->state.numWaitingFlashOperations++;
		}
		else {
			DispatchSystemEvents(FruityHal::SystemEvents::FLASH_OPERATION_SUCCESS);
		}

		return NRF_SUCCESS;
	}

	uint32_t sd_flash_write(uint32_t* const p_dst, const uint32_t* const p_src, uint32_t size)
	{
		START_OF_FUNCTION();
		u32 sourcePage            = ((u32)p_src - FLASH_REGION_START_ADDRESS) / FruityHal::GetCodePageSize();
		u32 sourcePageOffset      = ((u32)p_src - FLASH_REGION_START_ADDRESS) % FruityHal::GetCodePageSize();
		u32 destinationPage       = ((u32)p_dst - FLASH_REGION_START_ADDRESS) / FruityHal::GetCodePageSize();
		u32 destinationPageOffset = ((u32)p_dst - FLASH_REGION_START_ADDRESS) % FruityHal::GetCodePageSize();

		if ((u32)p_src >= FLASH_REGION_START_ADDRESS && (u32)p_src < FLASH_REGION_START_ADDRESS + FruityHal::GetCodeSize()*FruityHal::GetCodePageSize()) {
			logt("RS", "Copy from page %u (+%u) to page %u (+%u), len %u", sourcePage, sourcePageOffset, destinationPage, destinationPageOffset, size * 4);
		}
		else {
			logt("RS", "Write ram to page %u (+%u), len %u", destinationPage, destinationPageOffset, size * 4);
		}

		if (size == 0 || size > SIM_MAX_FLASH_SIZE)
		{
			return NRF_ERROR_INVALID_LENGTH;
		}

		if (((u32)p_src) % 4 != 0) {
			logt("ERROR", "source unaligned");
			SIMEXCEPTION(IllegalArgumentException);
			return NRF_ERROR_INVALID_ADDR;
		}
		if (((u32)p_dst) % 4 != 0) {
			logt("ERROR", "dest unaligned");
			SIMEXCEPTION(IllegalArgumentException);
			return NRF_ERROR_INVALID_ADDR;
		}

		//Only toggle bits from 1 to 0 when writing!
		for (u32 i = 0; i < size; i++) {
			p_dst[i] &= p_src[i];
		}

		if (cherrySimInstance->simConfig.simulateAsyncFlash) {
			cherrySimInstance->currentNode->state.numWaitingFlashOperations++;
		}
		else {
			DispatchSystemEvents(FruityHal::SystemEvents::FLASH_OPERATION_SUCCESS);
		}

		return NRF_SUCCESS;
	}


	//############### sd_ble ###################

	uint32_t sd_ble_enable(ble_enable_params_t* p_ble_enable_params, uint32_t* p_app_ram_base)
	{
		START_OF_FUNCTION();
		//Check if the settings comply with the ble stack
		if (p_ble_enable_params->gap_enable_params.periph_conn_count > cherrySimInstance->currentNode->bleStackMaxPeripheralConnections) {
			SIMEXCEPTION(IllegalArgumentException);
		}
		if (p_ble_enable_params->gap_enable_params.central_conn_count > cherrySimInstance->currentNode->bleStackMaxCentralConnections) {
			SIMEXCEPTION(IllegalArgumentException);
		}
		if (p_ble_enable_params->gap_enable_params.central_conn_count + p_ble_enable_params->gap_enable_params.periph_conn_count > cherrySimInstance->currentNode->bleStackMaxTotalConnections) {
			SIMEXCEPTION(IllegalArgumentException);
		}

		//Apply the settings
		cherrySimInstance->currentNode->state.configuredPeripheralConnectionCount = p_ble_enable_params->gap_enable_params.periph_conn_count;
		cherrySimInstance->currentNode->state.configuredCentralConnectionCount = p_ble_enable_params->gap_enable_params.central_conn_count;
		cherrySimInstance->currentNode->state.configuredTotalConnectionCount = p_ble_enable_params->gap_enable_params.central_conn_count + p_ble_enable_params->gap_enable_params.periph_conn_count;

		//TODO: Do the same checks for uuids, services, characteristics, etc,...

		cherrySimInstance->currentNode->state.initialized = true;
		return 0;
	}

	uint32_t sd_ble_tx_packet_count_get(uint16_t conn_handle, uint8_t* p_count)
	{
		START_OF_FUNCTION();
		*p_count = SIM_NUM_UNRELIABLE_BUFFERS;

		return 0;
	}

	uint32_t sd_ble_evt_get(uint8_t* p_dest, uint16_t* p_len)
	{
		START_OF_FUNCTION();
		if (cherrySimInstance->currentNode->eventQueue.size() > 0) {

			simBleEvent bleEvent = cherrySimInstance->currentNode->eventQueue.front();
			cherrySimInstance->currentNode->eventQueue.pop_front();

			if (cherrySimInstance->simEventListener != nullptr) {
				cherrySimInstance->simEventListener->CherrySimBleEventHandler(cherrySimInstance->currentNode, &bleEvent, FruityHal::GetEventBufferSize());
			}

			//We copy the current event so that we can access it during debugging if we want to get more information
			CheckedMemcpy(&cherrySimInstance->currentNode->currentEvent, &bleEvent, sizeof(simBleEvent));

			CheckedMemcpy(p_dest, &bleEvent.bleEvent, FruityHal::GetEventBufferSize());
			*p_len = FruityHal::GetEventBufferSize();

			return NRF_SUCCESS;
		}
		else {
			return NRF_ERROR_NOT_FOUND;
		}
	}

	//############### Other ###################

	uint32_t sd_rand_application_vector_get(uint8_t* p_buff, uint8_t length)
	{
		START_OF_FUNCTION();
		for (int i = 0; i < length; i++) {
			p_buff[i] = (u8)(PSRNG() * 256);
		}

		return 0;
	}

	uint32_t sd_app_evt_wait()
	{
		START_OF_FUNCTION();

		return 0;
	}

	uint32_t sd_nvic_ClearPendingIRQ(IRQn_Type IRQn)
	{
		START_OF_FUNCTION();


		return 0;
	}

	uint32_t sd_nvic_EnableIRQ(IRQn_Type IRQn)
	{
		START_OF_FUNCTION();

		return 0;
	}

	uint32_t sd_nvic_SetPriority(IRQn_Type IRQn, uint32_t priority)
	{
		START_OF_FUNCTION();

		return 0;
	}

	uint32_t sd_radio_notification_cfg_set(uint8_t type, uint8_t distance)
	{
		START_OF_FUNCTION();

		return 0;
	}

	//TODO: Implement this for SoC events
	uint32_t sd_evt_get(uint32_t* evt_id)
	{
		START_OF_FUNCTION();
		//FIXME: Not implemented correctly, should return soc events from a queue

		return NRF_ERROR_NOT_FOUND;
	}

	//Resets a node
	uint32_t sd_nvic_SystemReset()
	{
		START_OF_FUNCTION();
		cherrySimInstance->resetCurrentNode(RebootReason::UNKNOWN);

		return 0;
	}

	uint32_t NVIC_SystemReset()
	{
		START_OF_FUNCTION();
		return sd_nvic_SystemReset();
	}

	//############### App Timer Library ###################

	uint32_t app_timer_cnt_get(uint32_t* time)
	{
		START_OF_FUNCTION();
		if (APP_TIMER_PRESCALER != 0) {
			SIMEXCEPTION(IllegalArgumentException);//Non 0 prescaler not implemented
		}

		*time = (u32)(cherrySimInstance->simState.simTimeMs * (APP_TIMER_CLOCK_FREQ / 1000.0));

		return 0;
	}

	uint32_t app_timer_cnt_diff_compute(uint32_t nowTime, uint32_t previousTime, uint32_t* passedTime)
	{
		START_OF_FUNCTION();
		//Normal case
		if (nowTime >= previousTime) {
			*passedTime = nowTime - previousTime;
		}
		//In case of integer overflow
		else {
			*passedTime = UINT32_MAX - (previousTime - nowTime);
		}

		return 0;
	}


	//#### Not implemented for now ####


	uint32_t sd_softdevice_enable(nrf_clock_lf_cfg_t* clock_source, uint32_t* unsure)
	{
		START_OF_FUNCTION();
		return 0;
	}

	uint32_t sd_softdevice_disable()
	{
		START_OF_FUNCTION();
		return 0;
	}

	uint32_t sd_ble_gap_device_name_set(const ble_gap_conn_sec_mode_t* p_write_perm, const uint8_t* p_dev_name, uint16_t len)
	{
		START_OF_FUNCTION();
		return 0;
	}

	uint32_t sd_ble_gap_appearance_set(uint16_t appearance)
	{
		START_OF_FUNCTION();
		return 0;
	}

	uint32_t sd_ble_gap_ppcp_set(const ble_gap_conn_params_t* p_conn_params)
	{
		START_OF_FUNCTION();
		return 0;
	}

	uint32_t sd_power_dcdc_mode_set(uint8_t dcdc_mode)
	{
		START_OF_FUNCTION();
		return 0;
	}

	uint32_t sd_power_mode_set(uint8_t power_mode)
	{
		START_OF_FUNCTION();
		return 0;
	}

	uint32_t sd_ble_opt_set(uint32_t opt_id, ble_opt_t const *p_opt) {
		START_OF_FUNCTION();
		return NRF_SUCCESS;
	}

	uint32_t sd_ble_gatts_hvx(uint16_t conn_handle, ble_gatts_hvx_params_t const *p_hvx_params) {
		START_OF_FUNCTION();
		if (PSRNG() < cherrySimInstance->simConfig.sdBusyProbability) {
			return NRF_ERROR_BUSY;
		}

		SoftdeviceConnection* connection = cherrySimInstance->findConnectionByHandle(cherrySimInstance->currentNode, conn_handle);
		if (connection == nullptr) {
			return BLE_ERROR_INVALID_CONN_HANDLE;
		}

		nodeEntry* partnerNode = connection->partner;
		SoftdeviceConnection*  partnerConnection = connection->partnerConnection;

		//Should not happen, sim connection is always terminated at both ends simultaniously
		if (partnerConnection == nullptr) {
			SIMEXCEPTION(IllegalStateException);
		}

		//Always uses the more sophisticated connection simulation, rather than sending packets immediately
		SoftDeviceBufferedPacket* buffer = nullptr;
		buffer = findFreePacketBuffer(connection);

		if (buffer == nullptr || buffer->sender != 0) {
			return NRF_ERROR_RESOURCES;
		}

		buffer->globalPacketId = cherrySimInstance->simState.globalPacketIdCounter++;
		buffer->sender = cherrySimInstance->currentNode;
		buffer->receiver = partnerNode;
		buffer->connHandle = conn_handle;
		buffer->queueTimeMs = cherrySimInstance->simState.simTimeMs;
		CheckedMemcpy(&buffer->params.hvxParams, p_hvx_params, sizeof(*p_hvx_params));
		CheckedMemcpy(buffer->data, p_hvx_params->p_data, *p_hvx_params->p_len);
		// This is a workaround for hvxParams keeping only pointer to len.
		buffer->params.hvxParams.p_len = (uint16_t *)*p_hvx_params->p_len;
		buffer->params.hvxParams.p_data = buffer->data; //Reassign data pointer to our buffer
		buffer->isHvx = true;

		//printf("Q@NODE %u WRITES NOTIFICATION messageType %02X" EOL, cherrySimInstance->currentNode->id, buffer->data[0]);

		return 0;
	}

	uint32_t sd_ecb_block_encrypt(nrf_ecb_hal_data_t * p_ecb_data) {
		START_OF_FUNCTION();
		AES_ECB_encrypt(p_ecb_data->cleartext, p_ecb_data->key, p_ecb_data->ciphertext, 16);

		return 0;
	}

	uint32_t sd_power_reset_reason_clr(uint32_t p) {
		START_OF_FUNCTION();
		return 0;
	}

}

// The following enables ASAN flags.
// See https://github.com/google/sanitizers/wiki/AddressSanitizerFlags for more details.
const char *__asan_default_options() {
	return "detect_invalid_pointer_pairs=2:"
		"check_initialization_order=true:"
		"detect_stack_use_after_return=true";
}


//################################## Statistics ###########################################
// Collect statistics over the runtime of the sim
// These calls can be made within FruityMesh using the macros (e.g. SIMSTATCOUNT)
//#########################################################################################

std::map<std::string, int> simStatCounts;
void sim_collect_statistic_count(const char* key)
{
	if (simStatCounts.find(key) != simStatCounts.end())
	{
		simStatCounts[key] = simStatCounts[key] + 1;
	}
	else {
		simStatCounts[key] = 1;
	}
}

std::map<std::string, int> simStatAvgCounts;
std::map<std::string, int> simStatAvgTotal;
void sim_collect_statistic_avg(const char* key, int value)
{
	if (simStatAvgCounts.find(key) != simStatAvgCounts.end())
	{
		simStatAvgCounts[key] = simStatAvgCounts[key] + 1;
		simStatAvgTotal[key] = simStatAvgTotal[key] + value;
	}
	else {
		simStatAvgCounts[key] = 1;
		simStatAvgTotal[key] = value;
	}
}

void sim_print_statistics()
{
	printf("------ COUNTS --------" EOL);
	std::map<std::string, int>::iterator it;
	for (it = simStatCounts.begin(); it != simStatCounts.end(); it++) {
		printf("Key: %s, Count: %d" EOL, it->first.c_str(), it->second);
	}

	printf("------ AVG --------" EOL);
	std::map<std::string, int>::iterator it2;
	for (it2 = simStatAvgCounts.begin(); it2 != simStatAvgCounts.end(); it2++) {
		const char* key = it2->first.c_str();
		int avg = simStatAvgTotal[key] / simStatAvgCounts[key];
		printf("Key: %s, Count: %d, Avg: %d" EOL, key, simStatAvgCounts[key], avg);
	}

	printf("--------------" EOL);
}

uint32_t sim_get_stack_type()
{
	return (uint32_t) cherrySimInstance->currentNode->bleStackType;
}


bool isEmpty(const u8* data, u32 length)
{
	for (u32 i = 0; i < length; i++) {
		if (data[0] != 0x00) return false;
	}
	return true;
}

#endif
