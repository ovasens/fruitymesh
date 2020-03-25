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

#ifndef SYSTEMTEST_H_
#define SYSTEMTEST_H_

//Disable some annoying warnings
#pragma warning( push )
#pragma warning( disable : 4996)
#pragma warning( disable : 4297)

#ifdef _MSC_VER
#ifndef __ASM
#define __ASM               __asm
#endif

#ifndef __INLINE
#define __INLINE            
#endif

#ifndef __WEAK
#define __WEAK              
#endif

#ifndef __ALIGN
#define __ALIGN(n)          __align(n)
#endif

#define GET_SP()                __current_sp()

#define STACK_BASE 0 //FIXME: Should point to stack base
#define STACK_TOP 0 //FIXME: should point to stack top

#define _CRT_SECURE_NO_WARNINGS 1
#endif


#ifdef SIM_ENABLED

#include <debugbreak.h>

//FIXME: This was used to wrap some problematic areas in the code that could not be compiled by VS
#define SIM_PROBLEM

//Some configuration
#define SIMULATOR_NODE_DEFAULT_DBM_TX 4
#define SIMULATOR_NODE_DEFAULT_CALIBRATED_TX -59

#define BLE_STACK_SUPPORT_REQD
#define DEBUG

//This header file must be included in the build settings before every other include file (C and C++ Build Settings)
//It will define the SVCALL macro to erase function declarations from the nordic header files
//It will then declare special test_ functions that are implemented in SystemTest.c

//This define erases all sd_ declarations from the nordic header files
#define SVCALL(number, return_type, signature)

#define NRF_UART_BAUDRATE_38400 (38400UL)
#define NRF_UART_BAUDRATE_115200 (115200UL)
#define NRF_UART_BAUDRATE_460800 (460800UL)
#define NRF_UART_BAUDRATE_1M (1000000UL)

#define APP_TIMER_PRESCALER       0 // Value of the RTC1 PRESCALER register

//Some config stuff
//#define ACTIVATE_LOGGING
#define ACTIVATE_STDIO 1


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <ble_gap.h>
#include <nrf_sdm.h>
#include <ble.h>



//################ Enable compilation of all modules for Simulator ###########################
#define ACTIVATE_CLC_MODULE 1
#define ACTIVATE_CLC_COMM 1
#define ACTIVATE_CLC_CONN 1
#define ACTIVATE_EINK_MODULE 1
#define ACTIVATE_ASSET_MODULE 1
#define ACTIVATE_VS_MODULE 1
#define ACTIVATE_WM_MODULE 0
#define ACTIVATE_INS 1

#define ACTIVATE_WATCHDOG 1

#define ACTIVATE_UNSECURE_MEMORY_READBACK 1

typedef struct Node Node;
typedef struct GlobalState GlobalState;

#define NRF_GPIOTE_POLARITY_TOGGLE 1
#define NRF_GPIOTE_POLARITY_HITOLO 2
#define NRF_GPIOTE_POLARITY_LOTOHI 3

typedef uint32_t nrf_drv_gpiote_pin_t;
typedef uint32_t nrf_gpiote_polarity_t;

typedef uint32_t nrf_uart_baudrate_t;

typedef struct {                                    /*!< FICR Structure                                                        */
  uint32_t  RESERVED0[4];
  uint32_t  CODEPAGESIZE;                      /*!< Code memory page size in bytes.                                       */
  uint32_t  CODESIZE;                          /*!< Code memory size in pages.                                            */
  uint32_t  RESERVED1[4];
  uint32_t  CLENR0;                            /*!< Length of code region 0 in bytes.                                     */
  uint32_t  PPFC;                              /*!< Pre-programmed factory code present.                                  */
  uint32_t  RESERVED2;
  uint32_t  NUMRAMBLOCK;                       /*!< Number of individualy controllable RAM blocks.                        */

  union {
    uint32_t  SIZERAMBLOCK[4];                 /*!< Deprecated array of size of RAM block in bytes. This name is
                                                         kept for backward compatinility purposes. Use SIZERAMBLOCKS
                                                          instead.                                                             */
    uint32_t  SIZERAMBLOCKS;                   /*!< Size of RAM blocks in bytes.                                          */
  };
  uint32_t  RESERVED3[5];
  uint32_t  CONFIGID;                          /*!< Configuration identifier.                                             */
  uint32_t  DEVICEID[2];                       /*!< Device identifier.                                                    */
  uint32_t  RESERVED4[6];
  uint32_t  ER[4];                             /*!< Encryption root.                                                      */
  uint32_t  IR[4];                             /*!< Identity root.                                                        */
  uint32_t  DEVICEADDRTYPE;                    /*!< Device address type.                                                  */
  uint32_t  DEVICEADDR[2];                     /*!< Device address.                                                       */
  uint32_t  OVERRIDEEN;                        /*!< Radio calibration override enable.                                    */
  uint32_t  NRF_1MBIT[5];                      /*!< Override values for the OVERRIDEn registers in RADIO for NRF_1Mbit
                                                         mode.                                                                 */
  uint32_t  RESERVED5[10];
  uint32_t  BLE_1MBIT[5];                      /*!< Override values for the OVERRIDEn registers in RADIO for BLE_1Mbit
                                                         mode.                                                                 */
} NRF_FICR_Type;

