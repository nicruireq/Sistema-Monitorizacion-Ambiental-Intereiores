/*
 * sgp40driver.cpp
 *
 *  Created on: 26 oct 2024
 *      Author: Nicolas Ruiz Requejo
 */


#include "sgp40driver.h"
#include "i2c_master.h"
#include "sensirion_crc.h"

#include "esp_check.h"


static const char *TAG = "SGP40";

static i2c_port_t sgp40_i2c_port;

esp_err_t sgp40_i2c_master_init(i2c_port_t i2c_master_port)
{
	sgp40_i2c_port = i2c_master_port;

	return ESP_OK;
}

void sgp40_compensation_t_init(sgp40_compensation_t *obj, const uint16_t temperature_celsius,
								const uint16_t humidity_rh)
{
	uint16_t humidity_ticks = RH_to_ticks(humidity_rh);
	uint16_t temperature_ticks = Celsius_to_ticks(temperature_celsius);

	obj->humidity_msb = (uint8_t)(humidity_ticks >> 8);
	obj->humidity_lsb = (uint8_t)(humidity_ticks & 0x00FF);

	uint8_t humidity_arr[RAW_MEASURE_SIZE] = {obj->humidity_msb, obj->humidity_lsb};
	obj->humidity_CRC = sensirion_i2c_generate_crc(humidity_arr, RAW_MEASURE_SIZE);

	obj->temperature_msb = (uint8_t)(temperature_ticks >> 8);
	obj->temperature_lsb = (uint8_t)(temperature_ticks & 0x00FF);

	uint8_t temp_arr[RAW_MEASURE_SIZE] = {obj->temperature_msb, obj->temperature_lsb};
	obj->temperature_CRC = sensirion_i2c_generate_crc(temp_arr, RAW_MEASURE_SIZE);
}

