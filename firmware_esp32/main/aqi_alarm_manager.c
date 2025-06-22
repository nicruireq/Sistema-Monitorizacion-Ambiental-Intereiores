/*
 * aqi_alarm_manager.c
 *
 *  Created on: 5 may 2025
 *      Author: NRR
 */

#include "aqi_alarm_manager.h"
#include "aqi_alarm_triggers.h"
#include "global_system_signaler.h"
#include "aqi_config_manager.h"
#include "esp_log.h"

static const char *TAG = "AQI_ALARM_MGR";

static bool alarms_activation_state[AC_MAX_CLASSES];

static evaluate_alarm_trigger_condition*
		alarms_triggers_by_class[AC_MAX_CLASSES] = {
			aqi_is_temperature_above_max,
			aqi_is_temperature_below_min,
			aqi_is_humidity_above_max,
			aqi_is_humidity_below_min,
			aqi_is_voc_index_above_limit
};

void aqi_alarm_manager_init()
{
	for (int c = AC_TEMP_H; c < AC_MAX_CLASSES; c++)
	{
		alarms_activation_state[c] = false;
	}
}

/**
 * @brief Envía una alarma a los canales MQTT y GUI, y actualiza su estado de activación o desactivación.
 *
 * Esta función crea una alarma basada en la clase especificada y el estado deseado (activación o desactivación).
 * Intenta enviar la alarma tanto al sistema MQTT como a la interfaz gráfica de usuario (GUI).
 * Si al menos uno de los envíos es exitoso, se actualiza el estado de activación de la clase de alarma
 * en el vector global 'alarms_activation_state'.
 *
 * @param[in] alarm_class Clase de la alarma a enviar, definida en el enumerado Alarm_class.
 * @param[in] deactivate Indica si la alarma debe ser desactivada (true) o activada (false).
 *
 * @return
 * - ESP_OK si la alarma fue enviada exitosamente a al menos uno de los canales.
 * - ESP_FAIL si no se pudo enviar la alarma por ningún canal.
 *
 * @note Esta función genera mensajes de log para depuración en caso de errores
 *       al crear, clonar o enviar la alarma. El estado de activación de la alarma
 *       solo se actualiza si el mensaje fue enviado correctamente.
 */
static esp_err_t send_alarm(Alarm_class alarm_class, bool deactivate)
{
	Alarm_data_ptr alarm_to_mqtt = NULL;
	Alarm_data_ptr alarm_to_gui = NULL;
	bool alarm_mqtt_created = false;
	esp_err_t err_mqtt = ESP_FAIL;
	esp_err_t err_ui = ESP_FAIL;
	esp_err_t something_sent = ESP_FAIL;

	// Crear alarmas
	alarm_mqtt_created = alarm_type_create(&alarm_to_mqtt, deactivate, alarm_class);
	err_ui = alarm_type_clone(alarm_to_mqtt, &alarm_to_gui);

	// debug
	if (!alarm_mqtt_created)
		ESP_LOGE(TAG, "Alarma clase %s no se puede crear en heap", alarm_class_to_string(alarm_class));
	if (err_ui == ESP_FAIL)
		ESP_LOGE(TAG, "Alarma clase %s no se puede clonar en heap", alarm_class_to_string(alarm_class));

	// enviar alarmas
	if (alarm_mqtt_created)
	{
		err_mqtt = gss_send_alarm_data(alarm_to_mqtt, GSS_ID_MQTT_SENDER);
		if (err_mqtt == ESP_OK)	// debug
		{
			ESP_LOGI(TAG, "alarm %s enviada a cola mqtt", alarm_class_to_string(alarm_class));
		}
		else
		{
			ESP_LOGE(TAG, "alarm %s imposible enviar a cola mqtt", alarm_class_to_string(alarm_class));
		}
	}

	if (err_ui == ESP_OK)
	{
		err_ui = gss_send_alarm_data(alarm_to_gui, GSS_ID_GUI);
		if (err_ui == ESP_OK)	// debug
		{
			ESP_LOGI(TAG, "alarm %s enviada a cola UI", alarm_class_to_string(alarm_class));
		}
		else
		{
			ESP_LOGE(TAG, "alarm %s imposible enviar a cola UI", alarm_class_to_string(alarm_class));
		}
	}

	// Marcar alarma como activada si se ha podido enviar
	// una activacion de alarma al menos por un canal
	if ((err_mqtt == ESP_OK || err_ui == ESP_OK) && !deactivate)
	{
		// Dar por valida la activacion de la clase de alarma tratada
		alarms_activation_state[alarm_class] = true;
		something_sent = ESP_OK;
	}
	else if ((err_mqtt == ESP_OK || err_ui == ESP_OK) && deactivate)
	{
		// En el caso en el que se habia enviado una desactivacion de alarma,
		// marcar alarma como desactivadao si se ha podido enviar el mensaje
		// de desactivacion al menos por un canal

		// Dar por valida la desactivacion de la clase de alarma tratada
		alarms_activation_state[alarm_class] = false;
		something_sent = ESP_OK;
	}

	return something_sent;
}