typedef struct {                                    /*!< UICR Structure                                                        */
   uint32_t  CLENR0;                            /*!< Length of code region 0.                                              */
   uint32_t  RBPCONF;                           /*!< Readback protection configuration.                                    */
   uint32_t  XTALFREQ;                          /*!< Reset value for CLOCK XTALFREQ register.                              */
  uint32_t  RESERVED0;
  uint32_t  FWID;                              /*!< Firmware ID.                                                          */

  union {
     uint32_t  NRFFW[15];                       /*!< Reserved for Nordic firmware design.                                  */
     uint32_t  BOOTLOADERADDR;                  /*!< Bootloader start address.                                             */
  };
   uint32_t  NRFHW[12];                         /*!< Reserved for Nordic hardware design.                                  */
   uint32_t  CUSTOMER[32];                      /*!< Reserved for customer.                                                */
} NRF_UICR_Type;

typedef struct {                                    /*!< GPIO Structure                                                        */
  uint32_t  RESERVED0[321];
   uint32_t  OUT;                               /*!< Write GPIO port.                                                      */
   uint32_t  OUTSET;                            /*!< Set individual bits in GPIO port.                                     */
   uint32_t  OUTCLR;                            /*!< Clear individual bits in GPIO port.                                   */
  uint32_t  IN;                                /*!< Read GPIO port.                                                       */
   uint32_t  DIR;                               /*!< Direction of GPIO pins.                                               */
   uint32_t  DIRSET;                            /*!< DIR set register.                                                     */
   uint32_t  DIRCLR;                            /*!< DIR clear register.                                                   */
  uint32_t  RESERVED1[120];
   uint32_t  PIN_CNF[32];                       /*!< Configuration of GPIO pins.                                           */
} NRF_GPIO_Type;


//UART Stuff
typedef enum
{
	NRF_GPIO_PIN_NOPULL   = 0,
	NRF_GPIO_PIN_PULLDOWN = 1,
	NRF_GPIO_PIN_PULLUP   = 3
} nrf_gpio_pin_pull_t;

//Sample rates for Lis2dh12 sensor
typedef enum {
	LIS2DH12_RATE_0 = 0,		/**< Power down */
	LIS2DH12_RATE_1 = 1 << 4,	/**< 1 Hz */
	LIS2DH12_RATE_10 = 2 << 4,	/**< 10 Hz*/
	LIS2DH12_RATE_25 = 3 << 4,
	LIS2DH12_RATE_50 = 4 << 4,
	LIS2DH12_RATE_100 = 5 << 4,
	LIS2DH12_RATE_200 = 6 << 4,
	LIS2DH12_RATE_400 = 7 << 4    /** 1k+ rates not implemented */
}lis2dh12_sample_rate_t;

typedef enum
{
	NRF_UART_HWFC_DISABLED = 0,
	NRF_UART_HWFC_ENABLED  = 1
} nrf_uart_hwfc_t;

typedef enum 
{
	SD_EVT_IRQn = 1,
	UART0_IRQn = 2,
} IRQn_Type;

typedef enum
{
	NRF_UART_EVENT_ERROR = 1, //Not the original value!
	NRF_UART_EVENT_RXDRDY = 2, //Not the original value!
	NRF_UART_EVENT_TXDRDY = 4, //Not the original value!
	NRF_UART_EVENT_RXTO = 8, //Not the original value!
	NRF_UART_INT_MASK_RXDRDY = 16, //Not the original value!
	NRF_UART_INT_MASK_ERROR = 32 //Not the original value!
} nrf_uart_event_t;

