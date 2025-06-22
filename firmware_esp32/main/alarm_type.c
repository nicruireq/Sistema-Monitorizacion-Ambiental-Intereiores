/*
 * alarm_type.c
 *
 *  Created on: 6 may 2025
 *      Author: NRR
 */


#include "alarm_type.h"
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "ALARM_TYPE";

static const char* info_sentences[AC_MAX_CLASSES] = {
    "La temperatura es demasiado elevada. Recomendamos activar la climatizacion.",
    "La temperatura es demasiado baja. Recomendamos activar la climatizacion.",
    "La humedad es demasiado elevada. Recomendamos deshumidificar el ambiente.",
    "La humedad es demasiado baja. Recomendamos humidificar el ambiente.",
    "La calidad del aire es mala. Recomendamos ventilar o purificar el aire."
};


const char* alarm_class_to_string(Alarm_class aclass)
{
	switch (aclass)
	{
		case AC_TEMP_H:
			return "AC_TEMP_H";
		case AC_TEMP_L:
			return "AC_TEMP_L";
		case AC_HUM_H:
			return "AC_HUM_H";
		case AC_HUM_L:
			return "AC_HUM_L";
		case AC_VOC_LIMIT:
			return "AC_VOC_LIMIT";
		case AC_MAX_CLASSES:
			return "AC_MAX_CLASSES";
		default:
			return "UNKNOWN_ALARM_CLASS";
	}
}


bool alarm_type_create(Alarm_data_ptr* out_alarm_data, bool disable,
                       Alarm_class alarm_class)
{
    if (out_alarm_data == NULL)
    {
        return false;
    }

    heap_caps_check_integrity_all(true);
    ESP_LOGI(TAG, "Heap libre antes de malloc: %d", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

    (*out_alarm_data) = (Alarm_data_ptr)malloc(sizeof(Alarm_data_t));

    heap_caps_check_integrity_all(true);
    ESP_LOGI(TAG, "Heap libre despues de malloc: %d", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

    // fallo de malloc
    if ((*out_alarm_data) == NULL)
    {
        return false;
    }

    (*out_alarm_data)->disable = disable;
    (*out_alarm_data)->alarm_class = alarm_class;
    (*out_alarm_data)->info = info_sentences[alarm_class];

    return true;
}

esp_err_t alarm_type_clone(Alarm_data_ptr src_alarm_data, Alarm_data_ptr* dst_alarm_data)
{
    if (src_alarm_data == NULL || dst_alarm_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *dst_alarm_data = (Alarm_data_ptr)malloc(sizeof(Alarm_data_t));
    if (*dst_alarm_data == NULL)
    {
        return ESP_FAIL;
    }

    (*dst_alarm_data)->disable = src_alarm_data->disable;
    (*dst_alarm_data)->alarm_class = src_alarm_data->alarm_class;
    (*dst_alarm_data)->info = src_alarm_data->info;

    return ESP_OK;
}

esp_err_t alarm_type_to_JSON(struct json_out * json_buffer, int buffer_size, Alarm_data_ptr alarm_data)
{
    esp_err_t ret = ESP_FAIL;
    int printed = 0;

    if ((alarm_data != NULL) && (json_buffer != NULL))
    {
        printed = json_printf(json_buffer, "{ disable: %B, "
                                         "alarm_class: %d,"
                                         "info: %Q }",
                                         ((int)alarm_data->disable),
                                         alarm_data->alarm_class,
                                         alarm_data->info);
        if (printed > buffer_size)
        {
        	ESP_LOGE(TAG, "Se escriben mas bytes(%d) que el size del buffer(%d)",
        			printed, buffer_size);

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

void alarm_data_release(Alarm_data_t* alarm_data)
{
	if (alarm_data != NULL) free(alarm_data);
}
