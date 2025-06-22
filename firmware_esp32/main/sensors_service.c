/*
 * sensors_service.c
 *
 *  Created on: 26 ene 2025
 *      Author: NRR
 */

#include "sensors_service.h"
#include "sensors_type.h"
#include "aqi_alarm_manager.h"

#include "esp_log.h"
#include "esp_check.h"

static const char * TAG = "SENSORS_SERVICE";

static TaskHandle_t sensors_service_task_handler = NULL;
static i2c_port_t current_i2c_master_port;
static GasIndexAlgorithmParams voc_algorithm_params;


/**
 * Task for sensors reading each interval
 */
static void sensors_sampling_task( void * pvParameters )
{
	int milliseconds = (int) pvParameters;
	TickType_t xLastWakeTime;

	uint16_t last_humidity_rh = SGP40_DEFAULT_HUMIDITY_RH;
	uint16_t last_temperature_celsius = SGP40_DEFAULT_TEMPERATURE_CELSIUS;
	uint16_t last_humidity_raw = RH_to_ticks(last_humidity_rh);
	uint16_t last_temperature_raw = Celsius_to_ticks(last_temperature_celsius);
	uint16_t last_VOC_raw = 0;
	int32_t last_VOC_index = 0;
	sgp40_compensation_t last_sgp40_compensation;
	sgp40_compensation_t_init(&last_sgp40_compensation,
			last_temperature_celsius,
			last_humidity_rh);
	SGP40_COMPENSATION selected_compensation = SGP40_COMP_ON;
	bool first_SGP40_measure = true;
	bool first_SHT40_measure = true;
	uint8_t sht40_errors_counter = 0;
	uint8_t sgp40_errors_counter = 0;
	bool feed_voc_algorithm = true;

	while (1)
	{
		esp_err_t retSHT = sht40_i2c_get_measure_high_precision(&last_temperature_raw, &last_humidity_raw);

		if (retSHT != ESP_OK)
		{
			// casos de error de lectura del SHT40
			if (first_SHT40_measure)
			{
				selected_compensation = SGP40_COMP_OFF;
				first_SHT40_measure = false;
				sht40_errors_counter++;
				// notificar error en primera lectura del sht40
				ESP_LOGE(TAG, "SHT40 error en primera lectura");
			}
			else
			{
				/* se usaran las ultimas mediciones hasta el maximo de intentos
				   seguidos fallidos que se permiten */
				if (sht40_errors_counter >= SENSORS_SERVICE_MAX_RETRIES)
				{
					// notificar error en sensor SHT40
					// abortar
					ESP_ERROR_CHECK(retSHT);
				}
				else
				{
					// notificar error en lectura
					ESP_LOGE(TAG, "SGP40 error en lectura, num errores seguidos %u", sht40_errors_counter);
					sht40_errors_counter++;
				}
			}
		}
		else
		{
			// se han obtenido mediciones del sht40, actualizar las ultimas
			// las mediciones raw ya se han actualizado en la llamada a la funcion de
			// medicion del sensor con alta resolucion
			last_temperature_celsius = sht40_temperature_signal_to_celsius(last_temperature_raw);
			last_humidity_rh = sht40_humidity_signal_to_RH(last_humidity_raw);

			ESP_LOGI(TAG, "SHT40 medicion correcta,temp_raw=%u, temp=%u, humedad_raw=%u, humedad=%u",
					last_temperature_raw,
					last_temperature_celsius,
					last_humidity_raw,
					last_humidity_rh);

			// reiniciar contador de errores del sensor sht40
			sht40_errors_counter = 0;

			// activar compensacion para el sgp40
			selected_compensation = SGP40_COMP_ON;

			// actualizar datos de compensacion para el sgp40
			sgp40_compensation_t_init(&last_sgp40_compensation,
						last_temperature_celsius,
						last_humidity_rh);
		}

		esp_err_t retSGP = sgp40_i2c_get_raw_measure(&last_VOC_raw,
								selected_compensation, &last_sgp40_compensation);

		if (retSGP != ESP_OK)
		{
			// casos de error de lectura del SGP40
			if (first_SGP40_measure)
			{
				first_SGP40_measure = false;
				sgp40_errors_counter++;
				// notificar error de primera lectura del SGP40
				ESP_LOGE(TAG, "SGP40 error en primera lectura");
				// no se va a alimentar el algoritmo de VOC index
				feed_voc_algorithm = false;
			}
			else
			{
				/* se usaran la ultima medicion que se hiciera del SGP40
				 * hasta el maximo de intentos seguidos fallidos que
				 * se permiten */
				if (sgp40_errors_counter >= SENSORS_SERVICE_MAX_RETRIES)
				{
					feed_voc_algorithm = false;
					// notificar error en sensor SGP40
					// abortar
					ESP_ERROR_CHECK(retSGP);
				}
				else
				{
					// Aun se va a alimentar el algoritmo de VOC index
					// con la ultima medicion valida del SGP40
					feed_voc_algorithm = true;
					sgp40_errors_counter++;
					// notificar error en lectura
					ESP_LOGE(TAG, "SGP40 error en lectura, num errores seguidos %u", sgp40_errors_counter);
				}
			}
		}
		else
		{
			/* Reiniciar contador de errores si las subsiguientes
			 * lecturas del sensor SGP40 son satisfactorias */
			sgp40_errors_counter = 0;
			feed_voc_algorithm = true;
		}

		if (feed_voc_algorithm)
		{
			bool mqtt_data_created = false;
			bool gui_data_created = false;
			Sensors_data_ptr mqtt_data = NULL;
			Sensors_data_ptr gui_data = NULL;

			GasIndexAlgorithm_process(&voc_algorithm_params, ((int32_t)last_VOC_raw), &last_VOC_index);

			ESP_LOGI(TAG, "Se ejecuta algoritmo VOC Index con datos:"
					"temp=%u, humedad=%u, voc_raw(uint16_t)=%u, voc_raw(int32_t)=%d , voc_index=%d",
					last_temperature_celsius, last_humidity_rh, last_VOC_raw, ((int)last_VOC_raw), ((int)last_VOC_index));

			// montar objeto para mandar por mqtt si se ha podido crear
			mqtt_data_created = sensors_type_create(&mqtt_data, last_VOC_raw, last_VOC_index,
					last_temperature_celsius, last_humidity_rh);
			ESP_LOGI(TAG, "Direccion de memoria de mqtt_data: %p", mqtt_data);
			if (mqtt_data_created)
			{
				// envio por mqtt
				if (gss_send_sensors_data(mqtt_data, GSS_ID_MQTT_SENDER) != ESP_OK)
				{
					ESP_LOGE(TAG, "Cola envio sensor data a mqtt llena, se descartara.");
				}
				else
				{
					ESP_LOGI(TAG, "Datos sensores enviado a cola mqtt.");
				}

				gui_data_created = sensors_type_create(&gui_data, last_VOC_raw, last_VOC_index,
									last_temperature_celsius, last_humidity_rh);
				ESP_LOGI(TAG, "Direccion de memoria de gui_data: %p", gui_data);

				if (gui_data_created)
				{
					// Realizar evaluacion para la activacion/desactivacion de alarmas
					// aqui, justo antes de enviar los datos de sensor a la gui para
					// lograr ahorrar memoria y que la implementacion sea correcta
					// ya que si queremos leer los mismo datos (gui_data) depues
					// de mandarlo por el GSS no tenemos garantias de que se haya
					// podido destruir antes de poder completar la evaluacion para las alarmas
					aqi_alarm_manager_evaluate(gui_data);

					if (gss_send_sensors_data(gui_data, GSS_ID_GUI) != ESP_OK)
					{
						ESP_LOGE(TAG, "Cola envio sensor data a LCD llena, se descartara.");
					}
					else
					{
						ESP_LOGI(TAG, "Datos sensores enviado a cola LCD.");
					}
				}
				else
				{
					ESP_LOGE(TAG, "No se pudo reservar memoria para datos de sensores (gui)");
				}
			}
			else
			{
				ESP_LOGE(TAG, "No se pudo reservar memoria para datos de sensores (mqtt)");
			}
		}

		// Dormimos la tarea hasta el siguiente ciclo de lectura
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime,  pdMS_TO_TICKS(milliseconds));
	}
}