esp_err_t sgp40_i2c_get_raw_measure(uint16_t *raw_measure,
								    SGP40_COMPENSATION compensation,
									sgp40_compensation_t *comp_data)
{
	uint8_t humidity[HUMIDITY_BUFFER_SIZE];
	uint8_t temperature[TEMPERATURE_BUFFER_SIZE];
	esp_err_t transactionResult = ESP_OK;
	i2c_cmd_handle_t cmdRawMeasure;

	if ((compensation == SGP40_COMP_ON) && (comp_data != NULL))
	{
		// 0 is MSB, 1 is LSB
		humidity[0] = comp_data->humidity_msb;
		humidity[1] = comp_data->humidity_lsb;
		humidity[2] = comp_data->humidity_CRC;
		temperature[0] = comp_data->temperature_msb;
		temperature[1] = comp_data->temperature_lsb;
		temperature[2] = comp_data->temperature_CRC;
	}
	else
	{
		// 0 is MSB, 1 is LSB
		humidity[0] = SGP40_DEFAULT_HUMIDITY_0;
		humidity[1] = SGP40_DEFAULT_HUMIDITY_1;
		humidity[2] = (uint8_t)SGP40_DEFAULT_HUMIDITY_CRC;
		temperature[0] = SGP40_DEFAULT_TEMPERATURE_0;
		temperature[1] = SGP40_DEFAULT_TEMPERATURE_1;
		temperature[2] = (uint8_t)SGP40_DEFAULT_TEMPERATURE_CRC;
	}

	cmdRawMeasure = i2c_cmd_link_create();

	// start condition
	i2c_master_start(cmdRawMeasure);

	// send address and write op with ACK
	i2c_master_write_byte(cmdRawMeasure, (SGP40_ADDRESS << 1) | I2C_MASTER_WRITE,
			I2C_ACK);
	// send byte 0 and 1 of raw measurement command
	i2c_master_write_byte(cmdRawMeasure, SGP40_CMD_MEASURE_RAW_0, I2C_ACK);
	i2c_master_write_byte(cmdRawMeasure, SGP40_CMD_MEASURE_RAW_1, I2C_ACK);
	// send humidity+CRC word
	i2c_master_write(cmdRawMeasure, humidity, HUMIDITY_BUFFER_SIZE, I2C_ACK);
	// send temperature+CRC word
	i2c_master_write(cmdRawMeasure, temperature, TEMPERATURE_BUFFER_SIZE, I2C_ACK);
	i2c_master_stop(cmdRawMeasure);
	transactionResult = i2c_master_cmd_begin(sgp40_i2c_port, cmdRawMeasure,
			SGP40_WAIT_TIME_MS / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmdRawMeasure);

	// wait the maximum time needed by sgp40 to provide the raw measure
	TickType_t xLastWakeTime = xTaskGetTickCount();
	TickType_t antes = xTaskGetTickCount();

	ESP_LOGI(TAG, "SGP40 Ticks espera: %lu, cuantos ms es un tick: %lu",
			(SGP40_TIME_UNTIL_MEASURE_AVAILABLE_MS / portTICK_PERIOD_MS),
			portTICK_PERIOD_MS);

	TickType_t waiting_ticks = (SGP40_TIME_UNTIL_MEASURE_AVAILABLE_MS / portTICK_PERIOD_MS);
	// Importante alargar el tiempo de espera para leer los datos del sensor
	// si el tiempo de un tick del sistema es mayor al tiempo de espera del datasheet
	if (SGP40_TIME_UNTIL_MEASURE_AVAILABLE_MS <= portTICK_PERIOD_MS)
	{
		waiting_ticks = 2;
	}

	vTaskDelayUntil(&xLastWakeTime, waiting_ticks);

	ESP_LOGI(TAG, "SGP40 al despertar han pasado: %lu ms", ((xLastWakeTime-antes) * portTICK_PERIOD_MS));


	if (transactionResult == ESP_OK)
	{
		// retrieve the measure
		cmdRawMeasure = i2c_cmd_link_create();
		i2c_master_start(cmdRawMeasure);
		// send address and read operation
		i2c_master_write_byte(cmdRawMeasure, (SGP40_ADDRESS << 1) | I2C_MASTER_READ,
					I2C_ACK);
		uint8_t measureBuffer[RAW_MEASURE_SIZE];
		i2c_master_read(cmdRawMeasure, measureBuffer, RAW_MEASURE_SIZE,
				I2C_MASTER_ACK);
		uint8_t readMeasureCRC = 0;
		i2c_master_read_byte(cmdRawMeasure, &readMeasureCRC, I2C_MASTER_LAST_NACK);
		i2c_master_stop(cmdRawMeasure);
		transactionResult = i2c_master_cmd_begin(sgp40_i2c_port, cmdRawMeasure,
				SGP40_WAIT_TIME_MS / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmdRawMeasure);

		if (transactionResult == ESP_OK)
		{
			if ((sensirion_i2c_check_crc(measureBuffer, RAW_MEASURE_SIZE, readMeasureCRC) == NO_ERROR))
			{
				*raw_measure = (measureBuffer[0] << 8) | measureBuffer[1];
			}
			else
			{
				transactionResult = ESP_ERR_INVALID_CRC;
			}
		}
		else
		{
			ESP_RETURN_ON_ERROR(transactionResult, TAG, "Lectura de datos de SGP40 fallida");
		}
	}
	else
	{
		ESP_RETURN_ON_ERROR(transactionResult, TAG, "Solicitud de medicion a SGP40 fallida");
	}

	/* Solo para test*/
	/* vTaskDelay(SGP40_RAW_MEASURE_POLLING_TIME_MS / portTICK_PERIOD_MS); */

	return (transactionResult);
}

esp_err_t sgp40_i2c_soft_reset(void)
{
	// tbd
	return ESP_FAIL;
}

esp_err_t sgp40_i2c_measure_test(uint16_t *test_result)
{
	// tbd
	return ESP_FAIL;
}
