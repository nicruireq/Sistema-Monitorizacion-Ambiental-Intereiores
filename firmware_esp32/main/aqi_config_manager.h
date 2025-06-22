/*
 * aqi_config_manager.h
 *
 *  Created on: 18 feb 2025
 *      Author: NRR
 */

#ifndef MAIN_AQI_CONFIG_MANAGER_H_
#define MAIN_AQI_CONFIG_MANAGER_H_

#include "esp_err.h"
#include "aqi_device_config_type.h"

// Config vars keys
#define AQI_KEY_SCREEN_TIME						"SCRT"
#define AQI_KEY_ROOM_NAME						"ROOM"
#define AQI_KEY_ALARM_TEMP_H					"TH"
#define AQI_KEY_ALARM_TEMP_L					"TL"
#define AQI_KEY_ALARM_HUMIDITY_H				"HH"
#define AQI_KEY_ALARM_HUMIDITY_L				"HL"
#define AQI_KEY_ALARM_VOC_INDEX_LIMIT			"VOCL"
#define AQI_KEY_WIFI_PROVISIONING_STATE			"WPRVST"

// numero de tags en aqi_config_var_t - 1 (el ultimo es un no-valor para inicializar
// variables de tipo aqi_config_var_t)
#define AQI_NUM_CFG_VARS						8u

typedef enum
{
	AQI_CV_SCREEN_TIME,
	AQI_CV_ROOM_NAME,
	AQI_CV_ALARM_TEMP_H,
	AQI_CV_ALARM_TEMP_L,
	AQI_CV_ALARM_HUMIDITY_H,
	AQI_CV_ALARM_HUMIDITY_L,
	AQI_CV_ALARM_VOC_INDEX_LIMIT,
	AQI_CV_WIFI_PROVISIONING_STATE,
	AQI_CV_NOT_VAR

} aqi_config_var_t;


/**
 * @brief Inicializa el gestor de configuración del dispositivo.
 *
 * @return
 *     - ESP_OK: Inicialización exitosa.
 *     - ESP_FAIL: Error en la inicialización.
 */
esp_err_t aqi_config_manager_init();

/**
 * @brief Obtiene el valor de una variable de configuración especifica.
 * 	      Esta operacion siempre se realiza desde la cache local.
 * 	      Esta opracion es thread-safe
 *
 * @param var La variable de configuración a obtener.
 * @param out_buffer Puntero al buffer donde se almacenará el valor obtenido.
 * @param len Longitud del buffer.
 *
 * @return
 *     - ESP_OK: Operación exitosa.
 *     - ESP_ERR_INVALID_ARG: out_buffer es NULL o su longitud (len)
 *     							no coincide con el tipo de datos para
 *     							la variable de configuracion var.
 */
esp_err_t aqi_config_manager_get(aqi_config_var_t var, void * out_buffer, size_t len);

/**
 * @brief Obtiene todos los datos de configuración del dispositivo. No comprueba
 * 		  si los datos se han marcado como cambiados o no.
 * 		  Esta funcion no es thread-safe y se puede ejecutar aunque el aqi_config_manager
 * 		  no haya sido inicializado con aqi_config_manager_init().
 *
 * @param config Puntero a la estructura donde se almacenarán los datos de configuración.
 * @param key_data_failed[out] Puntero a aqi_config_var_t, cuando se produce un error de lectura
 *        en la flash de una de las variables de config indica que variable produjo el error. Si
 *        no se han producido errores de lectura y todas las variables son accesibles en la flash
 *        devuelve. Esto permite utilizar la función para comprobar la existencia de las variables
 *        de configuracion en la flash
 *
 * @return
 *     - ESP_OK: Operación exitosa.
 *     - ESP_ERR_INVALID_ARG: Argumento inválido.
 *     - ESP_FAIL: Error al obtener los datos.
 *
 * @warning Lee desde la cache local
 */
esp_err_t aqi_config_manager_get_all(AQI_device_config_data_t_ptr config, aqi_config_var_t* key_data_failed);

/**
 * @brief Establece el valor de una variable de configuración específica.
 * 		  Esta operacion es thread-safe.
 *
 * @param var La variable de configuración a establecer.
 * @param to_write Puntero al valor que se va a escribir.
 * @param len Longitud del valor a escribir.
 *
 * @return
 *     - ESP_OK: Operación exitosa.
 *     - ESP_ERR_INVALID_ARG: Argumento inválido.
 *     - ESP_FAIL: Error al establecer el valor.
 *
 * @warning No es thread-safe. Lee directamente de la flash, no lee de la cahce local.
 * 			Para leer de la cache local usar aqi_config_manager_get
 */
esp_err_t aqi_config_manager_set(aqi_config_var_t var, void * to_write, size_t len);

/**
 * @brief Establece todos los datos de configuración del dispositivo.
 * 		  Esta operacion es thread-safe
 *
 * @param config Puntero a la estructura que contiene los datos de configuración.
 *
 * @return
 *     - ESP_OK: Operación exitosa.
 *     - ESP_ERR_INVALID_ARG: Argumento inválido.
 *     - ESP_FAIL: Error al establecer los datos.
 *
 * @warning No es thread-safe
 */
esp_err_t aqi_config_manager_set_all(const AQI_device_config_data_t_ptr config);

/**
 * @brief Realiza un reset de la configuracion de WiFi obtenida mediante
 *        aprovisionamiento Blufi y resetea el dispositivo para iniciar un
 *        nuevo aprovisionamiento.
 */
void aqi_config_manager_reset_wifi_provisioning();

/**
 * @brief Libera los recursos utilizados por el gestor de configuración del dispositivo.
 *
 * @return
 *     - ESP_OK: Operación exitosa.
 *     - ESP_FAIL: Error al liberar los recursos.
 */
esp_err_t aqi_config_manager_release();

/**
 * @brief Procesa datos JSON recibidos para leer nueva configuración.
 * 		  Actualiza la configuración que haya cambiado y marca como
 * 		  cambiados las variables que se hayan cambiado en memoria.
 *
 * @param incoming_data La variable de configuración a establecer.
 * @param incoming_data Puntero a cadena con datos JSON.
 * @param len Longitud del los datos recibidos.
 *
 * @return
 *     - ESP_OK: Procesamiento exitoso.
 *     - ESP_ERR_INVALID_ARG: Argumento inválido, incoming_data es NULL
 *     - ESP_FAIL: Procesamiento abortado.
 */
esp_err_t aqi_process_received_config_data(const char* incoming_data, int len);

#endif /* MAIN_AQI_CONFIG_MANAGER_H_ */