esp_err_t sensors_service_init(int sample_rate_ms, i2c_port_t i2c_master_port, gpio_num_t sda_pin,
        gpio_num_t scl_pin)
{
	current_i2c_master_port = i2c_master_port;
	esp_err_t ret = ESP_FAIL;

	// Inicializa driver I2C
	// Utilizacion de los pines GPIO_NUM_26--> SDA GPIO_NUM_27--> SCL
	// Example:
	//ret = i2c_master_init(I2C_NUM_0, GPIO_NUM_26, GPIO_NUM_27, GPIO_PULLUP_ENABLE, false);
	ret = i2c_master_init(current_i2c_master_port, sda_pin, scl_pin, GPIO_PULLUP_ENABLE, false);
	ESP_RETURN_ON_ERROR(ret, TAG, "No se pudo iniciar el controlador de I2C");

	// Inicializa driver del sensor SGP40
	ret = sgp40_i2c_master_init(I2C_NUM_0);
	ESP_RETURN_ON_ERROR(ret, TAG, "No se pudo iniciar el driver de SGP40");

	// inicializa driver del sensor SHT40
	ret = sht40_i2c_master_init(I2C_NUM_0);
	ESP_RETURN_ON_ERROR(ret, TAG, "No se pudo iniciar el driver de SHT40");

	// inicializa algoritmo de calculo del VOC index
	GasIndexAlgorithm_init(&voc_algorithm_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);

	// iniciar una tarea para la lectura de los sensores
	if (sensors_service_task_handler == NULL)
	{
		// Inicializaciones siempre antes de lanzar tareas que
		// tengan que usen los modulos inicializados
		aqi_alarm_manager_init();
		// create task and set interval
//		if (xTaskCreate(sensors_sampling_task, "SensorsSrvTask", 2548,
//				(void*)sample_rate_ms, 4, &sensors_service_task_handler) != pdPASS)
		if (xTaskCreatePinnedToCore(sensors_sampling_task, "SensorsSrvTask", 3072,
				(void*)sample_rate_ms, 4, NULL, 1) != pdPASS)
		{
			ESP_LOGE(TAG, "Task for sensors service could not be allocated");
			ret = ESP_ERR_NO_MEM;
		}

		ret = ESP_OK;
	}

	return ret;
}

esp_err_t sensors_service_stop()
{
	esp_err_t ret = ESP_OK;

	if (sensors_service_task_handler != NULL)
	{
		vTaskDelete(sensors_service_task_handler);
		sensors_service_task_handler = NULL;
	}

	ret = i2c_master_close(current_i2c_master_port);
	ESP_RETURN_ON_ERROR(ret, TAG, "No se pudo cerrar el puerto i2c");

	return ret;
}
