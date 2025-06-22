/*
 * aqi_alarm_triggers.c
 *
 *  Created on: 7 may 2025
 *      Author: NRR
 */

#include "aqi_alarm_triggers.h"
#include "esp_log.h"

static const char *TAG = "AQI_ALARM_TRIG";

bool aqi_is_temperature_below_min(const Sensors_data_ptr sensor_data,
		const AQI_device_config_data_t_ptr config)
{
	ESP_LOGI(TAG, "Ejecuta trigger TEMP_L: %hu < %hu ?", sensor_data->temperature_celsius, config->alarm_temp_l);
    return sensor_data->temperature_celsius < config->alarm_temp_l;
}

bool aqi_is_temperature_above_max(const Sensors_data_ptr sensor_data,
		const AQI_device_config_data_t_ptr config)
{
	ESP_LOGI(TAG, "Ejecuta trigger TEMP_H: %hu > %hu ?", sensor_data->temperature_celsius, config->alarm_temp_h);
    return sensor_data->temperature_celsius > config->alarm_temp_h;
}

bool aqi_is_humidity_below_min(const Sensors_data_ptr sensor_data,
		const AQI_device_config_data_t_ptr config)
{
	ESP_LOGI(TAG, "Ejecuta trigger HUM_L: %hu < %hu ?", sensor_data->relative_humidity, config->alarm_humidity_l);
    return sensor_data->relative_humidity < config->alarm_humidity_l;
}

bool aqi_is_humidity_above_max(const Sensors_data_ptr sensor_data,
		const AQI_device_config_data_t_ptr config)
{
	ESP_LOGI(TAG, "Ejecuta trigger HUM_H: %hu > %hu ?", sensor_data->relative_humidity, config->alarm_humidity_h);
    return sensor_data->relative_humidity > config->alarm_humidity_h;
}

bool aqi_is_voc_index_above_limit(const Sensors_data_ptr sensor_data,
		const AQI_device_config_data_t_ptr config)
{
	ESP_LOGI(TAG, "Ejecuta trigger VOC: %hu > %hu ?", sensor_data->voc_index, config->alarm_voc_index);
    return sensor_data->voc_index > config->alarm_voc_index;
}