typedef enum
{
	NRF_UART_TASK_STARTRX = 1, //Not the original value!
	NRF_UART_TASK_STARTTX = 2
} nrf_uart_task_t;

typedef enum
{
	APP_IRQ_PRIORITY_LOW = 2 // Not the original value!
} app_irq_priority_t;

typedef struct 
{
	uint32_t ERRORSRC;
	uint32_t EVENTS_RXDRDY;
	uint32_t EVENTS_ERROR;
	uint32_t RXD;
	uint32_t TXD;
	uint32_t EVENTS_TXDRDY;
	//The original NRF implementation has some attributes here, but we dont care about those in the simulator.
} NRF_UART_Type;


typedef enum
{
	NRF_UART_PARITY_EXCLUDED = 1,//Not the original value!
} nrf_uart_parity_t;


#define NRF_UART_INT_MASK_RXTO 0 //Not the original value!
#define NRF_WDT_RR0 0
#define NRF_WDT_TASK_START 0
#define NRF_WDT_BEHAVIOUR_RUN_SLEEP 0
#define FM_WATCHDOG_TIMEOUT (32768UL * 60UL * 2UL)
#define FM_WATCHDOG_TIMEOUT_SAFE_BOOT (32768UL * 20UL)

void nrf_gpio_pin_set(uint32_t pin_number);
void nrf_gpio_pin_clear(uint32_t pin_number);
void nrf_gpio_pin_toggle(uint32_t pin_number);
void nrf_gpio_cfg_default(uint32_t pin_number);
void nrf_gpio_cfg_output(uint32_t pin_number);
void nrf_gpio_cfg_input(uint32_t pin_number, nrf_gpio_pin_pull_t pull_config);
void nrf_uart_baudrate_set(NRF_UART_Type *p_reg, nrf_uart_baudrate_t baudrate);
void nrf_uart_configure(NRF_UART_Type *p_reg, nrf_uart_parity_t parity, nrf_uart_hwfc_t hwfc);
void nrf_uart_txrx_pins_set(NRF_UART_Type *p_reg, uint32_t pseltxd, uint32_t pselrxd);
void nrf_uart_hwfc_pins_set(NRF_UART_Type *p_reg, uint32_t pselrts, uint32_t pselcts);
void nrf_uart_event_clear(NRF_UART_Type *p_reg, nrf_uart_event_t event);
void nrf_uart_int_enable(NRF_UART_Type *p_reg, uint32_t int_mask);
void nrf_uart_enable(NRF_UART_Type *p_reg);
void nrf_uart_task_trigger(NRF_UART_Type *p_reg, nrf_uart_task_t task);
bool nrf_uart_int_enable_check(NRF_UART_Type *p_reg, uint32_t int_mask);
bool nrf_uart_event_check(NRF_UART_Type *p_reg, nrf_uart_event_t event);
void nrf_uart_int_disable(NRF_UART_Type *p_reg, uint32_t int_mask);
void nrf_wdt_reload_request_set(int rr_register);
void nrf_wdt_task_trigger(int task);
void nrf_wdt_behaviour_set(int behaviour);
void nrf_wdt_reload_value_set(uint32_t reload_value);
void nrf_wdt_reload_request_enable(int rr_regist);
void nrf_delay_ms(uint32_t volatile number_of_ms);
uint8_t nrf_uart_rxd_get(NRF_UART_Type * p_reg);

//Unfortunatly can't return RebootReason, as this would create a circular dependency.
uint8_t ST_getRebootReason();

//Dummy implementation for bmg
#define BMG250_OK 0
#define BMG250_I2C_ADDR 1
#define BMG250_I2C_INTF 2
#define BMG250_GYRO_NORMAL_MODE 3
#define BMG250_RANGE_1000_DPS 4
#define BMG250_ODR_100HZ 5
#define BMG250_BW_NORMAL_MODE 6
#define BMG250_DATA_TIME_SEL 7
#define BMG250_FIFO_ALL_SETTING 8
#define BMG250_DISABLE 0
#define BMG250_FIFO_GYRO 9
#define BMG250_ENABLE 10
#define BMG250_PUSH_PULL 11
#define BMG250_ACTIVE_HIGH 12
#define BMG250_EDGE_TRIGGER 13
#define BMG250_INT_EN_ADDR 14
#define FIFO_WM_INT_ASSERTED 15

