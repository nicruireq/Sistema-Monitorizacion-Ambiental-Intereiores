/*
 * sgp40driver.h
 *
 *  Created on: 26 oct 2024
 *      Author: Nicolas Ruiz Requejo
 */

#ifndef MAIN_SGP40DRIVER_H_
#define MAIN_SGP40DRIVER_H_

#include <esp_types.h>
#include "driver/i2c.h"
#include "esp_err.h"

// CONSTANTS
#define SGP40_ADDRESS         					0x59
#define SGP40_POWER_UP_TIME_MS   				1	// the max power-up time is 0.6ms
#define SGP40_TIME_UNTIL_MEASURE_AVAILABLE_MS	30
#define SGP40_TIME_UNTIL_TEST_AVAILABLE_MS      250
#define SGP40_RAW_MEASURE_POLLING_TIME_MS		1000	// 1 second between measures
#define SGP40_WAIT_TIME_MS						500
#define SGP40_HEATER_OFF_TIME_MS				1
#define SGP40_DEFAULT_HUMIDITY_0				0x80
#define SGP40_DEFAULT_HUMIDITY_1				0x00
#define SGP40_DEFAULT_HUMIDITY_CRC				0xA2
#define SGP40_DEFAULT_HUMIDITY_RH				50
#define SGP40_DEFAULT_TEMPERATURE_0				0x66
#define SGP40_DEFAULT_TEMPERATURE_1				0x66
#define SGP40_DEFAULT_TEMPERATURE_CRC			0x93
#define SGP40_DEFAULT_TEMPERATURE_CELSIUS		25

// COMMANDS
#define SGP40_CMD_MEASURE_RAW_0		0x26
#define SGP40_CMD_MEASURE_RAW_1		0x0F
#define SGP40_CMD_MEASURE_TEST_0	0x28
#define SGP40_CMD_MEASURE_TEST_1	0x0E
#define SGP40_CMD_HEATER_OFF_0		0x36
#define SGP40_CMD_HEATER_OFF_1		0x15
#define SGP40_CMD_SOFT_RESET_0		0x00
#define SGP40_CMD_SOFT_RESET_1		0x06

// Utils: bytes number
#define HUMIDITY_BUFFER_SIZE	3
#define TEMPERATURE_BUFFER_SIZE	3
#define RAW_MEASURE_SIZE	2

// Utils: conversion macros
#define RH_to_ticks(ticks)			( ((ticks) * 65535) / 100 )
#define Celsius_to_ticks(ticks)	( (((ticks) + 45) * 65535) / 175 )

typedef enum
{
	SGP40_COMP_OFF,
	SGP40_COMP_ON
} SGP40_COMPENSATION;

typedef struct sgp40_compensation
{
	uint8_t humidity_msb;
	uint8_t humidity_lsb;
	uint8_t humidity_CRC;
	uint8_t temperature_msb;
	uint8_t temperature_lsb;
	uint8_t temperature_CRC;
} sgp40_compensation_t;

//*****************************************************************************
//      SGP40 API
//*****************************************************************************

/**
 * @brief Inicializa una estructura de tipo sgp40_compensation_t con valores
 * 		  de temperatura y humedad convertidos en ticks segun
 * 		  la tabla 10 del datasheet del SGP40. Y calcula sus CRC.
 *
 * @param[in]	obj	puntero a estructura sgp40_compensation_t
 * @param[in]	temperature	temperatura en grados celsius
 * @param[in]	humidity	humedad en % de humedad relativa
 */
extern void sgp40_compensation_t_init(sgp40_compensation_t *obj, const uint16_t temperature_celsius,
										const uint16_t humidity_rh);

extern esp_err_t sgp40_i2c_master_init(i2c_port_t i2c_master_port);
extern esp_err_t sgp40_i2c_get_raw_measure(uint16_t *raw_measure,
										   SGP40_COMPENSATION compensation,
										   sgp40_compensation_t *comp_data);
extern esp_err_t sgp40_i2c_soft_reset(void);
extern esp_err_t sgp40_i2c_measure_test(uint16_t *test_result);


#endif /* MAIN_SGP40DRIVER_H_ */
