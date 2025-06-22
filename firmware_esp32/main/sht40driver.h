/*
 * sht40driver.h
 *
 *  Created on: 25 nov 2024
 *      Author: Sigma
 *
 *  WARNING: The sensor SHT40 does not support clock-stretching
 *  		 Returns NACk to read headers if it is busy with a measurement
 *  		 or heating
 */

#ifndef MAIN_SHT40DRIVER_H_
#define MAIN_SHT40DRIVER_H_

#include <esp_types.h>

#define SHT40_ADDRESS	0x44

#define SHT40_POWER_UP_TIME_MS			 1u
#define SHT40_SOFT_RESET_TIME_MS		 1u
#define SHT40_MEASURE_TIME_LOW_MS		 3u
#define SHT40_MEASURE_TIME_MED_MS		 5u
#define SHT40_MEASURE_TIME_HIGH_MS		 10u
#define SHT40_GENERAL_TIME_MS			 10u
#define SHT40_I2CBUS_WAIT_TIME_MS		 500u
#define SHT40_MEASURE_POLLING_TIME_MS	 1000u	// 1 second between measures
#define SHT40_EMPTY_TIME_MS				 0u

// SHT40 Commands
#define SHT40_NUMBER_OF_CMD									6
#define SHT40_CMD_TEMP_HUMIDITY_HIGH_PRECISION				0xFD
#define SHT40_CMD_TEMP_HUMIDITY_MEDIUM_PRECISION			0xF6
#define SHT40_CMD_TEMP_HUMIDITY_LOW_PRECISION				0xE0
#define SHT40_CRC_LENGTH_BYTES								1u
#define SHT40_CMD_SERIAL_NUMBER								0x89
#define SHT40_CMD_SOFT_RESET								0x94
#define SHT40_CMD_GENERAL_CALL_RESET						0x06
#define SHT40_GENERAL_CALL_ADDRESS							0x00

// Byte sizes
#define SHT40_WORD_LENGTH_BYTES			2u
#define SHT40_CRC_LENGTH_BYTES			1u
#define SHT40_FRAME_LENGTH_BYTES	(2u * (SHT40_WORD_LENGTH_BYTES + SHT40_CRC_LENGTH_BYTES))
#define SHT40_FORMULA_DENOMINATOR 		65535.0f


//*****************************************************************************
//      SGP40 API
//*****************************************************************************

#define sht40_quotient(signal_ticks) ( (((float)signal_ticks)) / SHT40_FORMULA_DENOMINATOR )

extern esp_err_t sht40_i2c_master_init(i2c_port_t i2c_master_port);
extern esp_err_t sht40_i2c_get_measure_high_precision(uint16_t *temp, uint16_t *humidity);
extern esp_err_t sht40_i2c_get_measure_medium_precision(uint16_t *temp, uint16_t *humidity);
extern esp_err_t sht40_i2c_get_measure_low_precision(uint16_t *temp, uint16_t *humidity);
extern uint16_t sht40_temperature_signal_to_celsius(uint16_t temperature_ticks);
extern uint16_t sht40_humidity_signal_to_RH(uint16_t humidity_ticks);
extern esp_err_t sht40_i2c_soft_reset(void);
extern esp_err_t sht40_i2c_general_call_reset(void);
extern esp_err_t sht40_i2c_serial_number(uint32_t *serial_number);


#endif /* MAIN_SHT40DRIVER_H_ */