typedef uint32_t (*bmg250_com_fptr_t)(uint8_t dev_id, uint8_t reg_addr,
    uint8_t *data, uint8_t len);

typedef uint32_t (*bmg250_write_fptr_t)(uint8_t dev_id,
    uint8_t const *data, uint8_t len);

typedef void(*bmg250_delay_fptr_t)(uint32_t ms);

typedef struct
{
	uint8_t *data;
	uint16_t length;
	uint8_t fifo_time_enable;
	uint8_t fifo_header_enable;
	uint8_t fifo_data_enable;
	uint16_t gyro_byte_start_idx;

	uint32_t sensor_time;
	uint8_t skipped_frame_count;
	uint8_t fifo_down;
} bmg250_fifo_frame;

typedef struct
{
	uint32_t dev_id;
	uint32_t interface;
	uint32_t(*read)(uint8_t, uint8_t, uint8_t *, uint8_t);
	uint32_t(*write)(uint8_t, uint8_t const *, uint8_t);
	void(*delay_ms)(uint32_t volatile);
	bmg250_fifo_frame *fifo;
	uint8_t power_mode;
} bmg250_dev;


typedef struct
{
	uint8_t odr;
	uint8_t range;
	uint8_t bw;
} bmg250_cfg;

typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
	uint32_t sensortime;
} bmg250_sensor_data;

enum bmg250_int_channel {
	/*! interrupt Channel 1 */
	BMG250_INT_CHANNEL_1,
	/*! interrupt Channel 2 */
	BMG250_INT_CHANNEL_2
};

enum bmg250_int_types {
	/*! data ready interrupt  */
	BMG250_DATA_RDY_INT,
	/*! fifo full interrupt */
	BMG250_FIFO_FULL_INT,
	/*! fifo watermark interrupt */
	BMG250_FIFO_WATERMARK_INT
};

typedef struct {
	uint8_t output_en;
	uint8_t output_mode;
	uint8_t output_type;
	uint8_t edge_ctrl;
	uint8_t input_en;
} bmg250_int_pin_settg;

typedef struct{
	/*! Interrupt channel */
	enum bmg250_int_channel int_channel;
	/*! Select Interrupt */
	enum bmg250_int_types int_type;
	/*! FIFO FULL INT 1-enable, 0-disable */
	bmg250_int_pin_settg int_pin_settg;
	uint8_t fifo_full_int_en;
	/*! FIFO WTM INT 1-enable, 0-disable */
	uint8_t fifo_wtm_int_en;
} bmg250_int_settg;


typedef struct
{
	uint8_t number_of_fifo_sample;
	bool fifo_is_empty;
	bool fifo_watermark_interrupt;
}fifo_status;


int8_t bmg250_set_fifo_config(uint8_t config, uint8_t enable, const bmg250_dev *dev);
int8_t bmg250_get_fifo_data(const bmg250_dev *dev);
int8_t bmg250_extract_gyro(bmg250_sensor_data *gyro_data, uint8_t *data_length, const bmg250_dev *dev);
int8_t bmg250_set_fifo_wm(uint8_t wm_frame_count, uint16_t *fifo_length, const bmg250_dev *dev);
uint32_t bmg250_init(bmg250_dev* data);
uint8_t bmg250_set_power_mode(const bmg250_dev *dev); 
uint8_t bmg250_get_sensor_settings(bmg250_cfg *gyro_cfg, const bmg250_dev *dev);
uint32_t bmg250_set_sensor_settings(const bmg250_cfg *gyro_cfg, const bmg250_dev *dev);
int8_t bmg250_get_sensor_data(uint8_t data_sel, bmg250_sensor_data *gyro, const bmg250_dev *dev);
int8_t bmg250_set_fifo_flush(const bmg250_dev *dev);
int8_t bmg250_set_int_config(const bmg250_int_settg *int_config, const bmg250_dev *dev);
uint32_t bmg250_set_regs(uint8_t reg_addr, uint8_t *data, uint16_t len, const bmg250_dev *dev);
int8_t bmg250_get_int_status(uint8_t *int_status, const bmg250_dev *dev);


