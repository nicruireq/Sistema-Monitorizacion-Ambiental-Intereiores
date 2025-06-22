/*
 * aqi_config_manager.c
 *
 *  Created on: 18 feb 2025
 *      Author: NRR
 */

#include "aqi_config_manager.h"

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#include "blufi_manager.h"

static const char *TAG = "AQI_CONFIG_MANAGER";

static nvs_handle_t aqi_nvs_handle;

// Configuracion por defecto
static AQI_device_config_data_t aqi_default_config;

// Cache global
static AQI_device_config_data_t aqi_config_cache;

// Para exclusion mutua
static SemaphoreHandle_t aqi_config_manager_mutex = NULL;


esp_err_t aqi_config_manager_init()
{
	AQI_device_config_data_t vars_from_flash;
	aqi_config_var_t var_error;
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_LOGI(TAG, "Flash inicializada?: %s", esp_err_to_name(err));

    // Creacion mutex
    if (aqi_config_manager_mutex == NULL)
    {
    	aqi_config_manager_mutex = xSemaphoreCreateMutex();
    	if (aqi_config_manager_mutex == NULL)
    	{
    		ESP_LOGE(TAG, "No se pudo crear el mutex");
    		err = ESP_FAIL;
    	}
    }

    if (err == ESP_OK)
    {
        err = nvs_open("storage", NVS_READWRITE, &aqi_nvs_handle);

        ESP_LOGI(TAG, "NVS Open?: %s", esp_err_to_name(err));

        if (err == ESP_OK)
        {
        	// Inicializar variables de config en flash si no existen
        	err = aqi_config_manager_get_all(&vars_from_flash, &var_error);
        	if (var_error != AQI_CV_NOT_VAR)
        	{
        		// Cargar config por defecto
        		aqi_default_config.save_screen_seconds = 30;
        		strncpy(aqi_default_config.room_name, "Room", AQI_MAX_ROOM_NAME_SZ);
        		aqi_default_config.alarm_temp_h = 30;
        		aqi_default_config.alarm_temp_l = 17;
        		aqi_default_config.alarm_humidity_h = 70;
        		aqi_default_config.alarm_humidity_l = 40;
        		aqi_default_config.alarm_voc_index = 300;
        		aqi_default_config.reset_wifi_provisioning = false;

        		// Si no existen crear
        		err = aqi_config_manager_set_all(&aqi_default_config);
        		ESP_ERROR_CHECK(err);
        	}
        	else
        	{
        		ESP_LOGI(TAG, "Variables de config en flash ya estaban creadas");
        		// Inicializar cache si ya existian las variables en la flash
        		aqi_device_config_data_clone(&vars_from_flash, &aqi_config_cache);
        	}
        }
        else
        {
            ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error initializing NVS: %s", esp_err_to_name(err));
    }

    return err;
}


esp_err_t aqi_config_manager_get(aqi_config_var_t var, void * out_buffer, size_t len)
{
	esp_err_t ret = ESP_ERR_INVALID_ARG;

	if (out_buffer != NULL)
	{
		if (xSemaphoreTake(aqi_config_manager_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			switch (var)
			{
			case AQI_CV_SCREEN_TIME:
				if (len == sizeof(uint8_t))
				{
					*((uint8_t *)out_buffer) = aqi_config_cache.save_screen_seconds;
					ret =  ESP_OK;
				}
				break;
			case AQI_CV_ROOM_NAME:
				if (len == AQI_MAX_ROOM_NAME_SZ)
				{
					strncpy((char *)out_buffer, aqi_config_cache.room_name, AQI_MAX_ROOM_NAME_SZ);
					ret =  ESP_OK;
				}
				break;
			case AQI_CV_ALARM_TEMP_H:
				if (len == sizeof(uint16_t))
				{
					*((uint16_t *)out_buffer) = aqi_config_cache.alarm_temp_h;
					ret =  ESP_OK;
				}
				break;
			case AQI_CV_ALARM_TEMP_L:
				if (len == sizeof(uint16_t))
				{
					*((uint16_t *)out_buffer) = aqi_config_cache.alarm_temp_l;
					ret =  ESP_OK;
				}
				break;
			case AQI_CV_ALARM_HUMIDITY_H:
				if (len == sizeof(uint16_t))
				{
					*((uint16_t *)out_buffer) = aqi_config_cache.alarm_humidity_h;
					ret =  ESP_OK;
				}
				break;
			case AQI_CV_ALARM_HUMIDITY_L:
				if (len == sizeof(uint16_t))
				{
					*((uint16_t *)out_buffer) = aqi_config_cache.alarm_humidity_l;
					ret =  ESP_OK;
				}
				break;
			case AQI_CV_ALARM_VOC_INDEX_LIMIT:
				if (len == sizeof(uint16_t))
				{
					*((uint16_t *)out_buffer) = aqi_config_cache.alarm_voc_index;
					ret =  ESP_OK;
				}
				break;
			case AQI_CV_WIFI_PROVISIONING_STATE:
				if (len == sizeof(bool))
				{
					*((bool *)out_buffer) = aqi_config_cache.reset_wifi_provisioning;
					ret =  ESP_OK;
				}
				break;
			default:
				break;
			}

			xSemaphoreGive(aqi_config_manager_mutex);
		}
		else
		{
			ESP_LOGE(TAG, "No se pudo cerrar el mutex");
			ret = ESP_ERR_TIMEOUT;
		}
	}

	return ret;
}


// Metodo antiguo que leia de la flash directamente
//esp_err_t aqi_config_manager_get(aqi_config_var_t var, void * out_buffer, size_t len)
//{
//	esp_err_t err = ESP_FAIL;
//
//	if (out_buffer != NULL)
//	{
//		switch (var)
//		{
//		case AQI_CV_SCREEN_TIME:
//			if (len == sizeof(uint8_t))
//			{
//				// dejar de referencia la siguiente linea por si se implementara
//				// un parametro para forzar la lectura desde la flash en vez de cache
//				err = nvs_get_u8(aqi_nvs_handle, AQI_KEY_SCREEN_TIME, (uint8_t *)out_buffer);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		case AQI_CV_ROOM_NAME:
//			if (len == AQI_MAX_ROOM_NAME_SZ)
//			{
//				err = nvs_get_str(aqi_nvs_handle, AQI_KEY_ROOM_NAME, (char *)out_buffer, &len);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		case AQI_CV_ALARM_TEMP_H:
//			if (len == sizeof(uint16_t))
//			{
//				err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_H, (uint16_t *)out_buffer);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		case AQI_CV_ALARM_TEMP_L:
//			if (len == sizeof(uint16_t))
//			{
//				err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_L, (uint16_t *)out_buffer);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		case AQI_CV_ALARM_HUMIDITY_H:
//			if (len == sizeof(uint16_t))
//			{
//				err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_H, (uint16_t *)out_buffer);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		case AQI_CV_ALARM_HUMIDITY_L:
//			if (len == sizeof(uint16_t))
//			{
//				err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_L, (uint16_t *)out_buffer);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		case AQI_CV_ALARM_VOC_INDEX_LIMIT:
//			if (len == sizeof(uint16_t))
//			{
//				err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_VOC_INDEX_LIMIT, (uint16_t *)out_buffer);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		case AQI_CV_WIFI_PROVISIONING_STATE:
//			if (len == sizeof(bool))
//			{
//				int8_t aqi_cv_wps_buff = 0;
//				bool* aqi_cv_wps_out = ((bool *) out_buffer);
//				err = nvs_get_i8(aqi_nvs_handle, AQI_KEY_WIFI_PROVISIONING_STATE, &aqi_cv_wps_buff);
//				*aqi_cv_wps_out = ((bool) aqi_cv_wps_buff);
//			}
//			else
//			{
//				err = ESP_ERR_INVALID_ARG;
//			}
//			break;
//		default:
//			err = ESP_ERR_INVALID_ARG;
//		}
//	}
//	else
//	{
//		err = ESP_ERR_INVALID_ARG;
//	}
//
//	return err;
//}

esp_err_t aqi_config_manager_get_all(AQI_device_config_data_t_ptr config, aqi_config_var_t* key_data_failed)
{
	// suponer que sera OK y acumular errores si hubiera
	esp_err_t err = ESP_OK;
	size_t len = AQI_MAX_ROOM_NAME_SZ;
	// En principio marcar que no hay errores
	*key_data_failed = AQI_CV_NOT_VAR;

	if (config == NULL || key_data_failed == NULL)
	{
		err = ESP_ERR_INVALID_ARG;
	}
	else
	{
		if (xSemaphoreTake(aqi_config_manager_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			if ((err = nvs_get_u8(aqi_nvs_handle, AQI_KEY_SCREEN_TIME, &(config->save_screen_seconds))) != ESP_OK)
			{
				*key_data_failed = AQI_CV_SCREEN_TIME;
			}

			if ((err = nvs_get_str(aqi_nvs_handle, AQI_KEY_ROOM_NAME, config->room_name, &len)) != ESP_OK)
			{
				*key_data_failed = AQI_CV_ROOM_NAME;
			}

			if ((err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_H, &(config->alarm_temp_h))) != ESP_OK)
			{
				*key_data_failed = AQI_CV_ALARM_TEMP_H;
			}

			if ((err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_L, &(config->alarm_temp_l))) != ESP_OK)
			{
				*key_data_failed = AQI_CV_ALARM_TEMP_L;
			}

			if ((err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_H, &(config->alarm_humidity_h))) != ESP_OK)
			{
				*key_data_failed = AQI_CV_ALARM_HUMIDITY_H;
			}

			if ((err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_L, &(config->alarm_humidity_l))) != ESP_OK)
			{
				*key_data_failed = AQI_CV_ALARM_HUMIDITY_L;
			}

			if ((err = nvs_get_u16(aqi_nvs_handle, AQI_KEY_ALARM_VOC_INDEX_LIMIT, &(config->alarm_voc_index))) != ESP_OK)
			{
				*key_data_failed = AQI_CV_ALARM_VOC_INDEX_LIMIT;
			}

			int8_t aqi_cv_wps_buff = 0;
			if ((err = nvs_get_i8(aqi_nvs_handle, AQI_KEY_WIFI_PROVISIONING_STATE, &aqi_cv_wps_buff)) != ESP_OK)
			{
				*key_data_failed = AQI_CV_WIFI_PROVISIONING_STATE;
			}
			else
			{
				// inicializacion correcta con conversiones de tipos
				config->reset_wifi_provisioning = ((bool) aqi_cv_wps_buff);
			}

			xSemaphoreGive(aqi_config_manager_mutex);
		}
		else
		{
			ESP_LOGE(TAG, "No se pudo tomar el mutex");
			err = ESP_ERR_TIMEOUT;
		}
	}

	return err;
}

