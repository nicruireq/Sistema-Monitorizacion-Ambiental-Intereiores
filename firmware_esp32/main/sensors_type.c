/*
 * sensors_type.c
 *
 *  Created on: 30 ene 2025
 *      Author: NRR
 */

#include "sensors_type.h"
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "SENSORS_TYPE";

bool sensors_type_create(Sensors_data_ptr* out_sensor_data , uint16_t voc_raw,
						uint16_t voc_index, uint16_t temperature_celsius,
						uint16_t relative_humidity)
{
	if (out_sensor_data == NULL)
	{
		return false;
	}

	heap_caps_check_integrity_all(true);
	ESP_LOGI(TAG, "Heap libre antes de malloc: %d", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

	(*out_sensor_data) = (Sensors_data_ptr)malloc(sizeof(Sensors_data_t));

	heap_caps_check_integrity_all(true);
	ESP_LOGI(TAG, "Heap libre despues de malloc: %d", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

	// fallo de malloc
	if ((*out_sensor_data) == NULL)
	{
		return false;
	}

	(*out_sensor_data)->relative_humidity = relative_humidity;
	(*out_sensor_data)->temperature_celsius = temperature_celsius;
	(*out_sensor_data)->voc_index = voc_index;
	(*out_sensor_data)->voc_raw = voc_raw;

	return true;
}

esp_err_t sensors_type_to_JSON(struct json_out * json_buffer, uint32_t buffer_size, Sensors_data_ptr sensors_data)
{
	esp_err_t ret = ESP_FAIL;
	int printed = 0;

	if ((sensors_data != NULL) && (json_buffer != NULL))
	{
		printed = json_printf(json_buffer, " { humidity_rh: %u, "
								 "temp_celsius: %u,"
								 "voc_raw: %u,"
								 "voc_index: %u}",
								 sensors_data->relative_humidity,
								 sensors_data->temperature_celsius,
								 sensors_data->voc_raw,
								 sensors_data->voc_index);
		if (printed > buffer_size)
		{
			ret = ESP_ERR_INVALID_SIZE;
		}
		else
		{
			ret = ESP_OK;
		}
	}
	else
	{
		ret = ESP_ERR_INVALID_ARG;
	}

	return ret;
}

//esp_err_t sensors_type_parse(char *data, int data_len, Sensors_data_ptr sensors_data)
//{
//	int ret;
//
//	ret = json_scanf(data, data_len,
//			"{ humidity_rh: %Q,"
//				"temp_celsius: %Q,"
//				"voc_index: %f,",
//			&(weatherData->city), &(weatherData->description),
//			&(weatherData->feels_like), &(weatherData->humidity),
//			&(weatherData->temp), &(weatherData->temp_max),
//			&(weatherData->temp_min), &(weatherData->wind_speed)
//		);
//
//	if (!ret)
//	{
//		ret = ESP_FAIL;
//	}
//	else
//	{
//		ret = ESP_OK;
//	}
//
//	return ret;
//}