typedef void(*nrf_drv_gpiote_evt_handler_t)(nrf_drv_gpiote_pin_t, nrf_gpiote_polarity_t);

#define TLV493D_OP_MODE_MCM 1

typedef struct
{
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t temp;
}TLV493D_data_frame_t;

typedef struct
{
	int32_t x;
	int32_t y;
	int32_t z;
} LisSensor;
typedef struct
{
	LisSensor sensor;
}lis2dh12_sensor_buffer_t;

typedef struct
{
	nrf_gpiote_polarity_t sense;
	nrf_gpio_pin_pull_t   pull;
	bool                  is_watcher;
	bool                  hi_accuracy;
} nrf_drv_gpiote_in_config_t;

uint32_t TLV493D_A1B6_initialise(int32_t dataPin);
void TLV493D_A1B6_set_operation_mode(void* ptr, uint32_t mode);
uint32_t TLV493D_A1B6_read_frame(void* ptr, TLV493D_data_frame_t *out);

#define LIS2DH12_INTERFACE_SPI 1
#define LIS2DH12_INTERFACE_TWI 2
#define LIS2DH12_MODE_BYPASS 3
#define LIS2DH12_SCALE2G 4
#define LIS2DH12_RES12BIT 5
#define LIS2DH12_ODR_MASK_100HZ 6
#define LIS2DH12_HIGH_RESOLUTION_MODE 7
#define LIS2DH12_ODR_MASK_10HZ 8
#define LIS2DH12_I1_AOI 9
#define LIS2DH12_I1_WTM 10
#define LIS2DH12_MODE_STREAM 12
#define LIS2DH12_ZHIE_MASK 1
#define LIS2DH12_YHIE_MASK 2
#define LIS2DH12_XHIE_MASK 4
#define LIS2DH12_HPCF_BW_0_5HZ_MASK 0x20
#define LIS2DH12_FDS_MASK 0x08
#define LIS2DH12_HPIS1_MASK 0x01
#define LIS2DH12_RET_OK 0
typedef uint32_t lis2dh12_ret_t;
uint8_t lis2dh12_init(uint32_t interface, int32_t slaveSelectPin);
void lis2dh12_reset();
void lis2dh12_enable();
void lis2dh12_set_fifo_mode(uint32_t);
void lis2dh12_set_scale(uint32_t);
void lis2dh12_set_resolution(uint32_t);
void lis2dh12_set_data_rate(uint32_t);
void lis2dh12_set_mode(uint32_t);
void lis2dh12_write_reg2(uint32_t);
void lis2dh12_set_sleep_to_wake_threshold(float);
uint32_t lis2dh12_set_interrupts(uint8_t, uint8_t);
lis2dh12_ret_t lis2dh12_set_fifo_watermark(size_t count);
lis2dh12_ret_t lis2dh12_set_sample_rate(lis2dh12_sample_rate_t sample_rate);
uint8_t lis2dh12_set_latch();
void lis2dh12_set_hlactive();
void lis2dh12_set_sleep_to_wake_duration(uint32_t, uint32_t, float);
uint32_t lis2dh12_set_int1_ths(float);
uint32_t lis2dh12_set_int1_duration(uint32_t);
uint32_t lis2dh12_set_int1_cfg(uint32_t);
lis2dh12_ret_t lis2dh12_read_int1_src(uint8_t *ctrl, uint32_t size);
uint32_t lis2dh12_reset_act_dur();
uint32_t lis2dh12_reset_act_ths();
void lis2dh12_read_samples(lis2dh12_sensor_buffer_t* buffer, uint32_t count);
uint32_t nrf_drv_gpiote_in_init(nrf_drv_gpiote_pin_t pin, nrf_drv_gpiote_in_config_t const * p_config, nrf_drv_gpiote_evt_handler_t evt_handler);
uint32_t nrf_drv_gpiote_in_event_enable(nrf_drv_gpiote_pin_t pin, bool);
lis2dh12_ret_t lis2dh12_get_fifo_sample_number(size_t* count);
lis2dh12_ret_t lis2dh12_get_fifo_status(fifo_status* status);

#define BME280_RET_OK 0
#define BME280_MODE_SLEEP 1
#define BME280_OVERSAMPLING_2 2
#define BME280_IIR_8 3
#define BME280_STANDBY_1000_MS 4
#define BME280_MODE_NORMAL 5
#define BME280REG_CTRL_MEAS 6
typedef uint32_t BME280_Ret;

