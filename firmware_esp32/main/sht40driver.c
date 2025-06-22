/*
 * sht40driver.c
 *
 *  Created on: 25 nov 2024
 *      Author: Sigma
 */

#include "i2c_master.h"
#include "sht40driver.h"
#include "sensirion_crc.h"

#include "esp_log.h"
#include "esp_check.h"


static const char *TAG = "SHT40";

static i2c_port_t sht40_i2c_port;

esp_err_t sht40_i2c_master_init(i2c_port_t i2c_master_port)
{
	sht40_i2c_port = i2c_master_port;

	return ESP_OK;
}

static esp_err_t sht40_i2c_read_frame(uint8_t sht40_cmd_id, uint16_t *data_word1, uint16_t *data_word2,
										TickType_t time_to_read)
{
	i2c_cmd_handle_t i2c_cmd_handle;
	esp_err_t transaction_result = ESP_FAIL;
	uint8_t frame_buffer[SHT40_FRAME_LENGTH_BYTES];

	i2c_cmd_handle = i2c_cmd_link_create();
	// start condition
	i2c_master_start(i2c_cmd_handle);

	// send address and write operation with ACK
	i2c_master_write_byte(i2c_cmd_handle, (SHT40_ADDRESS << 1) | I2C_MASTER_WRITE,
			I2C_ACK);
	// send byte of command
	i2c_master_write_byte(i2c_cmd_handle, sht40_cmd_id, I2C_ACK);
	i2c_master_stop(i2c_cmd_handle);

	transaction_result = i2c_master_cmd_begin(sht40_i2c_port, i2c_cmd_handle,
			SHT40_I2CBUS_WAIT_TIME_MS / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(i2c_cmd_handle);

	// wait the maximum time needed by sht40 to provide the result data frame
	TickType_t xLastWakeTime = xTaskGetTickCount();
	TickType_t antes = xTaskGetTickCount();

	ESP_LOGI(TAG, "SHT40 Ticks espera: %lu, cuantos ms es un tick: %lu",
			(time_to_read / portTICK_PERIOD_MS), portTICK_PERIOD_MS);

	//vTaskDelay(time_to_read / portTICK_PERIOD_MS);
	TickType_t waiting_ticks = (time_to_read / portTICK_PERIOD_MS);
	// Importante alargar el tiempo de espera para leer los datos del sensor
	// si el tiempo de un tick del sistema es mayor al tiempo de espera del datasheet
	if (time_to_read <= portTICK_PERIOD_MS)
	{
		waiting_ticks = 2;
	}

	vTaskDelayUntil(&xLastWakeTime, waiting_ticks);

	ESP_LOGI(TAG, "SHT40 al despertar han pasado: %lu ms", ((xLastWakeTime-antes) * portTICK_PERIOD_MS));

	if (transaction_result == ESP_OK)
	{
		// retrieve the measure
		i2c_cmd_handle = i2c_cmd_link_create();
		i2c_master_start(i2c_cmd_handle);
		// send address and read operation
		i2c_master_write_byte(i2c_cmd_handle, (SHT40_ADDRESS << 1) | I2C_MASTER_READ,
				I2C_ACK);
		i2c_master_read(i2c_cmd_handle, frame_buffer, SHT40_FRAME_LENGTH_BYTES,
				I2C_MASTER_LAST_NACK);
		i2c_master_stop(i2c_cmd_handle);

		transaction_result = i2c_master_cmd_begin(sht40_i2c_port, i2c_cmd_handle,
					SHT40_I2CBUS_WAIT_TIME_MS / portTICK_PERIOD_MS);
			i2c_cmd_link_delete(i2c_cmd_handle);

		if (transaction_result == ESP_OK)
		{
			uint8_t crc_first_frame = frame_buffer[2];
			uint8_t crc_second_frame = frame_buffer[5];

			if ((sensirion_i2c_check_crc(frame_buffer, SHT40_WORD_LENGTH_BYTES, crc_first_frame) != NO_ERROR)
					|| (sensirion_i2c_check_crc(&frame_buffer[3], SHT40_WORD_LENGTH_BYTES, crc_second_frame) != NO_ERROR))
			{
				transaction_result = ESP_ERR_INVALID_CRC;
			}

			// devolver datos leidos del sensor solo si ambos son validos,
			// si alguno se transmitio con errores se devolvera error y
			// no se devuelven las lecturas
			if (transaction_result != ESP_ERR_INVALID_CRC)
			{
				*data_word1 = (frame_buffer[0] << 8) | frame_buffer[1];
				*data_word2 = (frame_buffer[3] << 8) | frame_buffer[4];
			}
		}
		else
		{
			ESP_RETURN_ON_ERROR(transaction_result, TAG, "Lectura de datos de SHT40 fallida: %s", esp_err_to_name(transaction_result));
		}
	}
	else
	{
		ESP_RETURN_ON_ERROR(transaction_result, TAG, "solicitud I2C a SHT40 fallida: %s", esp_err_to_name(transaction_result));
	}

	/* Solo para test: */
	/*vTaskDelay(SHT40_MEASURE_POLLING_TIME_MS / portTICK_PERIOD_MS);*/

	return (transaction_result);
}

esp_err_t sht40_i2c_get_measure_high_precision(uint16_t *temp, uint16_t *humidity)
{
	return (sht40_i2c_read_frame(SHT40_CMD_TEMP_HUMIDITY_HIGH_PRECISION,
										temp, humidity, SHT40_MEASURE_TIME_HIGH_MS));
}

uint16_t sht40_temperature_signal_to_celsius(uint16_t temperature_ticks)
{
	return ((uint16_t)(-45.0f + 175.0f * sht40_quotient(temperature_ticks)));
}

uint16_t sht40_humidity_signal_to_RH(uint16_t humidity_ticks)
{
	return ((uint16_t)(-6.0f + 125.0f * sht40_quotient(humidity_ticks)));
}

esp_err_t sht40_i2c_get_measure_medium_precision(uint16_t *temp, uint16_t *humidity)
{
	return (sht40_i2c_read_frame(SHT40_CMD_TEMP_HUMIDITY_MEDIUM_PRECISION,
										temp, humidity, SHT40_MEASURE_TIME_MED_MS));
}

esp_err_t sht40_i2c_get_measure_low_precision(uint16_t *temp, uint16_t *humidity)
{
	return (sht40_i2c_read_frame(SHT40_CMD_TEMP_HUMIDITY_LOW_PRECISION,
										temp, humidity, SHT40_MEASURE_TIME_HIGH_MS));
}

static esp_err_t sht40_i2c_reset_cmd(uint8_t sht40_cmd_reset)
{
	i2c_cmd_handle_t cmd_reset;
	esp_err_t transaction_result = ESP_FAIL;
	uint8_t device_address = SHT40_ADDRESS;

	if (sht40_cmd_reset == SHT40_CMD_GENERAL_CALL_RESET)
	{
		device_address = SHT40_GENERAL_CALL_ADDRESS;
	}

	cmd_reset = i2c_cmd_link_create();
	// start condition
	i2c_master_start(cmd_reset);

	// send address and write op with ACK
	i2c_master_write_byte(cmd_reset, (device_address << 1) | I2C_MASTER_WRITE,
			I2C_ACK);
	// send byte of command high precision measure
	i2c_master_write_byte(cmd_reset, sht40_cmd_reset, I2C_ACK);
	i2c_master_stop(cmd_reset);

	transaction_result = i2c_master_cmd_begin(sht40_i2c_port, cmd_reset,
			SHT40_I2CBUS_WAIT_TIME_MS / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd_reset);

	return transaction_result;
}

esp_err_t sht40_i2c_soft_reset(void)
{
	return (sht40_i2c_reset_cmd(SHT40_CMD_SOFT_RESET));
}

esp_err_t sht40_i2c_general_call_reset(void)
{
	return (sht40_i2c_reset_cmd(SHT40_CMD_GENERAL_CALL_RESET));
}

esp_err_t sht40_i2c_serial_number(uint32_t *serial_number)
{
	uint16_t serial_number_h = 0;
	uint16_t serial_number_l = 0;

	esp_err_t ret = sht40_i2c_read_frame(SHT40_CMD_SERIAL_NUMBER,
			&serial_number_h, &serial_number_l, SHT40_GENERAL_TIME_MS);

	*serial_number = (serial_number_h << 16) | serial_number_l;

	return (ret);
}