esp_err_t aqi_config_manager_set(aqi_config_var_t var, void * to_write, size_t len)
{
	esp_err_t err = ESP_FAIL;

	if (to_write != NULL)
	{
		if (xSemaphoreTake(aqi_config_manager_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			switch (var)
			{
			case AQI_CV_SCREEN_TIME:
			{
				if (len == sizeof(uint8_t))
				{
					uint8_t val = *((uint8_t *)to_write);
					err = nvs_set_u8(aqi_nvs_handle, AQI_KEY_SCREEN_TIME, val);
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			case AQI_CV_ROOM_NAME:
			{
				if (len == AQI_MAX_ROOM_NAME_SZ)
				{
					err = nvs_set_str(aqi_nvs_handle, AQI_KEY_ROOM_NAME, (char *)to_write);
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			case AQI_CV_ALARM_TEMP_H:
			{
				if (len == sizeof(uint16_t))
				{
					uint16_t val = *((uint16_t *)to_write);
					err = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_H, val);
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			case AQI_CV_ALARM_TEMP_L:
			{
				if (len == sizeof(uint16_t))
				{
					uint16_t val = *((uint16_t *)to_write);
					err = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_L, val);
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			case AQI_CV_ALARM_HUMIDITY_H:
			{
				if (len == sizeof(uint16_t))
				{
					uint16_t val = *((uint16_t *)to_write);
					err = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_H, val);
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			case AQI_CV_ALARM_HUMIDITY_L:
			{
				if (len == sizeof(uint16_t))
				{
					uint16_t val = *((uint16_t *)to_write);
					err = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_L, val);
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			case AQI_CV_ALARM_VOC_INDEX_LIMIT:
			{
				if (len == sizeof(uint16_t))
				{
					uint16_t val = *((uint16_t *)to_write);
					err = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_VOC_INDEX_LIMIT, val);
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			case AQI_CV_WIFI_PROVISIONING_STATE:
			{
				if (len == sizeof(bool))
				{
					err = nvs_set_i8(aqi_nvs_handle, AQI_KEY_WIFI_PROVISIONING_STATE, *((int8_t *)to_write));
				}
				else
				{
					err = ESP_ERR_INVALID_ARG;
				}
				break;
			}
			default:
				err = ESP_ERR_INVALID_ARG;
				ESP_LOGE(TAG, "Variable no reconocida: %d", var);
			}

			// solo si las escrituras son exitosas
			if (err == ESP_OK)
			{
				err = nvs_commit(aqi_nvs_handle);

				if (err == ESP_OK)
				{
					// Actualizar la cache despues del commit
					switch (var)
					{
					case AQI_CV_SCREEN_TIME:
						aqi_config_cache.save_screen_seconds = *((uint8_t *)to_write);
						break;
					case AQI_CV_ROOM_NAME:
						strncpy(aqi_config_cache.room_name, (char *)to_write, AQI_MAX_ROOM_NAME_SZ);
						break;
					case AQI_CV_ALARM_TEMP_H:
						aqi_config_cache.alarm_temp_h = *((uint16_t *)to_write);
						break;
					case AQI_CV_ALARM_TEMP_L:
						aqi_config_cache.alarm_temp_l = *((uint16_t *)to_write);
						break;
					case AQI_CV_ALARM_HUMIDITY_H:
						aqi_config_cache.alarm_humidity_h = *((uint16_t *)to_write);
						break;
					case AQI_CV_ALARM_HUMIDITY_L:
						aqi_config_cache.alarm_humidity_l = *((uint16_t *)to_write);
						break;
					case AQI_CV_ALARM_VOC_INDEX_LIMIT:
						aqi_config_cache.alarm_voc_index = *((uint16_t *)to_write);
						break;
					case AQI_CV_WIFI_PROVISIONING_STATE:
						aqi_config_cache.reset_wifi_provisioning = *((bool *)to_write);
						break;
					default:
						break;
					}
				}
				else
				{
					ESP_LOGE(TAG, "Error en nvs commit: %s", esp_err_to_name(err));
				}
			}

			xSemaphoreGive(aqi_config_manager_mutex);
		}
		else
		{
			ESP_LOGE(TAG, "No se pudo tomar el mutex");
			err = ESP_ERR_TIMEOUT;
		}
	}
	else
	{
		err = ESP_ERR_INVALID_ARG;
		ESP_LOGE(TAG, "Buffer de escritura nulo");
	}

	return err;
}

esp_err_t aqi_config_manager_set_all(const AQI_device_config_data_t_ptr config)
{
	esp_err_t err[AQI_NUM_CFG_VARS];
	esp_err_t ret = ESP_FAIL;
	uint8_t writings_number = 0;
	uint8_t i;

	if (config != NULL)
	{
		if (xSemaphoreTake(aqi_config_manager_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			err[AQI_CV_SCREEN_TIME] = nvs_set_u8(aqi_nvs_handle, AQI_KEY_SCREEN_TIME, config->save_screen_seconds);
			err[AQI_CV_ROOM_NAME] = nvs_set_str(aqi_nvs_handle, AQI_KEY_ROOM_NAME, config->room_name);
			err[AQI_CV_ALARM_TEMP_H] = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_H, config->alarm_temp_h);
			err[AQI_CV_ALARM_TEMP_L] = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_TEMP_L, config->alarm_temp_l);
			err[AQI_CV_ALARM_HUMIDITY_H] = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_H, config->alarm_humidity_h);
			err[AQI_CV_ALARM_HUMIDITY_L] = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_HUMIDITY_L, config->alarm_humidity_l);
			err[AQI_CV_ALARM_VOC_INDEX_LIMIT] = nvs_set_u16(aqi_nvs_handle, AQI_KEY_ALARM_VOC_INDEX_LIMIT, config->alarm_voc_index);
			err[AQI_CV_WIFI_PROVISIONING_STATE] = nvs_set_i8(aqi_nvs_handle, AQI_KEY_WIFI_PROVISIONING_STATE, (int8_t) config->reset_wifi_provisioning);

			for (i = 0; i < AQI_NUM_CFG_VARS; i++)
			{
				if (err[i] == ESP_OK)
				{
					writings_number++;
				}
				else
				{
					ESP_LOGE(TAG, "Error escribiendo variable %u: %s", i, esp_err_to_name(err[i]));
				}
			}

			// Solo commit cuando todas las escrituras se pueden realizar
			if (writings_number == AQI_NUM_CFG_VARS)
			{
				esp_err_t commit_err = nvs_commit(aqi_nvs_handle);
				if (commit_err == ESP_OK)
				{
					// sincroniza la cache
					aqi_device_config_data_clone(config, &aqi_config_cache);
					ret = ESP_OK;
				}
				else
				{
					ESP_LOGE(TAG, "Error en nvs_commit: %s", esp_err_to_name(commit_err));
				}
			}

			xSemaphoreGive(aqi_config_manager_mutex);
		}
		else
		{
			ESP_LOGE(TAG, "No se pudo tomar el mutex");
			ret = ESP_ERR_TIMEOUT;
		}
	}
	else
	{
		ESP_LOGE(TAG, "config data invalid");
	}

	return ret;
}

void aqi_config_manager_reset_wifi_provisioning()
{
	bool to_provision = true;

	// Indicar con el flag que queremos volver a aprovisionar wifi
	esp_err_t err = aqi_config_manager_set(AQI_CV_WIFI_PROVISIONING_STATE,
										&to_provision, sizeof(bool));
	if (err == ESP_OK)
	{
		err = blufi_manager_release_wifi_config();
		if (err != ESP_OK)
		{
			ESP_LOGE(TAG, "La config del stack wifi actual en NVS "
					"podria no haberse borrado correctamente");
		}
		else
		{
			ESP_LOGI(TAG, "La config del stack wifi esta limpia");
		}

		err = aqi_config_manager_release();

		// Tras el reinicio se comprueba el flag para indicar si debe
		// iniciar el proceso de aprovisionamiento wifi (true)
		ESP_LOGI(TAG, "Reiniciando para comenzar wifi provisioning...");

		esp_restart();
	}
	else
	{
		ESP_ERROR_CHECK(err);
	}
}

esp_err_t aqi_config_manager_release()
{
	esp_err_t err = ESP_FAIL;

	nvs_close(aqi_nvs_handle);

	err = nvs_flash_deinit();

	if (err == ESP_OK)
	{
		if (aqi_config_manager_mutex != NULL)
		{
			vSemaphoreDelete(aqi_config_manager_mutex);
			aqi_config_manager_mutex = NULL;
		}
	}

	return err;
}

esp_err_t aqi_process_received_config_data(const char* incoming_data, int len)
{
	esp_err_t err = ESP_FAIL;
	AQI_device_config_data_t new_config_data;
	AQI_device_config_data_t current_config_data;
	aqi_config_var_t var_failed = AQI_CV_NOT_VAR;

	if (incoming_data != NULL)
	{
		err = aqi_device_config_data_type_parse(incoming_data, len, &new_config_data);
		if (err == ESP_OK)
		{
			err = aqi_config_manager_get_all(&current_config_data, &var_failed);

			// conservar integridad en la actualizacion de los datos a modo
			// de transaccion, o se lee todo o nada
			if (err == ESP_OK)
			{
				// ========Debug=======
				ESP_LOGI(TAG, "Config current:\n"
						"screen time = %u\n"
						"room name = %s\n"
						"temp_H = %u\n"
						"temp_L = %u\n"
						"humi_H = %u\n"
						"humi_L = %u\n"
						"voc level = %u\n"
						"reset_wifi_prov = %d\n\n"
						"New config:\n"
						"screen time = %u\n"
												"room name = %s\n"
												"temp_H = %u\n"
												"temp_L = %u\n"
												"humi_H = %u\n"
												"humi_L = %u\n"
												"voc level = %u\n"
												"reset_wifi_prov = %d\n\n", current_config_data.save_screen_seconds,
						current_config_data.room_name, current_config_data.alarm_temp_h,
						current_config_data.alarm_temp_l, current_config_data.alarm_humidity_h,
						current_config_data.alarm_humidity_l, current_config_data.alarm_voc_index,
						current_config_data.reset_wifi_provisioning,
						new_config_data.save_screen_seconds, new_config_data.room_name,
						new_config_data.alarm_temp_h, new_config_data.alarm_temp_l,
						new_config_data.alarm_humidity_h, new_config_data.alarm_humidity_l,
						new_config_data.alarm_voc_index, new_config_data.reset_wifi_provisioning);
				//=====================

				// Solo actualizamos los que hayan cambiado
				// activar procedimiento adecuado en el sistema a cada cambio de config

				if (current_config_data.save_screen_seconds
						!= new_config_data.save_screen_seconds)
				{
					if (aqi_config_manager_set(AQI_CV_SCREEN_TIME,
							&(new_config_data.save_screen_seconds), sizeof(uint8_t)) == ESP_OK)
					{
						// Actualizar var en cache
						aqi_config_cache.save_screen_seconds = new_config_data.save_screen_seconds;
						// TODO notificar cambio de tiempo de pantalla inactiva
					}
				}

				if (strncmp(current_config_data.room_name, new_config_data.room_name,
						AQI_MAX_ROOM_NAME_SZ) != 0)
				{
					if (aqi_config_manager_set(AQI_CV_ROOM_NAME,
							&(new_config_data.room_name), AQI_MAX_ROOM_NAME_SZ) == ESP_OK)
					{
						// Actualizar var en cache
						strncpy(aqi_config_cache.room_name, new_config_data.room_name,
								AQI_MAX_ROOM_NAME_SZ);
						// TODO notificar cambio de nombre de estancia
					}
				}

				if (current_config_data.alarm_temp_h
						!= new_config_data.alarm_temp_h)
				{
					if (aqi_config_manager_set(AQI_CV_ALARM_TEMP_H,
							&(new_config_data.alarm_temp_h), sizeof(uint16_t)) == ESP_OK)
					{
						// Actualizar var en cache
						aqi_config_cache.alarm_temp_h = new_config_data.alarm_temp_h;
					// TODO notificar cambio de limite de alarma de temp superior
					}
				}

				if (current_config_data.alarm_temp_l
						!= new_config_data.alarm_temp_l)
				{
					if (aqi_config_manager_set(AQI_CV_ALARM_TEMP_L,
							&(new_config_data.alarm_temp_l), sizeof(uint16_t)) == ESP_OK)
					{
						// Actualizar var en cache
						aqi_config_cache.alarm_temp_l = new_config_data.alarm_temp_l;
					// TODO notificar cambio de limite de alarma de temp inferior
					}
				}

				if (current_config_data.alarm_humidity_h
						!= new_config_data.alarm_humidity_h)
				{
					if (aqi_config_manager_set(AQI_CV_ALARM_HUMIDITY_H,
							&(new_config_data.alarm_humidity_h), sizeof(uint16_t)) == ESP_OK)
					{
						aqi_config_cache.alarm_humidity_h = new_config_data.alarm_humidity_h;
						// TODO notificar cambio de limite de alarma de humedad superior
					}
				}

				if (current_config_data.alarm_humidity_l
						!= new_config_data.alarm_humidity_l)
				{
					if (aqi_config_manager_set(AQI_CV_ALARM_HUMIDITY_L,
							&(new_config_data.alarm_humidity_l), sizeof(uint16_t)) == ESP_OK)
					{
						aqi_config_cache.alarm_humidity_l = new_config_data.alarm_humidity_l;
						// TODO notificar cambio de limite de alarma de humedad inferior
					}
				}

				if (current_config_data.alarm_voc_index
						!= new_config_data.alarm_voc_index)
				{
					if (aqi_config_manager_set(AQI_CV_ALARM_VOC_INDEX_LIMIT,
							&(new_config_data.alarm_voc_index), sizeof(uint16_t)) == ESP_OK)
					{
						aqi_config_cache.alarm_voc_index = new_config_data.alarm_voc_index;
					// TODO notificar cambio de limite de alarma de voc index
					}
				}

				// Solo hay que comprobar si se quiere resetear el provisioning
				// para notificar el inicio del proceso
				if (new_config_data.reset_wifi_provisioning)
				{
					// TODO guardar los params de aprendizaje del algoritmo de VOC
					//		antes del reset ?
					// TODO notificar el reset del provisioning

					aqi_config_manager_reset_wifi_provisioning();
				}
			}
			else
			{
				ESP_LOGE(TAG, "Lectura de config actual de la flash fallida.\n"
							  "Tipo error: %s; Var fallida: %d",
							  esp_err_to_name(err), var_failed);
			}
		}
	}
	else
	{
		err = ESP_ERR_INVALID_ARG;
	}

	return err;
}