uint32_t bme280_init(int32_t slaveSelectPin);
uint32_t bme280_set_mode_assert(uint32_t);
uint32_t bme280_set_oversampling_press(uint32_t);
uint32_t bme280_set_oversampling_temp(uint32_t);
uint32_t bme280_set_oversampling_hum(uint32_t);
uint32_t bme280_set_iir(uint32_t);
uint32_t bme280_set_interval(uint32_t);
uint32_t bme280_read_reg(uint32_t);
uint32_t bme280_read_measurements();
uint32_t bme280_get_pressure();
uint32_t bme280_get_temperature();
uint32_t bme280_get_humidity();

#define SIM_MAX_FLASH_SIZE (4096 * 128)

//We need to redefine the macro that calculates the sizes of MasterBootRecord, Softddevice,...

#define UNITS_TO_MSEC(TIME, RESOLUTION) (((TIME) * ((u32)RESOLUTION)) / 1000)

#define BOOTLOADER_ADDRESS (NRF_UICR->BOOTLOADERADDR)

extern int globalBreakCounter; 

//We keep a pointer to our GlobalState, this state contains the whole state of a node as known to FruityMesh
extern GlobalState* simGlobalStatePtr;
#define GS (simGlobalStatePtr)

//We keep a number of pointers to hardware peripherals so that our FruityMesh implementation
//does not have to include the simulator. It will access all hardware using these pointers and we can
//therefore redirect all access
extern NRF_FICR_Type* simFicrPtr;
extern NRF_UICR_Type* simUicrPtr;
extern NRF_GPIO_Type* simGpioPtr;
extern NRF_UART_Type* simUartPtr;
extern uint8_t* simFlashPtr;
#define NRF_FICR (simFicrPtr)
#define NRF_UICR (simUicrPtr)
#define NRF_GPIO (simGpioPtr)
#define NRF_UART0 (simUartPtr)

//The flash region start address points to the beginning of the flash memory. All address calculations must
//use the correct addresses including the start address of the flash space.
//The pages however are always counted from the beginning of the flash memory.
#define FLASH_REGION_START_ADDRESS ((u32)simFlashPtr)

//Used to collect statistic counts in the simulator, a hashmap is used to count all calls to this function under the given key
#define SIMSTATCOUNT(key) sim_collect_statistic_count(key)
#define SIMSTATAVG(key, value) sim_collect_statistic_avg(key, value)