static void evaluate_procedure(Sensors_data_ptr sensor_data, AQI_device_config_data_t_ptr limits,
								Alarm_class alarm_class)
{
	// Si se cumple la condicion que dispara esta clase de alarma
	if (alarms_triggers_by_class[alarm_class](sensor_data, limits))
	{
		ESP_LOGI(TAG, "Trying alarm generation for: %s", alarm_class_to_string(alarm_class));

		// Si no hay una alarma activa de esta clase
		if (!alarms_activation_state[alarm_class])
		{
			ESP_LOGI(TAG, "Alarm %s generada no estaba activa, SEND ACTIVATION", alarm_class_to_string(alarm_class));
			// enviar activacion alarma
			send_alarm(alarm_class, false);
		}
		else
		{
			// Ya hay una alarma de esta clase activa, no se envia
			ESP_LOGI(TAG, "Alarm %s not sent, ALREADY EXISTS", alarm_class_to_string(alarm_class));
		}
	}
	else
	{
		// si no se cumple la condicion de activacion pero habia
		// una alarma activa de este tipo, enviar desactivacion de alarma
		if (alarms_activation_state[alarm_class])
		{
			ESP_LOGI(TAG, "Alarma %s ya estaba activa, SEND DEACTIVATION", alarm_class_to_string(alarm_class));
			send_alarm(alarm_class, true);
		}
	}
}

void aqi_alarm_manager_evaluate(Sensors_data_ptr incoming_sensor_data)
{
	// Leer limites actuales
	AQI_device_config_data_t current_limits;
	aqi_config_manager_get(AQI_CV_ALARM_TEMP_H, &(current_limits.alarm_temp_h),
			sizeof(current_limits.alarm_temp_h));
	aqi_config_manager_get(AQI_CV_ALARM_TEMP_L, &(current_limits.alarm_temp_l),
			sizeof(current_limits.alarm_temp_l));
	aqi_config_manager_get(AQI_CV_ALARM_HUMIDITY_H, &(current_limits.alarm_humidity_h),
			sizeof(current_limits.alarm_humidity_h));
	aqi_config_manager_get(AQI_CV_ALARM_HUMIDITY_L, &(current_limits.alarm_humidity_l),
			sizeof(current_limits.alarm_humidity_l));
	aqi_config_manager_get(AQI_CV_ALARM_VOC_INDEX_LIMIT, &(current_limits.alarm_voc_index),
			sizeof(current_limits.alarm_voc_index));

	// Debug
	ESP_LOGI(TAG, "Evaluacion. Config actual: TEMP_H=%hu;\nTEMP_L=%hu; "
			"\nHUM_H=%hu;\nHUM_L=%hu;\nVOC_L=%hu", current_limits.alarm_temp_h,
			current_limits.alarm_temp_l, current_limits.alarm_humidity_h,
			current_limits.alarm_humidity_l, current_limits.alarm_voc_index);
	//////////////////

	for (int c = AC_TEMP_H; c < AC_MAX_CLASSES; c++)
	{
		evaluate_procedure(incoming_sensor_data, &current_limits, c);
	}
}
