/*
 * aqi_device_config.c
 *
 *  Created on: 18 feb 2025
 *      Author: NRR
 */

#include "aqi_device_config_type.h"
#include <string.h>

#include "esp_log.h"

static const char *TAG = "AQI_DEVICE_CONFIG_TYPE";

bool aqi_device_config_data_type_init(AQI_device_config_data_t_ptr device_config_data ,
										uint8_t save_screen_seconds,
										const char * room_name,
										uint16_t alarm_temp_h,
										uint16_t alarm_temp_l,
										uint16_t alarm_humidity_h,
										uint16_t alarm_humidity_l,
										uint16_t alarm_voc_index,
										bool reset_wifi_provisioning)
{
	bool ret = false;

	if (device_config_data != NULL)
	{
		device_config_data->save_screen_seconds = save_screen_seconds;

		if (room_name == NULL)
		{
			memset(device_config_data->room_name, '\0', AQI_MAX_ROOM_NAME_SZ);
		}
		else
		{
			strncpy(device_config_data->room_name, room_name, AQI_MAX_ROOM_NAME_SZ);
		}

		device_config_data->room_name[AQI_MAX_ROOM_NAME_SZ-1] = '\0';
		device_config_data->alarm_temp_h = alarm_temp_h;
		device_config_data->alarm_humidity_l = alarm_temp_l;
		device_config_data->alarm_humidity_h= alarm_humidity_h,
		device_config_data->alarm_humidity_l = alarm_humidity_l;
		device_config_data->alarm_voc_index = alarm_voc_index;
		device_config_data->reset_wifi_provisioning = reset_wifi_provisioning;
		ret = true;
	}
	else
	{
		ret = false;
	}

	return ret;
}

bool aqi_device_config_data_clone(const AQI_device_config_data_t_ptr src, AQI_device_config_data_t_ptr dest)
{
    if (src == NULL || dest == NULL)
    {
        return false;
    }

    dest->save_screen_seconds = src->save_screen_seconds;
    strncpy(dest->room_name, src->room_name, AQI_MAX_ROOM_NAME_SZ);
    dest->room_name[AQI_MAX_ROOM_NAME_SZ - 1] = '\0'; // Asegura terminación nula

    dest->alarm_temp_h = src->alarm_temp_h;
    dest->alarm_temp_l = src->alarm_temp_l;
    dest->alarm_humidity_h = src->alarm_humidity_h;
    dest->alarm_humidity_l = src->alarm_humidity_l;
    dest->alarm_voc_index = src->alarm_voc_index;
    dest->reset_wifi_provisioning = src->reset_wifi_provisioning;

    return true;
}


esp_err_t aqi_device_config_data_type_parse(const char *data, int data_len,
								AQI_device_config_data_t_ptr device_config_data)
{
	int ret = 0;
	char * room_name = NULL;

	ret = json_scanf(data, data_len,
			"{ screen_sec: %hhu,"
			"room: %Q,"
			"alarm_voc: %hu,"
			"alarm_temp_h: %hu,"
			"alarm_temp_l: %hu,"
			"alarm_hum_h: %hu,"
			"alarm_hum_l: %hu,"
			"rst_wifi_prov: %B }",
			&(device_config_data->save_screen_seconds),
			&room_name,
			&(device_config_data->alarm_voc_index),
			&(device_config_data->alarm_temp_h),
			&(device_config_data->alarm_temp_l),
			&(device_config_data->alarm_humidity_h),
			&(device_config_data->alarm_humidity_l),
			&(device_config_data->reset_wifi_provisioning)
	);

	if ((room_name != NULL) && (strlen(room_name) > 0))
	{
		strncpy(device_config_data->room_name, room_name, AQI_MAX_ROOM_NAME_SZ);
		device_config_data->room_name[AQI_MAX_ROOM_NAME_SZ-1] = '\0';
		free(room_name);
	}

	// no data consumed or scan error
	if (ret <= 0)
	{
		ret = ESP_FAIL;
	}
	else
	{
		ret = ESP_OK;
	}

	return ret;
}