uint32_t sd_ble_gap_adv_data_set(uint8_t const *p_data, uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen);
uint32_t sd_ble_gap_adv_stop();
uint32_t sd_ble_gap_adv_start(ble_gap_adv_params_t const *p_adv_params);
uint32_t sd_ble_gap_device_name_set(ble_gap_conn_sec_mode_t const *p_write_perm, uint8_t const *p_dev_name, uint16_t len);
uint32_t sd_ble_gap_appearance_set(uint16_t appearance);
uint32_t sd_ble_gap_ppcp_set(ble_gap_conn_params_t const *p_conn_params);
uint32_t sd_ble_gap_connect(ble_gap_addr_t const *p_peer_addr, ble_gap_scan_params_t const *p_scan_params, ble_gap_conn_params_t const *p_conn_params);
uint32_t sd_ble_gap_connect_cancel();
uint32_t sd_ble_gap_disconnect(uint16_t conn_handle, uint8_t hci_status_code);
uint32_t sd_ble_gap_sec_info_reply(uint16_t conn_handle, ble_gap_enc_info_t const *p_enc_info, ble_gap_irk_t const *p_id_info, ble_gap_sign_info_t const *p_sign_info);
uint32_t sd_ble_gap_encrypt(uint16_t conn_handle, ble_gap_master_id_t const *p_master_id, ble_gap_enc_info_t const *p_enc_info);
uint32_t sd_ble_gap_conn_param_update(uint16_t conn_handle, ble_gap_conn_params_t const *p_conn_params);
uint32_t sd_ble_uuid_vs_add(ble_uuid128_t const *p_vs_uuid, uint8_t *p_uuid_type);
uint32_t sd_ble_gatts_service_add(uint8_t type, ble_uuid_t const *p_uuid, uint16_t *p_handle);
uint32_t sd_ble_gatts_characteristic_add(uint16_t service_handle, ble_gatts_char_md_t const *p_char_md, ble_gatts_attr_t const *p_attr_char_value, ble_gatts_char_handles_t *p_handles);
uint32_t sd_ble_gatts_sys_attr_set(uint16_t conn_handle, uint8_t const *p_sys_attr_data, uint16_t len, uint32_t flags);
uint32_t sd_ble_gattc_primary_services_discover(uint16_t conn_handle, uint16_t start_handle, ble_uuid_t const *p_srvc_uuid);
uint32_t sd_ble_gattc_characteristics_discover(uint16_t conn_handle, ble_gattc_handle_range_t const *p_handle_range);
uint32_t sd_ble_gattc_write(uint16_t conn_handle, ble_gattc_write_params_t const *p_write_params);
uint32_t sd_ble_gap_scan_stop();
uint32_t sd_ble_gap_scan_start(ble_gap_scan_params_t const *p_scan_params);
uint32_t sd_ble_tx_packet_count_get(uint16_t conn_handle, uint8_t *p_count);
uint32_t sd_ble_gap_disconnect(uint16_t conn_handle, uint8_t hci_status_code);
uint32_t sd_ble_gap_address_set(uint8_t addr_cycle_mode, ble_gap_addr_t const *p_addr);
uint32_t sd_ble_gap_address_get(ble_gap_addr_t *p_addr);
uint32_t sd_nvic_SystemReset();
uint32_t sd_ble_gap_rssi_start(uint16_t conn_handle, uint8_t threshold_dbm, uint8_t skip_count);
uint32_t sd_ble_gap_rssi_stop(uint16_t conn_handle);
uint32_t sd_power_dcdc_mode_set(uint8_t dcdc_mode);
uint32_t sd_power_mode_set(uint8_t power_mode);
uint32_t sd_flash_page_erase(uint32_t page_number);
uint32_t sd_flash_write(uint32_t * const p_dst, uint32_t const * const p_src, uint32_t size);
uint32_t sd_rand_application_vector_get(uint8_t * p_buff, uint8_t length);
uint32_t sd_ble_evt_get(uint8_t *p_dest, uint16_t *p_len);
uint32_t sd_app_evt_wait();
uint32_t sd_nvic_ClearPendingIRQ(IRQn_Type IRQn);
uint32_t sd_ble_enable(ble_enable_params_t * p_ble_enable_params, uint32_t * p_app_ram_base);
uint32_t sd_ble_gap_tx_power_set(int8_t tx_power);
uint32_t sd_nvic_EnableIRQ(IRQn_Type IRQn);
uint32_t sd_nvic_SetPriority(IRQn_Type IRQn, uint32_t priority);
uint32_t sd_evt_get(uint32_t* evt_id);
uint32_t sd_ble_gatts_hvx(uint16_t conn_handle, ble_gatts_hvx_params_t const *p_hvx_params);
uint32_t sd_ecb_block_encrypt(nrf_ecb_hal_data_t * p_ecb_data);
uint32_t sd_ble_opt_set(uint32_t opt_id, ble_opt_t const *p_opt);
uint32_t sd_power_reset_reason_clr(uint32_t p);
uint32_t sd_ble_gattc_descriptors_discover(uint16_t conn_handle, ble_gattc_handle_range_t const *p_handle_range);

uint32_t NVIC_SystemReset();

uint32_t app_timer_cnt_get(uint32_t* time);
uint32_t app_timer_cnt_diff_compute(uint32_t nowTime, uint32_t previousTime, uint32_t* passedTime);
typedef void (*ble_radio_notification_evt_handler_t) (bool radio_active);


void sim_collect_statistic_count(const char* key);
void sim_collect_statistic_avg(const char* key, int value);
void sim_print_statistics();

uint32_t sim_get_stack_type();

//Configuration
struct ModuleConfiguration;
void setBoardConfiguration_CherrySim(struct BoardConfiguration* config);
void setFeaturesetConfiguration_CherrySim(struct ModuleConfiguration* config, void* module);
uint32_t initializeModules_CherrySim(bool createModule);

//Helpers
bool isEmpty(const uint8_t* data, uint32_t length);

#ifdef __cplusplus
}
#endif


//Enable all necessary modules and parts
#define ACTIVATE_LOGGING 1

#define ACTIVATE_CLC_MODULE 1


#endif
#endif /* SYSTEMTEST_H_ */
